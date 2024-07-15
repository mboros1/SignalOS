OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kernel_entry)

SECTIONS {
    . = 0x40000;

    .text BLOCK(4k) : ALIGN(4k)
    {
        *(.text)
        *(.text.*)
    }

    .rodata BLOCK(4k) : ALIGN(4k)
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data BLOCK(4k) : ALIGN(4k)
    {
        *(.data)
        *(.data.*)
        interrupt_descriptors = .;
        KEEP (*(.interrupt_descriptors));
    }

    .bss BLOCK(4k) : ALIGN(4k)
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

}
