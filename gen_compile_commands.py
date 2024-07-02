#!python

import os, json


template =   { "directory": "/Users/martinboros/git/SignalOS",
     "arguments": ['x86_64-unknown-linux-gnu-cc', '', '-I.', '-m64', '-mno-red-zone', '-mno-mmx', '-mno-sse', '-mno-sse2', '-mno-sse3', '-mno-3dnow', '-ffreestanding', '-fno-omit-frame-pointer', '-fno-pic', '-Wall', '-W', '-Wshadow', '-Wno-format', '-Wno-unused-parameter', '-Wstack-usage=1024', '-fno-stack-protector', '-std=gnu11', '-gdwarf', '-MD', '-MF', '.deps/boot.d', '-MP', '', '-Os', '-fomit-frame-pointer', '-DWEENSYOS_KERNEL', '-c', 'boot.c', '-o', 'obj/boot.o'],
     "file": "{{file}}" }

build_template = { "directory": "/Users/martinboros/git/SignalOS",
     "arguments": ['gcc-14', '-Wall', '-I.', '-W' '-std=gnu++1z'],
     "file": "build/mkbootdisk.cc" }

compile_commands = []

for file in os.listdir("."):
    if file.endswith(".c"):
        compile_command = json.dumps(template).replace('{{file}}', file)
        compile_commands.append(json.loads(compile_command))

compile_commands.append(build_template)

with open("compile_commands.json", "w") as f:
    f.write(json.dumps(compile_commands))
