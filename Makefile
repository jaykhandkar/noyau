I686GNU ?= i686-elf

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Iinclude -DPRINTF_DISABLE_SUPPORT_FLOAT 
ASMFLAGS = -Iinclude
LDFLAGS = -ffreestanding -O2 -lgcc -nostdlib

BUILD_DIR = build
SRC_DIR = src
OVMFDIR = meta/OVMF

all: $(BUILD_DIR)/noyau.bin

clean:
	rm -rf $(BUILD_DIR)

image: $(BUILD_DIR)/noyau.bin
	cp $(BUILD_DIR)/noyau.bin iso/boot/
	grub-mkrescue -o noyau.iso iso

run: image
	qemu-system-i386 -m 4G -drive if=pflash,format=raw,unit=0,file=$(OVMFDIR)/OVMF_CODE.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=$(OVMFDIR)/OVMF_VARS.fd -cdrom noyau.iso

run_legacy: image
	qemu-system-i386 -cdrom noyau.iso

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(I686GNU)-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(I686GNU)-gcc $(ASMFLAGS) -c $< -o $@

$(BUILD_DIR)/font.o: meta/font.psf
	objcopy -O elf32-i386 -B i386 -I binary $< $(BUILD_DIR)/font.o

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)
OBJ_FILES += $(BUILD_DIR)/font.o

$(BUILD_DIR)/noyau.bin: $(SRC_DIR)/linker.ld $(OBJ_FILES) 
	$(I686GNU)-gcc $(LDFLAGS) -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/noyau.bin $(OBJ_FILES)
