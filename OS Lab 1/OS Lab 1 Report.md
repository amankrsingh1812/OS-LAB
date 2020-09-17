### Exercise 1: Inline Assembly

Refer to file [ex1.c](ex1.c)

The following inline assembly code will increment the value of x by 1.

```c
asm ("incl %0":"+r"(x));
```
- `incl` instruction increments the operand by 1.
- `+r` is used to allocate any free register to the variable x and use that register as both Input and Output
- `%0` corresponds to the register allocated to x.

---
### Exercise 3: Loading Kernel from Bootloader

**Trace**: Refer to file <FILE>



**(a)**  Following instructions change the addressing to 32 bit protected mode.

```assembly
0x7c1d: lgdt   gdtdesc 											# lgdt (%esi)
0x7c22: mov    %cr0,%eax
0x7c25: or     $0x1,%ax
0x7c29: mov    %eax,%cr0
0x7c2c: ljmp   $(SEG_KCODE<<3), $start32    # ljmp $0xb866,$0x87c31
```

**(b)**  The last instruction that bootloader executed is

```assembly
0x7d87:	call   *0x10018
```

This instructions is calling the entry function found in ELF Header. In `bootmain.c` this corresponds to following lines.

```C
entry = (void(*)(void))(elf->entry);
entry();
```

First instruction of kernel is

```assembly
0x10000c:	mov    %cr4,%eax
```

**(c)** This information is stored in elf header. First the bootloader loads first 4096 bytes (1st page) into memory. This page contains elf header which has an array of program headers. these program headers contains the size and offset of different segments of kernel which are then loaded into memory.

```c
// Load each program segment (ignores ph flags).
ph = (struct proghdr*)((uchar*)elf + elf->phoff);
eph = ph + elf->phnum;
for(; ph < eph; ph++){
  pa = (uchar*)ph->paddr;
  readseg(pa, ph->filesz, ph->off);
  if(ph->memsz > ph->filesz)
    stosb(pa + ph->filesz, 0, ph->memsz - ph->filesz);
}
```

---
### Exercise 4: 

Refer the output of &nbsp; `objdump -h kernel` - [objdump_kernel.png](objdump_kernel.png) 

Refer the output of &nbsp;`objdump -h bootblock.o` - [objdump_bootblock.png](objdump_bootblock.png) 

`objdump -h` &nbsp; displays the header of an executable file. In this case it displays the contents of program section headers of the ELF Binaries.

The important program sections in an ELF Binary - 
- `.text` - All the executable instructions of the program
- `.rodata` - The read-only data of the program like the ASCII string constants in C.
- `.data` - The initialized global and static variables in the program.
- `.bss` - The uninitialized global and static variables in the program.

Each section has the following information -
- `LMA(Load memory address)` - The address at which the section is actually loaded in the memory.
- `VMA(Virtaul memory address)` - The address at which the binary assumes the section will be loaded.
- `Size` - The size of the section.
- `Offset` - The offset from the beginning of the harddrive where the section is located at.
- `Algn` - The value to which the section is aligned in memory and in the file.
- `CONTENTS, ALLOC, LOAD, READONLY, DATA, CODE` - Flags which gives additional information regarding the section. Eg. Is it READONLY, should it be LOADED etc. 


---
### Exercise 7: Adding System Call

For creating a xsystem call we need to change 6 files:- user.h,  syscall.h, syscall.c, usys.S, defs.h, sysproc.c

```C
// user.h 
int wolfie(void* buf, uint size);		// line 26
```

```C
// syscall.h 
#define SYS_wolfie 22								// line 23
```

```C
// syscall.c
extern int sys_wolfie(void);				// line 106
[SYS_wolfie]  sys_wolfie,						// line 130
```

```C
// usys.S 
SYSCALL(wolfie)											// line 32
```

```C
// defs.h 
int wolfie(void*, uint);						// line 123
```

```C
// sysproc.c
int sys_wolfie(){										// line 94
	char* buf;
	uint size;
	if(argptr(0, (void*)&buf, sizeof(buf)) < 0) return -1;
	if(argptr(0, (void*)&size, sizeof(size)) < 0) return -1;
	
	static char wolf[] = \
"                         \n"
"                     .    \n"
"                    / V\\  \n"
"                  / `  /  \n"
"                 <<   |   \n"
"                 /    |   \n"
"               /      |   \n"
"             /        |   \n"
"           /    \\  \\ /    \n"
"          (      ) | |    \n"
"  ________|   _/_  | |    \n"
"<__________\\______)\\__)   \n"
"                          \n";
    					  
    static uint wolf_len = sizeof(wolf);
  
    if(size < wolf_len) return -1;
    
    int i = 0;
    while(wolf[i] != '\0'){
     	buf[i] = wolf[i];
    	++i;
    }
    buf[i] = '\0';
    
    return wolf_len;
}
```
---
### Exercise 8: User Level Application

We created `wolfietest.c` in which created a buffer and used system call to fill that buffer with wolf ASCII image. Then we printed this buffer to console using `printf`. 1st parameter in `printf` is file descriptor which is 1 for console out. At the end we used `exit` system call to exit from this program.

```C
// wolfietest.c
#include "types.h"
#include "user.h"

int main(int argc, char *argv[]){
	printf(1, "I am a wolf. wooo.....\n\n");
	char wolf[500];
	wolfie(wolf, 500);
	printf(1, wolf);
	exit();
}
```

In Makefile we need to add `_wolfietest\` to `UPROGS` and `wolfietest.c` to `XTRA	`.

```makefile
// Makefile
UPROGS=\
	_cat\
	_echo\
	_forktest\
	_grep\
	_init\
	_kill\
	_ln\
	_ls\
	_mkdir\
	_rm\
	_sh\
	_stressfs\
	_usertests\
	_wc\
	_zombie\
	_wolfietest\																																# line 184
	
XTRA=\
	mkfs.c ulib.c user.h cat.c echo.c forktest.c grep.c kill.c\
	ln.c ls.c mkdir.c rm.c stressfs.c usertests.c wc.c zombie.c wolfietest.c\  	# line 251
	

```

