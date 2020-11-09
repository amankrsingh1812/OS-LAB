#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "stat.h"
#include "fcntl.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

struct swapqueue{
  struct spinlock lock;
  char* qchan;
  char* reqchan;
  int front;
  int rear; 
  int size;  
  struct proc* queue[NPROC+1];
} soq, siq;

struct victim
{
  pte_t* pte;
  struct proc* pr;
  uint va; 
};

// ----------------------------------------

int
fdalloc(struct file *f)
{
  int fd;
  struct proc *curproc = myproc();

  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd] == 0){
      curproc->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}

struct inode*
create(char *path, short type, short major, short minor)
{
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if((dp = nameiparent(path, name)) == 0)
    return 0;
  ilock(dp);

  if((ip = dirlookup(dp, name, 0)) != 0){
    iunlockput(dp);
    ilock(ip);
    if(type == T_FILE && ip->type == T_FILE)
      return ip;
    iunlockput(ip);
    return 0;
  }

  if((ip = ialloc(dp->dev, type)) == 0)
    panic("create: ialloc");

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  iupdate(ip);

  if(type == T_DIR){  // Create . and .. entries.
    dp->nlink++;  // for ".."
    iupdate(dp);
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      panic("create dots");
  }

  if(dirlink(dp, name, ip->inum) < 0)
    panic("create: dirlink");

  iunlockput(dp);

  return ip;
}

int open_file(char *path, int omode) {
  // char *path;
  int fd;
  struct file *f;
  struct inode *ip;

  // if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
  //   return -1;

  begin_op();

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op();
      return -1;
    }
  } else {
    if((ip = namei(path)) == 0){
      end_op();
      return -1;
    }
    ilock(ip);
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  end_op();

  f->type = FD_INODE;
  f->ip = ip;
  f->off = 0;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
  return fd;
}


// static struct inode*
// create(char *path, short type, short major, short minor)
// {
//   struct inode *ip, *dp;
//   char name[DIRSIZ];

//   if((dp = nameiparent(path, name)) == 0)
//     return 0;
//   ilock(dp);

//   if((ip = dirlookup(dp, name, 0)) != 0){
//     iunlockput(dp);
//     ilock(ip);
//     if(type == T_FILE && ip->type == T_FILE)
//       return ip;
//     iunlockput(ip);
//     return 0;
//   }

//   if((ip = ialloc(dp->dev, type)) == 0)
//     panic("create: ialloc");

//   ilock(ip);
//   ip->major = major;
//   ip->minor = minor;
//   ip->nlink = 1;
//   iupdate(ip);

//   if(dirlink(dp, name, ip->inum) < 0)
//     panic("create: dirlink");

//   iunlockput(dp);

//   return ip;
// }

// Open inode if exists
// Otherwise create new
struct inode*
open_inode(char *name) {
  struct inode *ip;

  begin_op();
  
  ip = namei(name);
  if (ip)
    return ip;
  
  ip = create(name, T_FILE, 0, 0);
  if (!ip) {
    panic("Unable to create/open inode");
    end_op();
    return 0;
  }
  
  iunlock(ip);
  end_op();
  return ip;
}

// Write / Overwrite contents
// to opened inode
int
write_inode(struct inode *ip, char *addr, int n) {
  int max = ((MAXOPBLOCKS-1-1-2) / 2) * 512;
  int i = 0, off = 0, r;
  while(i < n){
    int n1 = n - i;
    if(n1 > max)
      n1 = max;

    begin_op();
    ilock(ip);
    if ((r = writei(ip, addr + i, off, n1)) > 0)
      off += r;
    iunlock(ip);
    end_op();

    if(r < 0)
      break;
    if(r != n1)
      panic("short filewrite");
    i += r;
  }
  return i == n ? n : -1;
}

// Create name string from
// PID and VA[32:13]
void
get_name(int pid, uint addr, char *name) {
  int i = 0;
  while (pid) {
    name[i++] = '0' + (pid%10);
    pid = pid / 10;
  } 
  name[i++] = '_';
  while (addr) {
    name[i++] = '0' + (addr%10);
    addr = addr / 10;
  }
  name[i] = 0;
}


