#ifndef SIGNALOS_KERNEL_H
#define SIGNALOS_KERNEL_H

#include "x86-64.h"
#include "types.h"


// kernel page table (used for virtual memory)
extern x86_64_pagetable kernel_pagetable[];


// Process state constants
#define P_FREE      0                   // free slot
#define P_RUNNABLE  1                   // runnable process
#define P_BLOCKED   2                   // blocked process
#define P_BROKEN    3                   // faulted process
#define P_SLEPT     4                   // sleeping process

// Process descriptor type
typedef struct proc {
    x86_64_pagetable* pagetable;        // process's page table
    pid_t pid;                          // process ID
    int state;                          // process state (see above)
    regstate regs;                      // process's current registers
    // The first 4 members of `proc` must not change, but you can add more.
    size_t sleep_ts;
    size_t sleep_time;
} proc;
// Process table
#define NPROC 16                // maximum number of processes
extern proc ptable[NPROC];

// Hardware interrupt numbers
#define INT_IRQ                 32U
#define IRQ_TIMER               0
#define IRQ_KEYBOARD            1
#define IRQ_ERROR               19
#define IRQ_SPURIOUS            31

// kernel values hard coded
#define KERNEL_START_ADDR       0x40000
#define KERNEL_STACK_TOP        0x80000
//
// Physical memory size
#define MEMSIZE_PHYSICAL        0x200000
// Number of physical pages
#define NPAGES                  (MEMSIZE_PHYSICAL / PAGESIZE)

// Segment selectors
#define SEGSEL_BOOT_CODE        0x8             // boot code segment
#define SEGSEL_KERN_CODE        0x8             // kernel code segment
#define SEGSEL_KERN_DATA        0x10            // kernel data segment
#define SEGSEL_APP_CODE         0x18            // application code segment
#define SEGSEL_APP_DATA         0x20            // application data segment
#define SEGSEL_TASKSTATE        0x28            // task state segment

// exception_return
//    Return from an exception to user mode: load the page table
//    and registers and start the process back up. Defined in k-exception.S.
void exception_return(proc* p);

// kalloc(sz)
//    Kernel memory allocator. Allocates `sz` contiguous bytes and
//    returns a pointer to the allocated memory, or `nullptr` on failure.
//
//    The returned memory is initialized to 0xCC, which corresponds to
//    the x86 instruction `int3` (this may help you debug). You'll
//    probably want to reset it to something more useful.
//
//    Currently, `kalloc` is a page-based allocator: if `sz > PAGESIZE`
//    the allocation fails; if `sz < PAGESIZE` it allocates a whole page
//    anyway.
//
//    The handout code returns the next allocatable free page it can find.
//    It never reuses pages or supports freeing memory (you'll change that).
void *kalloc(size_t sz);

// kfree(kptr)
//    Free `kptr`, which must have been previously returned by `kalloc`.
//    If `kptr == nullptr` does nothing.
void kfree(void *kptr);

void* memset(void *v, int c, size_t n);


#endif // SIGNALOS_KERNEL_H
