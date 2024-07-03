set $boot_start = 0x7c00
set $init_boot_pagetable = 0x7c0d
set $real_to_prot = 0x7c3a
set $gdt = 0x7c70
set $gdtdesc = 0x7c80
set $boot_readsect = 0x7c90
set $boot = 0x7cf6

break *$boot_start
