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
KERNEL_OBJS = $(OBJDIR)/kernel.ko
# add rest here
KERNEL_LINKER_FILES = link/kernel.ld link/shared.ld


-include build/rules.mk

$(OBJDIR)/%.ko: %.c $(KERNELBUILDSTAMPS)
	$(call compile,$(KERNELCFLAGS) -O1 -DSIGNALOS_KERNEL -c $< -o $@,COMPILE $<)

$(OBJDIR)/boot.o: $(OBJDIR)/%.o: boot.c $(KERNELBUILDSTAMPS)
	$(call compile,$(CFLAGS) -Os -fomit-frame-pointer -DSIGNALOS_KERNEL -c $< -o $@,COMPILE $<)

$(OBJDIR)/bootentry.o: $(OBJDIR)/%.o: \
	bootentry.S  $(KERNELBUILDSTAMPS)
	$(call assemble,-Os -fomit-frame-pointer -c $< -o $@,ASSEMBLE $<)


boot: $(BOOT_OBJS)

$(OBJDIR)/kernel.full: $(KERNEL_OBJS) $(PROCESS_BINARIES) $(KERNEL_LINKER_FILES)
	$(call link,-T $(KERNEL_LINKER_FILES) -o $@ $(KERNEL_OBJS) -b binary $(PROCESS_BINARIES),LINK)

$(OBJDIR)/bootsector: $(BOOT_OBJS) link/boot.ld link/shared.ld
	$(call link,-T link/boot.ld link/shared.ld -o $@.full $(BOOT_OBJS),LINK)
	$(call run,$(OBJDUMP) -C -S $@.full >$@.asm)
	$(call run,$(NM) -n $@.full >$@.sym)
	$(call run,$(OBJCOPY) -S -O binary -j .text $@.full $@)

$(OBJDIR)/kernel: $(OBJDIR)/kernel.full
	$(call run,$(OBJCOPY) -j .text -j .rodata -j .data -j .bss -j .ctors -j .init_array $<,STRIP,$@)


$(OBJDIR)/mkbootdisk: build/mkbootdisk.cc $(BUILDSTAMPS)
	$(call run,$(HOSTCXX) -I. -std=gnu++1z -g -o $@,HOSTCOMPILE,$<)

kernel: $(OBJDIR)/kernel

$(QEMUIMAGEFILES): $(OBJDIR)/mkbootdisk $(OBJDIR)/bootsector $(OBJDIR)/kernel
	$(call run,$(OBJDIR)/mkbootdisk $(OBJDIR)/bootsector $(OBJDIR)/kernel > $@,CREATE $@)

all: $(QEMUIMAGEFILES)

QEMUIMG = -M q35 \
	-device piix4-ide,bus=pcie.0,id=piix4-ide \
	-drive file=$(QEMUIMAGEFILES),if=none,format=raw,id=bootdisk \
	-device ide-hd,drive=bootdisk,bus=piix4-ide.0

run: run-$(QEMUDISPLAY)
	@:
run-graphic: $(QEMUIMAGEFILES) check-qemu
	@echo '* Run `gdb -x build/signalos.gdb` to connect gdb to qemu.' 1>&2
	$(call run,$(QEMU_PRELOAD) $(QEMU) $(QEMUOPT) -gdb tcp::12949 $(QEMUIMG),QEMU $<)
run-console: $(QEMUIMAGEFILES) check-qemu-console
	@echo '* Run `gdb -x build/signalos.gdb` to connect gdb to qemu.' 1>&2
	$(call run,$(QEMU) $(QEMUOPT) -curses -gdb tcp::12949 $(QEMUIMG),QEMU $<)
run-monitor: $(QEMUIMAGEFILES) check-qemu
	$(call run,$(QEMU_PRELOAD) $(QEMU) $(QEMUOPT) -monitor stdio $(QEMUIMG),QEMU $<)
run-gdb: run-gdb-$(QEMUDISPLAY)
	@:
run-gdb-graphic: $(QEMUIMAGEFILES) check-qemu
	$(call run,$(QEMU_PRELOAD) $(QEMU) $(QEMUOPT) -gdb tcp::12949 $(QEMUIMG) &,QEMU $<)
	$(call run,sleep 0.5; gdb -x build/signalos.gdb,GDB)
run-gdb-console: $(QEMUIMAGEFILES) check-qemu-console
	$(call run,$(QEMU) $(QEMUOPT) -curses -gdb tcp::12949 $(QEMUIMG),QEMU $<)

run-$(RUNSUFFIX): run
run-graphic-$(RUNSUFFIX): run-graphic
run-console-$(RUNSUFFIX): run-console
run-monitor-$(RUNSUFFIX): run-monitor
run-gdb-$(RUNSUFFIX): run-gdb
run-gdb-graphic-$(RUNSUFFIX): run-gdb-graphic
run-gdb-console-$(RUNSUFFIX): run-gdb-console

