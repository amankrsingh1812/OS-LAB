### Part A: Lazy Allocation

We started Part A with the patch provided which just tricks the process into believing that it has the memory requested by updating the value of `proc->sz` (the size of the process) while not actually allocating any physical memory by commenting the call to `groproc(n)` in `sysproc.c`.

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

---

<div style="page-break-after: always;"></div>

### Task 4: Sanity Test

Our user program `memtest.c` creates *20 child processes*, each of which *iterates 20 times*, and each time *requests 4096 Bytes* using `malloc()`.

For the ***i****th child process*, in the ***j****th iteration*, the ***k****th byte* is set with the following function:
```c
  child_iter_byte[i][j][k] = ( i + j*k ) % 128
``` 

Every child process first iterates 20 times setting the byte values, after which it again iterates 20 times, comparing the stored value with the expected value, again computed using the above function.

Note: Each child is iterating 20 times in place of 10 times (as mentioned in assignment), because iterating for 10 times, doesn't cause the complete main memory to be used up. This main memory limit, set with KERNBASE cannot be set below 4MB (due to initialisation requirements of the kernel), at which we need to iterate for more than 10 times for each child process to actually test the correctness of our swapper.