int write_page(int pid, uint addr, char *buf){
  char name[14];

  get_name(pid, addr, name);
  
  int fd = open_file(name, O_CREATE|O_WRONLY);
  struct file *f;
  // if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0){
    cprintf("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFf\n");
    return -1;
  }
  int noc = filewrite(f, buf, 4096);
  if(noc < 0){
    cprintf("Unable to write. Exiting (proc.c::write_page)!!");
  }

  fileclose(f);
  return noc;
}

int
delete_page(char* path)
{
  struct inode *ip, *dp;
  struct dirent de;
  char name[DIRSIZ];
  uint off;


  begin_op();
  if((dp = nameiparent(path, name)) == 0){
    end_op();
    return -1;
  }

  ilock(dp);

  // Cannot unlink "." or "..".
  // if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
  //   goto bad;

  ip = dirlookup(dp, name, &off);
    // goto bad;
  ilock(ip);

  if(ip->nlink < 1)
    panic("unlink: nlink < 1");
  // if(ip->type == T_DIR && !isdirempty(ip)){
  //   iunlockput(ip);
  //   goto bad;
  // }

  memset(&de, 0, sizeof(de));
  if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
    panic("unlink: writei");
  if(ip->type == T_DIR){
    dp->nlink--;
    iupdate(dp);
  }
  iunlockput(dp);

  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);

  end_op();

  return 0;

// bad:
//   iunlockput(dp);
//   end_op();
//   return -1;
}

int read_page(int pid, uint addr, char *buf){
  char name[14];

  get_name(pid, addr, name);
  int fd = open_file(name, O_CREATE|O_RDONLY);
  struct file *f;
  // if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0){
    if(fd >= NOFILE)
    cprintf("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFf\n");
    return -1;
  }
  int noc = fileread(f, buf, 4096);
  if(noc < 0){
    cprintf("Unable to write. Exiting (proc.c::write_page)!!");
  }
  fileclose(f);
  delete_page(name);
  return noc;
}
// // Write page to disk using
// // PID, 20MSBs of VA, buffer
// void 
// write_page(int pid, uint addr, char *buf) {
//   // cprintf("Write start\n");
//   struct inode *ip;
//   char name[14];

//   get_name(pid, addr, name);
//   // cprintf("Write open\n");
//   ip = open_inode(name);

//   int bytesTranfer = write_inode(ip, buf, 4096);
//   if (bytesTranfer < 0) {
//     cprintf("Unable to write. Exiting (proc.c::write_page)!!");
//     return;
//   }
//   // cprintf("Write complete\n");
// }

// // Read one page from disk
// // for given PID, 20MSBs of VA
// // to buffer
// // Returns number of bytes read
// int 
// read_page(int pid, uint addr, char *buf) {
//   struct inode *ip;
//   char name[14];

//   get_name(pid, addr, name);
//   ip = open_inode(name);

//   ilock(ip);
//   int bytesTranfer = readi(ip, buf, 0, 4096);
//   iunlock(ip);

//   if (bytesTranfer < 0) {
//     cprintf("Unable to read. Exiting (proc.c::read_page)!!");
//     return -1;
//   }
//   cprintf("Read complete\n");
//   return bytesTranfer;
// }

// ----------------------------------------------
// struct frame{
//   struct proc* pr;
//   int empty;
//   pte_t* pte; 
// } frames[PHYSTOP/PGSIZE+1];

// void insertIntoFrames(uint pa, pte_t* pte, struct proc* pr){

// }

void enqueue(struct swapqueue* sq, struct proc* np){
  if(sq->size == NPROC) return; 
  sq->rear = (sq->rear + 1) % NPROC;
  sq->queue[sq->rear] = np; 
  sq->size++;  
}

struct proc* dequeue(struct swapqueue* sq){
  if (sq->size == 0) return 0; 
  struct proc* next = sq->queue[sq->front]; 
  sq->front = (sq->front + 1) % NPROC; 
  sq->size = sq->size - 1; 

  if(sq->size == 0){
    sq->front = 0;
    sq->rear = NPROC - 1;
  }

