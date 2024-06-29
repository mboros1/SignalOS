QEMUIMAGEFILES = signalos.img

-include config.mk

# `V` controls whether the makefiles print full compiler shell commands (V=1)
# or brief commands (V=0, default).
# For brief commands, run `make all`
# For verbose commands, run `make V=1 all`
V = 0
ifeq ($(V),1)
	compile = $(CC) $(CFLAGS) $(DEPCFLAGS) $(1)
	assemble = $(CC) $(CFLAGS) $(DEPCFLAGS) $(ASFLAGS) $(1)
	link = $(LD) $(LDFLAGS) $(1)
	run = $(1) $(3)
else
	compile = @/bin/echo " " $(2) && $(CC) $(CFLAGS) $(DEPCFLAGS) $(1)
	assemble = @/bin/echo " " $(2) && $(CC) $(CFLAGS) $(DEPCFLAGS) $(ASFLAGS) $(1)
	link = @/bin/echo " " $(2) $(patsubst %.full,%,$@) && $(LD) $(LDFLAGS) $(1)
	run = @$(if $(2),/bin/echo " " $(2) $(3) &&,) $(1) $(3)
endif

# `D` controls how QEMU responds to faults. Run `make D=1 run` to
# ask QEMU to print debugging information about interrupts and CPU resets,
# and to quit after the first triple fault instead of rebooting.

# control number of CPUs used by QEMU
NCPU = 1
LOG ?= file:log.txt
QEMUOPT = -net none -parallel $(LOG) -smp $(NCPU)
ifeq ($(D),1)
	QEMUOPT += -d int,cpu_reset,guest_errors -no-reboot
endif


# Object files
BOOT_OBJS = $(OBJDIR)/bootentry.o $(OBJDIR)/boot.o
# add rest here


-include build/rules.mk

$(OBJDIR)/boot.o: $(OBJDIR)/%.o: boot.c $(KERNELBUILDSTAMPS)
	$(call compile,$(CXXFLAGS) -Os -fomit-frame-pointer -DWEENSYOS_KERNEL -c $< -o $@,COMPILE $<)

$(OBJDIR)/bootentry.o: $(OBJDIR)/%.o: \
	bootentry.S  $(KERNELBUILDSTAMPS)
	$(call assemble,-Os -fomit-frame-pointer -c $< -o $@,ASSEMBLE $<)


boot: $(BOOT_OBJS)
