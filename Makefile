I686GNU ?= i686-elf
x86_64GNU ?= x86_64-elf

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Iinclude -DPRINTF_DISABLE_SUPPORT_FLOAT 
ASMFLAGS = -Iinclude
LDFLAGS = -ffreestanding -O2 -lgcc -nostdlib
KERNEL64FLAGS = -m64 -ffreestanding -z max-page-size=0x1000 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -std=gnu99 -Wall -Wextra -mcmodel=large -Iinclude
LD64FLAGS = -ffreestanding -O2 -nostdlib -lgcc -mcmodel=large

QEMUARGS =  -machine q35 -cpu core2duo\
	   -drive if=pflash,format=raw,unit=0,file=$(OVMFDIR)/OVMF_CODE.fd,readonly=on\
	   -drive if=pflash,format=raw,unit=1,file=$(OVMFDIR)/OVMF_VARS.fd\
	   -cdrom noyau.iso

BUILD_DIR = build
SRC_DIR = src
OVMFDIR = meta/OVMF

all: $(BUILD_DIR)/noyau.bin $(BUILD_DIR)/kernel64.bin

clean:
	rm -rf $(BUILD_DIR)

image: $(BUILD_DIR)/noyau.bin $(BUILD_DIR)/kernel64.bin
	cp $(BUILD_DIR)/noyau.bin iso/boot/
	cp $(BUILD_DIR)/kernel64.bin iso/boot/
	grub-mkrescue -o noyau.iso iso

run: image
	qemu-system-x86_64 $(QEMUARGS) 

run_legacy: image
	qemu-system-x86_64 -m 4G -machine q35 -cpu core2duo -cdrom noyau.iso

$(BUILD_DIR)/kernel64.o: $(SRC_DIR)/bootstrap64.S
	$(x86_64GNU)-gcc -c $< -o $@ $(KERNEL64FLAGS) 

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(I686GNU)-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(I686GNU)-gcc $(ASMFLAGS) -c $< -o $@

$(BUILD_DIR)/font.o: meta/font.psf
	objcopy -O elf32-i386 -B i386 -I binary $< $(BUILD_DIR)/font.o

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
LOADER_ASM_FILES = $(filter-out $(SRC_DIR)/bootstrap64.S,$(ASM_FILES))
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(LOADER_ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)
OBJ_FILES += $(BUILD_DIR)/font.o

$(BUILD_DIR)/noyau.bin: $(SRC_DIR)/linker.ld $(OBJ_FILES) 
	$(I686GNU)-gcc $(LDFLAGS) -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/noyau.bin $(OBJ_FILES)

$(BUILD_DIR)/kernel64.bin: $(BUILD_DIR)/kernel64.o $(SRC_DIR)/kernel_link.ld
	$(x86_64GNU)-gcc -T $(SRC_DIR)/kernel_link.ld -o $(BUILD_DIR)/kernel64.bin $(BUILD_DIR)/kernel64.o $(LD64FLAGS)
