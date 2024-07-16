#include "kernel.h"
#include "lapic.h"
#include "vmiter.h"
#include "x86-64.h"
#include <stddef.h>
#include <stdint.h>

// VGA text-mode buffer
static uint16_t *const VGA_BUFFER = (uint16_t *)0xB8000;
static const int VGA_WIDTH = 80;
static const int VGA_HEIGHT = 25;

#define PROC_SIZE 0x40000 // initial state only

proc ptable[NPROC]; // array of process descriptors
                    // Note that `ptable[0]` is never used.
proc *current;      // pointer to currently executing proc

#define HZ 100 // timer interrupt frequency (interrupts/sec)
int ticks;     // # timer interrupts so far

// Memory state
//    Information about physical page with address `pa` is stored in
//    `pages[pa / PAGESIZE]`. In the handout code, each `pages` entry
//    holds an `refcount` member, which is 0 for free pages.
//    You can change this as you see fit.

typedef struct pageinfo {
  uint8_t refcount;
} pageinfo;

bool pageinfo_used(pageinfo *page) { return page->refcount != 0; }

pageinfo pages[NPAGES];

void syscall_entry();

static void init_kernel_memory();
static void init_interrupts();
static void init_cpu_state();
uintptr_t syscall(regstate *regs);

x86_64_pagetable kernel_pagetable[5];
uint64_t kernel_gdt_segments[7];
static x86_64_taskstate kernel_taskstate;

void *memset(void *v, int c, size_t n) {
  for (char *p = (char *)v; n > 0; ++p, --n) {
    *p = c;
  }
  return v;
}

// reserved_physical_address(pa)
//    Returns true iff `pa` is a reserved physical address.

#define IOPHYSMEM 0x000A0000
#define EXTPHYSMEM 0x00100000

bool reserved_physical_address(uintptr_t pa) {
  return pa < PAGESIZE || (pa >= IOPHYSMEM && pa < EXTPHYSMEM);
}

uintptr_t round_up(uintptr_t ptr, unsigned long incr) {
  int offset = ptr % incr;
  if (offset) {
    return ptr - offset + incr;
  }
  return ptr;
}

// allocatable_physical_address(pa)
//    Returns true iff `pa` is an allocatable physical address, i.e.,
//    not reserved or holding kernel data.

bool allocatable_physical_address(uintptr_t pa) {
  extern char _kernel_end[];
  return !reserved_physical_address(pa) &&
         (pa < KERNEL_START_ADDR ||
          pa >= round_up((uintptr_t)_kernel_end, PAGESIZE)) &&
         (pa < KERNEL_STACK_TOP - PAGESIZE || pa >= KERNEL_STACK_TOP) &&
         pa < MEMSIZE_PHYSICAL;
}

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

static uintptr_t next_alloc_pa;

void *kalloc(size_t sz) {
  if (sz > PAGESIZE) {
    return NULL;
  }

  uintptr_t counter = 0;
  while (next_alloc_pa < MEMSIZE_PHYSICAL) {
    uintptr_t pa = next_alloc_pa;
    next_alloc_pa += PAGESIZE;
    counter += PAGESIZE;

    if (allocatable_physical_address(pa) &&
        !pageinfo_used(&pages[pa / PAGESIZE])) {
      pages[pa / PAGESIZE].refcount++;
      memset((void *)pa, 0xCC, PAGESIZE);
      return (void *)pa;
    }

    if (next_alloc_pa == MEMSIZE_PHYSICAL) {
      next_alloc_pa = PAGESIZE;
    }
    if (counter > MEMSIZE_PHYSICAL + PAGESIZE)
      return NULL;
  }
  return NULL;
}

// kfree(kptr)
//    Free `kptr`, which must have been previously returned by `kalloc`.
//    If `kptr == nullptr` does nothing.
void kfree(void *kptr) {
  // assert((uintptr_t)kptr % PAGESIZE == 0);
  // assert((uintptr_t)kptr < MEMSIZE_VIRTUAL);
  // assert((uintptr_t)kptr != 0);
  // assert(pages[(size_t)kptr / PAGESIZE].refcount > 0);

  pages[(size_t)kptr / PAGESIZE].refcount--;
  next_alloc_pa = 0;
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
    char c = str[i];
    if (c == '\n') {
      int offset = VGA_WIDTH - (index % VGA_WIDTH);
      for (int j = 0; j < offset; ++j) {
        VGA_BUFFER[index++] = vga_entry(' ', color);
      }
    } else {
      VGA_BUFFER[index++] = vga_entry(str[i], color);
    }
  }
}