  return next; 
}



static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
  initlock(&soq.lock, "soq");
  initlock(&siq.lock, "siq");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
  // cprintf("SOQ4: %s\n", soq.lock.name);
  acquire(&soq.lock);
  soq.qchan = (void*)0xA8080;
  soq.reqchan = (void*)0xA8000;
  soq.front = soq.size = 0; 
  soq.rear = NPROC - 1;
  release(&soq.lock);
  // cprintf("RSOQ4\n");

  acquire(&siq.lock);
  siq.qchan = (void*)0xB8081;
  siq.reqchan = (void*)0xB8001;
  siq.front = siq.size = 0; 
  siq.rear = NPROC - 1;
  release(&siq.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
  {
    cprintf("%s ",myproc()->name);
    panic("sched locks ");
  }
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
    create_kernel_process("swapoutprocess",swapoutprocess);
    create_kernel_process("swapinprocess",swapinprocess);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

// End a kernel process
void kernexit()
{
  exit();
}

// This function create a kernel process and add it to the processes queue.
void create_kernel_process(const char *name, void (*entrypoint)())
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)kernexit;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)entrypoint;

  if((p->pgdir = setupkvm()) == 0)
    panic("kernel process: out of memory?");

  p->sz = PGSIZE;
  p->parent = initproc;
  p->cwd = idup(initproc->cwd);
  // cprintf("%s %d\n",name,sizeof(p->name));
  safestrcpy(p->name, name, sizeof(p->name));

  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);

  return;
}

// void testKernelProcess()
// {
//   release(&ptable.lock);
//   cprintf("Sucess\n");
//   while(1);
//   return;
// }

void chooseVictim(int pid){
  
  struct proc* p;
  struct victim victims[4]={{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
  pde_t *pte;
  // cprintf("chooseVictim begin\n");
  cprintf("%d\n",victims[0].pte);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state == UNUSED|| p->state == ZOMBIE || p->state == EMBRYO || p->pid < 5|| p->pid == pid)
        continue;
      if((uint)p->pgdir < KERNBASE)
        cprintf("S right\n");
      for(uint i = PGSIZE; i< p->sz; i += PGSIZE){
        pte = (pte_t*)getpte(p->pgdir, (void *) i);
        if(!((*pte) & PTE_U)||!((*pte) & PTE_P)||!((*pte) & PTE_W))
          continue;
        int idx =(((*pte)&(uint)96)>>5);
        victims[idx].pte = pte;
        victims[idx].va = i;
        victims[idx].pr = p;
        // if(idx==0)
        // cprintf("vic %d %u\n",p->pid,pte);
      }
  }
    cprintf("nic %x %d\n",victims[0].pte,pid);

  // cprintf("chooseVictim loop end\n");
  for(int i=0;i<4;i++)
  {
    if(victims[i].pte != 0)
    {
      pte = victims[i].pte;
      victims[i].pr->swapOutCount++;
      // cprintf("Victim begin %d\n",i);
      release(&ptable.lock);
      release(&soq.lock);
      write_page(victims[i].pr->pid, (victims[i].va)>>12, (void *)P2V(PTE_ADDR(*pte)));   
      acquire(&soq.lock);
      acquire(&ptable.lock);
      cprintf("%d Pid:%d %d %d %d\n",i,victims[i].pr->pid,*pte,(*pte&(~PTE_P)),victims[i].va);
      *pte = ((*pte)&(~PTE_P));
      kfree((char *)P2V(PTE_ADDR(*pte)));
      lcr3(V2P(victims[i].pr->pgdir)); 
      // cprintf("Victim end %d\n",victims[i].pr->pid);
      return;
    }
  }
  return;
}

