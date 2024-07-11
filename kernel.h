#ifndef SIGNALOS_KERNEL_H
#define SIGNALOS_KERNEL_H

#include "x86-64.h"
#include "types.h"


// kernel page table (used for virtual memory)
extern x86_64_pagetable kernel_pagetable[];


// Hardware interrupt numbers
#define INT_IRQ                 32U
#define IRQ_TIMER               0
#define IRQ_KEYBOARD            1
#define IRQ_ERROR               19
#define IRQ_SPURIOUS            31

// Process descriptor type
typedef struct {
    x86_64_pagetable* pagetable;        // process's page table
    pid_t pid;                          // process ID
    int state;                          // process state (see above)
    regstate regs;                      // process's current registers
    // The first 4 members of `proc` must not change, but you can add more.
    size_t sleep_ts;
    size_t sleep_time;
} proc;

// exception_return
//    Return from an exception to user mode: load the page table
//    and registers and start the process back up. Defined in k-exception.S.
void exception_return(proc* p);


#endif // SIGNALOS_KERNEL_H