static void set_gate(x86_64_gatedescriptor *gate, uintptr_t addr, int type,
                     int dpl, int ist) {
  // TODO: need to implement panic syscall to use assert
  // assert((unsigned)type < 16 && (unsigned)dpl < 4 && (unsigned)ist < 8);
  gate->gd_low = (addr & 0x000000000000FFFFUL) | (SEGSEL_KERN_CODE << 16) |
                 ((uint64_t)ist << 32) | ((uint64_t)type << 40) |
                 ((uint64_t)dpl << 45) | X86SEG_P |
                 ((addr & 0x00000000FFFF0000UL) << 32);
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

// syscall(regs)
//    System call handler.
//
//    The register values from system call time are stored in `regs`.
//    The return value, if any, is returned to the user process in `%rax`.
//
//    Note that hardware interrupts are disabled when the kernel is running.

int syscall_page_alloc(uintptr_t addr);
int syscall_fork();
int syscall_exit(pid_t pid);
int syscall_kill(pid_t pid);
int syscall_sleep(size_t time);

uintptr_t syscall(regstate *regs) {
  // Copy the saved registers into the `current` process descriptor.
  // TODO: handle multiple cores?
  // current->regs = *regs;
  // regs = &current->regs;

  // Actually handle the exception.
  switch (regs->reg_rax) {
    // TODO: implement
  }
  return 0;
}

// init_kernel_memory
//    Set up early-stage segment registers and kernel page table.
//
//    The early-stage segment registers and global descriptors are
//    used during hardware initialization. The kernel page table is
//    used whenever no appropriate process page table exists.
//
//    The interrupt descriptor table tells the processor where to jump
//    when an interrupt or exception happens. See k-exception.S.
//
//    The layouts of these types are defined by the hardware.

static void set_app_segment(uint64_t *segment, uint64_t type, int dpl) {
  *segment = type | X86SEG_S                     // code/data segment
             | ((uint64_t)dpl << 45) | X86SEG_P; // segment present
}

static void set_sys_segment(uint64_t *segment, uintptr_t addr, size_t size,
                            uint64_t type, int dpl) {
  segment[0] = ((addr & 0x0000000000FFFFFFUL) << 16) |
               ((addr & 0x00000000FF000000UL) << 32) |
               ((size - 1) & 0x0FFFFUL) | (((size - 1) & 0xF0000UL) << 48) |
               type | ((uint64_t)dpl << 45) | X86SEG_P; // segment present
  segment[1] = addr >> 32;
}

x86_64_pagetable kernel_pagetable[5];
uint64_t kernel_gdt_segments[7];
static x86_64_taskstate kernel_taskstate;

void init_kernel_memory() {
  // stash_kernel_data(false);

  // initialize segments
  kernel_gdt_segments[0] = 0;
  set_app_segment(&kernel_gdt_segments[SEGSEL_KERN_CODE >> 3],
                  X86SEG_X | X86SEG_L, 0);
  set_app_segment(&kernel_gdt_segments[SEGSEL_KERN_DATA >> 3], X86SEG_W, 0);
  set_app_segment(&kernel_gdt_segments[SEGSEL_APP_CODE >> 3],
                  X86SEG_X | X86SEG_L, 3);
  set_app_segment(&kernel_gdt_segments[SEGSEL_APP_DATA >> 3], X86SEG_W, 3);
  set_sys_segment(&kernel_gdt_segments[SEGSEL_TASKSTATE >> 3],
                  (uintptr_t)&kernel_taskstate, sizeof(kernel_taskstate),
                  X86SEG_TSS, 0);
  x86_64_pseudodescriptor gdt;
  gdt.limit = (sizeof(uint64_t) * 3) - 1;
  gdt.base = (uint64_t)kernel_gdt_segments;

  asm volatile("lgdt %0" : : "m"(gdt.limit));

  // initialize kernel page table
  memset(kernel_pagetable, 0, sizeof(kernel_pagetable));
  kernel_pagetable[0].entry[0] =
      (x86_64_pageentry_t)&kernel_pagetable[1] | PTE_P | PTE_W | PTE_U;
  kernel_pagetable[1].entry[0] =
      (x86_64_pageentry_t)&kernel_pagetable[2] | PTE_P | PTE_W | PTE_U;
  kernel_pagetable[2].entry[0] =
      (x86_64_pageentry_t)&kernel_pagetable[3] | PTE_P | PTE_W | PTE_U;
  kernel_pagetable[2].entry[1] =
      (x86_64_pageentry_t)&kernel_pagetable[4] | PTE_P | PTE_W | PTE_U;

  // the kernel can access [1GiB,4GiB) of physical memory,
  // which includes important memory-mapped I/O devices
  kernel_pagetable[1].entry[1] = (1UL << 30) | PTE_P | PTE_W | PTE_PS;
  kernel_pagetable[1].entry[2] = (2UL << 30) | PTE_P | PTE_W | PTE_PS;
  kernel_pagetable[1].entry[3] = (3UL << 30) | PTE_P | PTE_W | PTE_PS;

  // user-accessible mappings for physical memory,
  // except that (for debuggability) nullptr is totally inaccessible
  for (vmiter_t it = vmiter_init(kernel_pagetable);
       vmiter_va(&it) < MEMSIZE_PHYSICAL;
       vmiter_va_add(&it, PAGESIZE)) {
    if (vmiter_va(&it) != 0) {
      vmiter_map(&it, vmiter_va(&it), PTE_P | PTE_W | PTE_U);
    }
  }

  wrcr3((uintptr_t)kernel_pagetable);

  // Now that boot-time structures (pagetable and global descriptor
  // table) have been replaced, we can reuse boot-time memory.
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

// Simple integer to ASCII conversion
void itoa(int value, char *str, int base) {
  char *ptr;
  char *low;
  // Set up the lookup table
  char *digits = "0123456789ABCDEF";
  // Handle negative numbers for decimal base
  if (value < 0 && base == 10) {
    *str++ = '-';
    value = -value;
  }
  ptr = str;
  // Convert to the chosen base
  do {
    *ptr++ = digits[value % base];
    value /= base;
  } while (value);
  // Null terminate the string
  *ptr-- = '\0';
  // Reverse the string
  for (low = str; low < ptr; low++, ptr--) {
    char temp = *low;
    *low = *ptr;
    *ptr = temp;
  }
}

// Print a hex value to the VGA console
void print_hex(uint32_t value, uint8_t color) {
  char buffer[9];
  itoa(value, buffer, 16);
  vga_print("0x", color);
  vga_print(buffer, color);
}

int kernel_main() {
  init_kernel_memory();
  // init_interrupts();
  // init_cpu_state();
  // Clear the VGA buffer with black background and light grey text
  clear_vga_buffer(VGA_BUFFER, vga_entry_color(COLOR_LIGHT_GREY, COLOR_BLACK));

  // Print a welcome message in light green text
  const char *welcome_message = "Welcome to SignalOS!\n";
  vga_print(welcome_message, vga_entry_color(COLOR_LIGHT_GREEN, COLOR_BLACK));

  // Get CPUID information with leaf 0x01 to get the number of cores and other
  // info
  x86_64_cpuid_t cpu_info = cpuid(0x1);

  // Extract number of cores (logical processors)
  uint8_t num_cores = (cpu_info.ebx >> 16) & 0xFF;

  // Print CPUID information to the VGA console
  vga_print("CPUID Info:\nEAX: ",
            vga_entry_color(COLOR_LIGHT_GREY, COLOR_BLACK));
  print_hex(cpu_info.eax, vga_entry_color(COLOR_LIGHT_GREY, COLOR_BLACK));

  vga_print("\nEBX: ", vga_entry_color(COLOR_LIGHT_GREY, COLOR_BLACK));
  print_hex(cpu_info.ebx, vga_entry_color(COLOR_LIGHT_GREY, COLOR_BLACK));

  vga_print("\nECX: ", vga_entry_color(COLOR_LIGHT_GREY, COLOR_BLACK));
  print_hex(cpu_info.ecx, vga_entry_color(COLOR_LIGHT_GREY, COLOR_BLACK));

  vga_print("\nEDX: ", vga_entry_color(COLOR_LIGHT_GREY, COLOR_BLACK));
  print_hex(cpu_info.edx, vga_entry_color(COLOR_LIGHT_GREY, COLOR_BLACK));

  // Print number of cores
  vga_print("\nNumber of cores: ",
            vga_entry_color(COLOR_LIGHT_GREY, COLOR_BLACK));
  char num_cores_str[4];
  itoa(num_cores, num_cores_str, 10);
  vga_print(num_cores_str, vga_entry_color(COLOR_LIGHT_GREY, COLOR_BLACK));

  // Infinite loop
  for (;;) {
  }
}
