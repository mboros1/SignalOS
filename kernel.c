#include "kernel.h"
#include "lapic.h"
#include "x86-64.h"
#include <stddef.h>
#include <stdint.h>

// VGA text-mode buffer
static uint16_t *const VGA_BUFFER = (uint16_t *)0xB8000;
static const int VGA_WIDTH = 80;
static const int VGA_HEIGHT = 25;

void syscall_entry();

static void init_kernel_memory();
static void init_interrupts();
static void init_cpu_state();

x86_64_pagetable kernel_pagetable[5];
uint64_t kernel_gdt_segments[7];
static x86_64_taskstate kernel_taskstate;

void *memset(void *v, int c, size_t n) {
  for (char *p = (char *)v; n > 0; ++p, --n) {
    *p = c;
  }
  return v;
}

// VGA color attributes
enum vga_color {
  COLOR_BLACK = 0,
  COLOR_BLUE = 1,
  COLOR_GREEN = 2,
  COLOR_CYAN = 3,
  COLOR_RED = 4,
  COLOR_MAGENTA = 5,
  COLOR_BROWN = 6,
  COLOR_LIGHT_GREY = 7,
  COLOR_DARK_GREY = 8,
  COLOR_LIGHT_BLUE = 9,
  COLOR_LIGHT_GREEN = 10,
  COLOR_LIGHT_CYAN = 11,
  COLOR_LIGHT_RED = 12,
  COLOR_LIGHT_MAGENTA = 13,
  COLOR_LIGHT_BROWN = 14,
  COLOR_WHITE = 15,
};

// Combine foreground and background colors into a VGA attribute byte
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
  return fg | bg << 4;
}

// Create a VGA entry (character and color)
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
  return (uint16_t)uc | (uint16_t)color << 8;
}

// Clear the VGA buffer
void clear_vga_buffer(uint16_t *buffer, uint8_t color) {
  for (int y = 0; y < VGA_HEIGHT; y++) {
    for (int x = 0; x < VGA_WIDTH; x++) {
      const size_t index = y * VGA_WIDTH + x;
      buffer[index] = vga_entry(' ', color);
    }
  }
}

// Print a string to the VGA buffer
void vga_print(const char *str, uint8_t color) {
  static size_t index = 0;
  for (size_t i = 0; str[i] != '\0'; i++) {
    if (index >= VGA_WIDTH * VGA_HEIGHT) {
      // If we reach the end of the screen, reset the index (simple scroll
      // simulation)
      // TODO: instead of wrapping to beginning, shift text up like a normal
      // terminal
      index = 0;
    }
    VGA_BUFFER[index++] = vga_entry(str[i], color);
  }
}

static void set_gate(x86_64_gatedescriptor* gate, uintptr_t addr,
                     int type, int dpl, int ist) {
  // TODO: need to implement panic syscall to use assert
    // assert((unsigned)type < 16 && (unsigned)dpl < 4 && (unsigned)ist < 8);
    gate->gd_low = (addr & 0x000000000000FFFFUL)
        | (SEGSEL_KERN_CODE << 16)
        | ((uint64_t)ist << 32)
        | ((uint64_t)type << 40)
        | ((uint64_t)dpl << 45)
        | X86SEG_P
        | ((addr & 0x00000000FFFF0000UL) << 32);
    gate->gd_high = addr >> 32;
}

void kernel_exception(regstate *regs) {
  // TODO: save registers to 'current' process
  // TODO: maybe some optional logging

  switch (regs->reg_intno) {
  case INT_IRQ: {
    // TODO: track ticks, handle lapic state
    // TODO: schedule next process
    break;
  }
  case INT_PF: {
    // TODO: implement page fault logic
    break;
  }
  default:
    // TODO: unhandled exception, put an error here
    return;
  }

  // TODO: schedule here in case we fall through case statement
}

void init_kernel_memory() {
  // TODO: initialize segments. references kernel_gdt_segments
  //        uses set_app_segment and set_sys_segment

  // TODO: initialize kernel page table.

  // TODO: user accessible mappings set up

  // TODO: wrcr3 set page table for some reason?
}

// processor state for taking an interrupt
extern x86_64_gatedescriptor interrupt_descriptors[256];

