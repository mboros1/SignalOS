###############################################################################
# Exception handlers
#
#   Assembly code defining kernel exception handlers
#   (for interrupts, traps, and faults).

// import constants from kernel.hh and x86-64.h
#include "k-asm.h"

.text

// kernel_entry
//    The bootloader jumps here after loading the kernel.
//    The code initializes `%rsp` to the top of the kernel stack,
//    then jumps to `kernel_start`.
.globl kernel_entry
kernel_entry:
        // initialize stack pointer and base pointer
        movq $KERNEL_STACK_TOP, %rsp
        movq %rsp, %rbp
        // clear `%rflags`
        pushq $0
        popfq
        // check for multiboot command line; if found pass it along
        cmpl $0x2BADB002, %eax
        jne 1f
        testl $4, (%rbx)
        je 1f
        movl 16(%rbx), %edi
        jmp 2f
1:      movq $0, %rdi
2:      jmp kernel_main

