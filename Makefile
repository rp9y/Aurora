CROSS ?= x86_64-elf
CC := $(CROSS)-gcc
LD := $(CROSS)-gcc
OBJCOPY := $(CROSS)-objcopy
GRUB_MKRESCUE_UEFI ?= x86_64-elf-grub-mkrescue
GRUB_MKRESCUE_BIOS ?= i686-elf-grub-mkrescue
GRUB_MKSTANDALONE ?= x86_64-elf-grub-mkstandalone
MFORMAT ?= mformat
MMD ?= mmd
MCOPY ?= mcopy
QEMU_IMG ?= qemu-img
ISO_FLAVOR ?= uefi

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
ISO_DIR := $(BUILD_DIR)/iso
KERNEL_ELF := $(BUILD_DIR)/aurora.elf
ISO_IMAGE := $(BUILD_DIR)/aurora.iso
ISO_IMAGE_UEFI := $(BUILD_DIR)/aurora-uefi.iso
ISO_IMAGE_BIOS := $(BUILD_DIR)/aurora-bios.iso
UTM_IMG := $(BUILD_DIR)/aurora-utm.img
UTM_QCOW2 := $(BUILD_DIR)/aurora-utm.qcow2
UEFI_BOOTX64 := $(BUILD_DIR)/bootx64.efi

C_SRCS := $(shell find arch boot core -name '*.c')
S_SRCS := $(shell find arch boot core -name '*.S')
OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(C_SRCS)) \
        $(patsubst %.S,$(OBJ_DIR)/%.o,$(S_SRCS))

CFLAGS := -std=c11 -ffreestanding -fno-stack-protector -fno-pic \
          -m64 -mno-red-zone -mcmodel=kernel -Wall -Wextra \
          -Wa,--noexecstack -Iinclude
LDFLAGS := -nostdlib -T arch/x86_64/linker.ld -z max-page-size=0x1000

.PHONY: all kernel iso iso-uefi iso-bios prep-iso utm-img utm-qcow2 clean

all: kernel

kernel: $(KERNEL_ELF)

$(KERNEL_ELF): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

iso: $(KERNEL_ELF)
ifeq ($(ISO_FLAVOR),bios)
	$(MAKE) iso-bios
else
	$(MAKE) iso-uefi
endif

prep-iso: $(KERNEL_ELF)
	@rm -rf $(ISO_DIR)
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/aurora.elf
	cp boot/grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg

iso-uefi: prep-iso
	$(GRUB_MKRESCUE_UEFI) -o $(ISO_IMAGE_UEFI) $(ISO_DIR)
	cp $(ISO_IMAGE_UEFI) $(ISO_IMAGE)

iso-bios: prep-iso
	$(GRUB_MKRESCUE_BIOS) -o $(ISO_IMAGE_BIOS) $(ISO_DIR)
	cp $(ISO_IMAGE_BIOS) $(ISO_IMAGE)

$(UEFI_BOOTX64): boot/grub/grub.cfg
	@mkdir -p $(BUILD_DIR)
	$(GRUB_MKSTANDALONE) -O x86_64-efi \
		--modules="normal configfile multiboot2 part_gpt part_msdos fat search search_fs_file" \
		-o $(UEFI_BOOTX64) \
		"boot/grub/grub.cfg=boot/grub/grub.cfg"

utm-img: $(KERNEL_ELF) $(UEFI_BOOTX64)
	@rm -f $(UTM_IMG)
	dd if=/dev/zero of=$(UTM_IMG) bs=1m count=64
	$(MFORMAT) -i $(UTM_IMG) -F ::
	$(MMD) -i $(UTM_IMG) ::/EFI
	$(MMD) -i $(UTM_IMG) ::/EFI/BOOT
	$(MMD) -i $(UTM_IMG) ::/boot
	$(MMD) -i $(UTM_IMG) ::/boot/grub
	$(MCOPY) -i $(UTM_IMG) $(UEFI_BOOTX64) ::/EFI/BOOT/BOOTX64.EFI
	$(MCOPY) -i $(UTM_IMG) $(KERNEL_ELF) ::/boot/aurora.elf
	$(MCOPY) -i $(UTM_IMG) boot/grub/grub.cfg ::/boot/grub/grub.cfg

utm-qcow2: utm-img
	@rm -f $(UTM_QCOW2)
	$(QEMU_IMG) convert -f raw -O qcow2 $(UTM_IMG) $(UTM_QCOW2)

clean:
	rm -rf $(BUILD_DIR)