void init_interrupts() {
  // initialize interrupt descriptors
  // Macros in `k-exception.S` initialized `interrupt_descriptors[]`
  // with function pointers in the `gd_low` members. We must change
  // them to the weird format x86-64 expects.
  for (int i = 0; i < 256; ++i) {
    uintptr_t addr = interrupt_descriptors[i].gd_low;
    set_gate(&interrupt_descriptors[i], addr, X86GATE_INTERRUPT,
             i == INT_BP ? 3 : 0, 0);
  }

  // ensure machine has an enabled APIC
  // TODO: need to implement panic syscall to use assert
  // assert(cpuid(1).edx & (1 << 9));
  uint64_t apic_base = rdmsr(MSR_IA32_APIC_BASE);
  // assert(apic_base & IA32_APIC_BASE_ENABLED);
  // assert((apic_base & 0xFFFFFFFFF000) == lapic_pa);

  // disable the old programmable interrupt controller
#define IO_PIC1 0x20 // Master (IRQs 0-7)
#define IO_PIC2 0xA0 // Slave (IRQs 8-15)
  outb(IO_PIC1 + 1, 0xFF);
  outb(IO_PIC2 + 1, 0xFF);
}

void init_cpu_state() {
  memset(&kernel_taskstate, 0, sizeof(kernel_taskstate));
  kernel_taskstate.ts_rsp[0] = KERNEL_STACK_TOP;

  x86_64_pseudodescriptor gdt;
  gdt.limit = sizeof(kernel_gdt_segments) - 1;
  gdt.base = (uint64_t)kernel_gdt_segments;

  x86_64_pseudodescriptor idt;
  idt.limit = sizeof(interrupt_descriptors) - 1;
  idt.base = (uint64_t)interrupt_descriptors;

  // load segment descriptor tables
  asm volatile("lgdt %0; ltr %1; lidt %2"
               :
               : "m"(gdt.limit), "r"((uint16_t)SEGSEL_TASKSTATE), "m"(idt.limit)
               : "memory", "cc");

  // initialize segments
  asm volatile("movw %%ax, %%fs; movw %%ax, %%gs"
               :
               : "a"((uint16_t)SEGSEL_KERN_DATA));

  // set up control registers
  uint32_t cr0 = rdcr0();
  cr0 |= CR0_PE | CR0_PG | CR0_WP | CR0_AM | CR0_MP | CR0_NE;
  wrcr0(cr0);

  // set up syscall/sysret
  wrmsr(MSR_IA32_STAR, ((uintptr_t)SEGSEL_KERN_CODE << 32) |
                           ((uintptr_t)SEGSEL_APP_CODE << 48));
  wrmsr(MSR_IA32_LSTAR, (uint64_t)syscall_entry);
  wrmsr(MSR_IA32_FMASK, EFLAGS_TF | EFLAGS_DF | EFLAGS_IF | EFLAGS_IOPL_MASK |
                            EFLAGS_AC | EFLAGS_NT);

  // initialize local APIC (interrupt controller)
  lapicstate_t *lapic = lapic_get();
  lapic_enable(lapic, INT_IRQ + IRQ_SPURIOUS);

  // timer is in periodic mode
  lapic->reg[APIC_REG_TIMER_DIVIDE].v = TIMER_DIVIDE_1;
  lapic->reg[APIC_REG_LVT_TIMER].v = TIMER_PERIODIC | (INT_IRQ + IRQ_TIMER);
  lapic->reg[APIC_REG_TIMER_INITIAL_COUNT].v = 0;

  // disable logical interrupt lines
  lapic->reg[APIC_REG_LVT_LINT0].v = LVT_MASKED;
  lapic->reg[APIC_REG_LVT_LINT1].v = LVT_MASKED;

  // set LVT error handling entry
  lapic->reg[APIC_REG_LVT_ERROR].v = INT_IRQ + IRQ_ERROR;

  // clear error status by reading the error;
  // acknowledge any outstanding interrupts
  lapic_error(lapic);
  lapic_ack(lapic);
}

int kernel_main() {
  init_interrupts();
  init_cpu_state();
  // Clear the VGA buffer with black background and light grey text
  clear_vga_buffer(VGA_BUFFER, vga_entry_color(COLOR_LIGHT_GREY, COLOR_BLACK));

  // Print a welcome message in light green text
  const char *welcome_message = "Welcome to SignalOS!";
  vga_print(welcome_message, vga_entry_color(COLOR_LIGHT_GREEN, COLOR_BLACK));

  // Infinite loop
  for (;;) {
  }
}
