#!/bin/bash

# Create the image file with the bootloader
dd if=obj/bootsector of=signalos.img bs=512 count=1

# add magic boot bytes
echo -n -e '\x55\xAA' | dd conv=notrunc of=signalos.img bs=1 seek=510


# Append the kernel ELF to the image file
dd if=obj/kernel of=signalos.img bs=512 seek=1

