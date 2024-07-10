#!/bin/bash

# File names
input_file="obj/kernel.full"
output_file="obj/kernel"

# List all sections in the input file
sections=$(objdump -h $input_file | grep -E '\.(text|rodata|data|bss)' | awk '{print $2}')

# Construct the objcopy command with all the .text sections
objcopy_cmd="objcopy"
for section in $sections; do
    objcopy_cmd="$objcopy_cmd -j $section"
done
objcopy_cmd="$objcopy_cmd $input_file $output_file"

# Execute the objcopy command
echo "Running: $objcopy_cmd"
$objcopy_cmd

