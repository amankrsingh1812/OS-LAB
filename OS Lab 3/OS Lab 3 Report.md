## **Part B**: 

Refer the patch files in `Patch/PartB/`

**Task 1: kernel processes:**

Function `create_kernel_process()` is defined in `proc.c` which creates a kernel process and add it to the processes queue. The function first finds an empty slot in the process table and assigns it to the newly created process. Then it allocates kernel stack for the process, sets up `trapframe` ,  puts the `exit()`function which will be called upon return from context after `trapframe` in stack , sets up context and its `eip` is made equal to `entrypoint` function. Then the page table for the new process is created by calling `setupkvm()` , the name of the process is set as the input argument `name`  and `intproc` is made as its parent. Finally the state of process is changed to `RUNNABLE` .

```c
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

  // Set up new context to start executing at entrypoint,
  // which returns to kernexit.
  sp -= 4;
  *(uint*)sp = (uint)exit;		 // end the kernel process upon return from entrypoint()

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
```

Upon return from the `entrypoint()` .The `exit()` function will terminate the process and thereby preventing it to return to `user mode` from `kernel mode` . The `create_kernel_process()` function is called in `forkret` (only when `forket` is called from `initprocess`) to create two `kernel processes` namely `swapoutprocess` and `swapinprocess` .

```c
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

}
```

**Task 2: swapping out mechanism:**

The newly created kernel process `swapoutprocess` is responsible for swapping out of pages on demand. The `swappoutprocess` supports a request queue for the swapping requests which is created from the `struct swapqueue` .

```c
struct swapqueue{
  struct spinlock lock;
  char* qchan;
  char* reqchan;
  int front;
  int rear; 
  int size;  
  struct proc* queue[NPROC+1];
}
```

The `enqueue()` and `dequeue()` for the `swapqueue` are implemented as shown below:

```c
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
```

An instance `siq` of the `struct swapqueue` is used as a request queue for the `swapoutprocess` . Any access to the `swapqueue` is protected by a `spinlock` .    

**Task 3: swapping in mechanism:**