void swapoutprocess(){
  cprintf("Sucess\n");
  sleep(soq.qchan, &ptable.lock);

  while(1){
    cprintf("\n\nENtering swapout\n");
    // cprintf("SOQ1\n");
    // pushcli();
    acquire(&soq.lock);
    while(soq.size){
      // struct proc* p=siq.queue[(siq.front + i)%NPROC];
      struct proc *p = dequeue(&soq);
      
      cprintf("PID: %d\n", p->pid);
      // cprintf("PID: %d\n", soq.queue[(soq.front + i)%NPROC]->pid);
      // ...
      // int victimframe = choosevictim();
      // victimframe++;
      // cprintf("chooseVictim start\n");

      // sti();
      chooseVictim(p->pid);
      // cli();

      // cprintf("chooseVictim end\n");

      p->satisfied = 1;
      // dequeue(&soq);
    }
    cprintf("\n\n");
    // acquire(&soq.lock);

    wakeup1(soq.reqchan);
    release(&soq.lock);
      // acquire(&ptable.lock);
    // popcli();
    sleep(soq.qchan, &ptable.lock);
    // release(&soq.lock);
    // acquire(&ptable.lock);
  }

}

void swapinprocess(){
  cprintf("Sucess\n");
  sleep(siq.qchan, &ptable.lock);
  while(1){
    cprintf("\n\nENtering swapin\n");
    cprintf("SOQ2\n");
    acquire(&siq.lock);
    while(siq.size){
      // struct proc* p=siq.queue[(siq.front + i)%NPROC];
      struct proc *p = dequeue(&siq);
      
      cprintf("PID: %d\n", p->pid);
      // ...
      // int victimframe = choosevictim();
      // victimframe++;
      release(&ptable.lock);
      release(&siq.lock);
      
      char* mem = kalloc();
      cprintf("kalloc done\n");
      read_page(p->pid,((p->trapva)>>12),mem);
      
      acquire(&siq.lock);
      acquire(&ptable.lock);
      cprintf("%d %d\n",*getpte(p->pgdir,(void *)p->trapva),p->swapOutCount);
      swapInMap(p->pgdir, (void *)PGROUNDDOWN(p->trapva), PGSIZE, V2P(mem));
      p->swapOutCount--;
      // siq.queue[(siq.front + i)%NPROC]->satisfied = 1;
    }
    cprintf("\n\n");
    wakeup1(siq.reqchan);
    release(&siq.lock);
    // release(&ptable.lock);
    sleep(siq.qchan, &ptable.lock);
    // acquire(&ptable.lock);
  }

}

void ps()
{
  struct proc *p;
    cprintf("PT1\n");
  acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state == UNUSED)
        continue;
      cprintf("Process: %s Pid: %d\n",p->name,p->pid);
    }
  release(&ptable.lock);
}


// void insertq(struct swapqueue* sq, struct proc* p){
  
//   return;
// }

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
// void
// sleep2(void *chan)
// {
//   struct proc *p = myproc();
  
//   if(p == 0)
//     panic("sleep");
  
//   acquire(&ptable.lock);  //DOC: sleeplock1
  
//   // Go to sleep.
//   p->chan = chan;
//   p->state = SLEEPING;
  
//   sched();

//   // Tidy up.
//   p->chan = 0;

//   // Reacquire original lock.
//   release(&ptable.lock);
// }

// void deq(){
//   acquire(&ptable.lock);

//   sleep(0001, &ptable.lock);

//   release(&ptable.lock);
// }





void submitToSwapOut(){
  struct proc* p = myproc();
  // pushcli();
  // cli();
  cprintf("submitToSwapOut %d\n",p->pid);
  acquire(&ptable.lock);
  p->satisfied = 0;

  acquire(&soq.lock);
  enqueue(&soq, p);
  wakeup1(soq.qchan);
  release(&soq.lock);

  // sti();
  // popcli();
  sleep(soq.reqchan, &ptable.lock);
  // cli();
  release(&ptable.lock);
  // sti();
  // popcli();

  return;

}

void submitToSwapIn(){
  struct proc* p = myproc();
  
  cli();
  // uint g=*getpte(p->pgdir,(void *)rcr2());
  cprintf("submitToSwapIn %d\n",p->trapva);
  acquire(&ptable.lock);

  acquire(&siq.lock);
    enqueue(&siq, p);
    wakeup1(siq.qchan);
  release(&siq.lock);
  
  sleep(siq.reqchan, &ptable.lock);
  
  release(&ptable.lock);
  sti();

  return;

}


