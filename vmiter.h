#ifndef VMITER_H
#define VMITER_H

#include "x86-64.h"

static const int initial_perm = 0xFFF;
static const x86_64_pageentry_t zero_pe = 0;

typedef struct vmiter {
  x86_64_pagetable* pt;
  x86_64_pageentry_t* pep;
  int level;
  int perm;
  // current virtual address
  uintptr_t va;

} vmiter_t;

// current physical address of the iterator
uint64_t vmiter_pa(vmiter_t* it);

// current virtual address of the iterator
uintptr_t vmiter_va(vmiter_t* it);

// initialize a new virtual memory iterator from a pagetable
vmiter_t vmiter_init(x86_64_pagetable* pt);

// advance to next page table
void vmiter_next(vmiter_t* it);


int vmiter_map(vmiter_t* it, uintptr_t pa, int perm);

// advance virtual address by n
void vmiter_va_add(vmiter_t* it, unsigned long n);

#endif // VMITER_H
