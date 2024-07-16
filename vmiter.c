#include "vmiter.h"
#include <stdint.h>
#include "kernel.h"

uint64_t vmiter_pa(vmiter_t *it) {
  if (*(it->pep) & PTE_P) {
    uintptr_t pa = *(it->pep) & PTE_PAMASK;
    if (it->level > 0) {
      pa &= ~0x1000UL;
    }
    return pa + (it->va & pageoffmask(it->level));
  } else {
    return -1;
  }
}

void vmiter_down(vmiter_t* it) {
    while (it->level > 0 && (*(it->pep) & (PTE_P | PTE_PS)) == PTE_P) {
        it->perm &= *(it->pep);
        --(it->level);
        uintptr_t pa = *(it->pep) & PTE_PAMASK;
        x86_64_pagetable* pt = (x86_64_pagetable*)pa;
        it->pep = &pt->entry[pageindex(it->va, it->level)];
    }
    // if ((*pep_ & PTE_PAMASK) >= 0x100000000UL) {
    //     panic("Page table %p may contain uninitialized memory!\n"
    //           "(Page table contents: %p)\n", pt_, *pep_);
    // }
}

uintptr_t vmiter_va(vmiter_t *it) { return it->va; }

void vmiter_real_find(vmiter_t *it, uintptr_t va) {
  if (it->level == 3 || ((it->va ^ va) & ~pageoffmask(it->level + 1)) != 0) {
    it->level = 3;
    if (va_is_canonical(va)) {
      it->perm = initial_perm;
      it->pep = &it->pt->entry[pageindex(va, it->level)];
    } else {
      it->perm = 0;
      it->pep = (x86_64_pageentry_t *)&zero_pe;
    }
  } else {
    int curidx = ((uintptr_t)it->pep & PAGEOFFMASK) >> 3;
    it->pep += pageindex(va, it->level) - curidx;
  }
  it->va = va;
  vmiter_down(it);
  // Implement your down() logic here if necessary
}

vmiter_t vmiter_init(x86_64_pagetable *pt) {
  vmiter_t it = {.pt = pt,
                 .pep = &pt->entry[0],
                 .level = 3,
                 .perm = initial_perm,
                 .va = 0};
  vmiter_real_find(&it, it.va);
  return it;
}


int vmiter_map(vmiter_t *it, uintptr_t pa, int perm) {
  if (pa == (uintptr_t)-1 && perm == 0) {
    pa = 0;
  }
  // assert(!(va_ & PAGEOFFMASK));
  // if (perm & PTE_P) {
  //   assert(pa != (uintptr_t)-1);
  //   assert((pa & PTE_PAMASK) == pa);
  // } else {
  //   assert(!(pa & PTE_P));
  // }
  // assert(!(perm & ~perm_ & (PTE_P | PTE_W | PTE_U)));

  while (it->level > 0 && perm) {
    // assert(!(*pep_ & PTE_P));
    x86_64_pagetable *pt = kalloc(PAGESIZE);
    if (!pt) {
      return -1;
    }
    memset(pt, 0, PAGESIZE);
    *(it->pep) = (uintptr_t)pt | PTE_P | PTE_W | PTE_U;
    vmiter_down(it);
  }

  if (it->level == 0) {
    *(it->pep) = pa | perm;
  }
  return 0;
}

void vmiter_va_add(vmiter_t *it, unsigned long n) {
  vmiter_real_find(it, it->va + n);
}
