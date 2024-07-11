#ifndef SIGNALOS_KERNEL_H
#define SIGNALOS_KERNEL_H

#include "x86-64.h"


// kernel page table (used for virtual memory)
extern x86_64_pagetable kernel_pagetable[];


// Hardware interrupt numbers
#define INT_IRQ                 32U
#define IRQ_TIMER               0
#define IRQ_KEYBOARD            1
#define IRQ_ERROR               19
#define IRQ_SPURIOUS            31



#endif // SIGNALOS_KERNEL_H
