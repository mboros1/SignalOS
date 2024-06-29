#include "elf.h"
#include "types.h"
#include "x86-64.h"

#define SECTORSIZE 512
#define ELFHDR ((struct elf_header *)0x3000) // scratch space
#define KERNEL_START_SECTOR 1

__attribute__((noreturn)) void boot();
static void boot_readsect(uintptr_t dst, uint32_t src_sect);
static void boot_readseg(uintptr_t dst, uint32_t src_sect, size_t filesz,
                         size_t memsz);

// boot
// 	Load the kernel and jump to it.
__attribute__((noreturn)) void boot() {
  boot_readseg((uintptr_t)ELFHDR, KERNEL_START_SECTOR, PAGESIZE, PAGESIZE);
  while (ELFHDR->e_magic != ELF_MAGIC) {
  }

  struct elf_program *ph =
      (struct elf_program *)((uint8_t *)ELFHDR + ELFHDR->e_phoff);
  struct elf_program *eph = ph + ELFHDR->e_phnum;
  for (; ph < eph; ++ph) {
    boot_readseg(ph->p_va, KERNEL_START_SECTOR + ph->p_offset / SECTORSIZE,
                 ph->p_filesz, ph->p_memsz);
  }

  // jump to kernel, never to return
  typedef __attribute__((noreturn)) void (*KernelEntry)();
  KernelEntry kernel_entry = (KernelEntry)ELFHDR->e_entry;
  kernel_entry();
}

// boot_readseg(dst, src_sect, filesz, memsz)
//	Load an ELF segment at virtual address `dst` from the IDE disk's sector
//`src_sect`. 	Copes `filesz` bytes into memory at `dst` from sectors `src_sect`
//and up, then clears 	memory in the range `[dst+filesz, dst+memsz)`.
static void boot_readseg(uintptr_t ptr, uint32_t src_sect, size_t filesz,
                         size_t memsz) {
  uintptr_t end_ptr = ptr + filesz;
  memsz += ptr;

  // round down to the boundary of the sector
  ptr &= ~(SECTORSIZE - 1);

  // read sectors
  for (; ptr < end_ptr; ptr += SECTORSIZE, ++src_sect) {
    boot_readsect(ptr, src_sect);
  }

  // clear bss segment
  for (; end_ptr < memsz; ++end_ptr) {
    *(uint8_t *)end_ptr = 0;
  }
}

// boot_waitdisk
//	Wait for the disk to be ready.
static void boot_waitdisk() {
  // Wait until the ATA status register says ready (0x40 is on) and not busy
  // (0x80 is off)
  while ((inb(0x1F7) & 0xC0) != 0x40) {
    // do nothing
  }
}

// boot_readsect(dst, src_sect)
//	Read disk sector number `src_sect` into address `dst`.
static void boot_readsect(uintptr_t dst, uint32_t src_sect) {
  // programmed I/O for "read sector"
  boot_waitdisk();
  outb(0x1F2, 1); // send `count = 1` as an ATA argument
  outb(0x1F3, src_sect);
  outb(0x1F4, src_sect >> 8);
  outb(0x1F5, src_sect >> 16);
  outb(0x1F6, (src_sect >> 24) | 0xE0);
  outb(0x1F7, 0x20); // send the command: 0x20 = read sectors

  // move the data into memory
  boot_waitdisk();
  insl(0x1F0, (void *)dst, SECTORSIZE / 4); // read 128 words from the disk
}
