# SignalOS

# Roadmap
* implement amd64 kernel with vga console
  - page table for 64 bit addressing
  - interrupts
  - scrolling console buffer
  - syscalls
  - basic libc
  - read from keyboard
  - processes
* passing criteria
  - be able to run a lua REPL
  - boot on mini pc
* try porting an arm64 version
  - collect necessary arm code that matches x86.h functionality
  - look into boot loader specific code
* passing criteria
  - boot arm on qemu first,then a64 pineboard
* research what it takes to get hackrf drivers to work
* research file system options
* research 