ARCH := x86_64
TARGET := x86_64-elf

ifneq ($(shell command -v $(TARGET)-gcc 2>/dev/null),)
CC := $(TARGET)-gcc
LD := $(TARGET)-gcc
OBJCOPY := $(TARGET)-objcopy
TOOLCHAIN_TARGET :=
CAN_LINK := 1
else ifneq ($(shell command -v clang 2>/dev/null),)
CC := clang
LD := clang
OBJCOPY := llvm-objcopy
TOOLCHAIN_TARGET := --target=x86_64-unknown-elf
ifneq ($(shell command -v ld.lld 2>/dev/null),)
CAN_LINK := 1
LDFLAGS_LINKER := -fuse-ld=lld
else
CAN_LINK := 0
endif
else
CC := gcc
LD := gcc
OBJCOPY := objcopy
TOOLCHAIN_TARGET :=
CAN_LINK := 0
endif

CFLAGS := $(TOOLCHAIN_TARGET) -std=c11 -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mcmodel=kernel -m64 -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -Werror -Iinclude
LDFLAGS := $(TOOLCHAIN_TARGET) -T kernel/arch/x86_64/linker.ld -nostdlib -ffreestanding -z max-page-size=0x1000
GRUB_MKRESCUE := $(shell command -v grub-mkrescue 2>/dev/null)
ifeq ($(GRUB_MKRESCUE),)
GRUB_MKRESCUE := $(shell command -v x86_64-elf-grub-mkrescue 2>/dev/null)
endif

BUILD_DIR := build
ISO_DIR := $(BUILD_DIR)/iso
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
ISO_IMAGE := $(BUILD_DIR)/aurora.iso

SOURCES_C := \
	kernel/boot/boot_info.c \
	kernel/core/panic.c \
	kernel/core/printk.c \
	kernel/core/console.c \
	kernel/core/interrupt/init.c \
	kernel/core/memory/init.c \
	kernel/core/scheduler/init.c \
	kernel/core/syscall/init.c \
	kernel/core/debug/init.c \
	kernel/core/ipc/init.c \
	kernel/core/libk/init.c \
	kernel/core/init/stage.c \
	kernel/core/kmain.c \
	kernel/debug/trace.c \
	kernel/drivers/serial.c \
	kernel/drivers/vga_text.c \
	kernel/drivers/keyboard.c \
	kernel/drivers/pit.c \
	kernel/ipc/channel.c \
	kernel/arch/x86_64/cpu_ops.c \
	kernel/arch/x86_64/io_ops.c \
	kernel/arch/x86_64/cpuid.c \
	kernel/arch/x86_64/idt.c \
	kernel/arch/x86_64/isr.c \
	kernel/arch/x86_64/pic.c \
	kernel/mm/pmm.c \
	kernel/mm/paging.c \
	kernel/mm/kheap.c \
	kernel/mm/vmm.c \
	kernel/sched/scheduler.c \
	kernel/sys/syscall.c \
	kernel/lib/string.c \
	kernel/libk/ring.c \
	kernel/init/system_init.c

SOURCES_S := \
	kernel/arch/x86_64/boot.S \
	kernel/arch/x86_64/isr_stubs.S \
	kernel/sched/context_switch.S

USERLAND_SOURCES_C := \
	userland/lib/string.c \
	userland/lib/syscall.c \
	userland/init.c

OBJECTS := $(addprefix $(BUILD_DIR)/,$(SOURCES_C:.c=.o) $(SOURCES_S:.S=.o))
USERLAND_OBJECTS := $(addprefix $(BUILD_DIR)/,$(USERLAND_SOURCES_C:.c=.o))

.PHONY: all clean iso run objects

ifeq ($(CAN_LINK),1)
all: $(KERNEL_ELF) $(USERLAND_OBJECTS)
else
all: objects
	@echo "ELF linker not found. Built all objects; install x86_64-elf toolchain or lld for kernel.elf."
endif

objects: $(BUILD_DIR) $(OBJECTS) $(USERLAND_OBJECTS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(if $(filter userland/%,$<),-Iuserland/include,) -c $< -o $@

$(BUILD_DIR)/%.o: %.S
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(BUILD_DIR) $(OBJECTS)
	$(LD) $(LDFLAGS_LINKER) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o $@

iso: $(KERNEL_ELF)
ifeq ($(CAN_LINK),0)
	@echo "Cannot build ISO: no ELF linker available."
	@exit 1
endif
ifeq ($(GRUB_MKRESCUE),)
	@echo "Cannot build ISO: grub-mkrescue not found."
	@exit 1
endif
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf
	cp boot/grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $(ISO_IMAGE) $(ISO_DIR)

run: iso
ifeq ($(CAN_LINK),0)
	@echo "Cannot run: no ELF linker available."
	@exit 1
endif
	qemu-system-x86_64 -cdrom $(ISO_IMAGE) -serial stdio -no-reboot -no-shutdown

clean:
	rm -rf $(BUILD_DIR)
