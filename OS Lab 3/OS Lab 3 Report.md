### Part A: Lazy Allocation

We started Part A with the patch provided which just tricks the process into believing that it has the memory which it requested by updating the value of `proc->sz` (the size of the process) while not actually allocating any physical memory by commenting the call to `groproc(n)` in `sysproc.c`.

This means that any access to the above requested memory results in a page fault as in reality no such memory has been provided and hence, it is an illegal reference.
Our lazy allocator in such cases of page faults allocates one page from the free physical memory available to the process and also updates the page table about this new allocation. 

#### Handling the Page Fault

Since xv6 does not handle page faults by default, we added the case when trap caused is due to a page fault and called our handler function `allocSinglePg(...)` which actually performs the task of allocation, with the required paramenters.

```c
  // trap.c
  case T_PGFLT:				// line 80
    allocSinglePg(myproc()->pgdir, rcr2());
    break;
```

- `myproc()->pgdir` returns a pointer to the page directory of the process
- The function `rcr2()` returns the the virtaul address which caused the page fault.
- Note: Page directory is the outer level of the 2-level page table in xv6

#### Allocating a new page

The allocation of page and updation the page table is done in `allocSinglePg(...)` in `vm.c`:

```c
  // vm.c
  void						// line 252
  allocSinglePg(pde_t *pgdir, uint va)
  {
    char *mem;
    uint a;
    a = PGROUNDDOWN(va);
    
    mem = kalloc();
    if(mem == 0){
      cprintf("allocuvm out of memory (3)\n");
      return;
    }

    memset(mem, 0, PGSIZE);
    if(mappages(pgdir, (char*)a, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
      cprintf("allocuvm out of memory (4)\n");
      kfree(mem);
    }
  }
```

- First, it aligns the virtual address to the start of a page using `PGROUNDDOWN(...)` because that is the proper starting virtual address to which physical memory will be mapped.
- Second, it uses `kalloc()` which returns the physical memory from the free list.
- Next, if allocation was successful (memory was available and allocated), it is filled with 0s. 
- Finally, `mappages(...)` is called which uses `pgdir` to locate (and create, if required) the page table contained the corresponding virtual address `a` and creating a corresponding page table entry (only a single entry in our case as we want `PGSIZE` amount of memory) having physical address `P2V(mem)`, as obtained above with the permissions set to writable(`PTE_W`) and user process accessible(`PTE_U`).
- In case of failure in `mappages(...)`, aquired memory `mem` is freed using `kfree(mem)`.

#### Sample output

## **Part B**: 

Refer the patch files in `Patch/PartB/`

#### Task 1: kernel processes:

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

#### Task 2: swapping out mechanism:

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

#### Task 3: swapping in mechanism:

#### Task 4: Sanity Test

Our user program `memtest.c` creates *20 child processes*, each of which *iterates 20 times*, and each time *requests 4096 Bytes* using `malloc()`.

For the ***i****th child process*, in the ***j****th iteration*, the ***k****th byte* is set with the following function:

```c
  child_iter_byte[i][j][k] = ( i + j*k ) % 128
```

Every child process first iterates 20 times setting the byte values, after which it again iterates 20 times, comparing the stored value with the expected value, again computed using the above function.

Note: Each child is iterating 20 times in place of 10 times (as mentioned in assignment), because iterating for 10 times, doesn't cause the complete main memory to be used up. This main memory limit, set with `KERNBASE` cannot be set below `4MB` (due to initialisation requirements of the kernel), at which we need to iterate for more than 10 times for each child process to actually test the correctness of our swapper.

