I686GNU ?= i686-elf
x86_64GNU ?= x86_64-elf

# flags for 32 bit loader
CFLAGS = -O2 -std=gnu99 -ffreestanding -nostdlib -Wall -Wextra -Iinclude -DPRINTF_DISABLE_SUPPORT_FLOAT 
ASMFLAGS = -Iinclude -ffreestanding -nostdlib
LDFLAGS = -ffreestanding -lgcc -nostdlib -lgcc

# flags for 64 bit kernel
KERNEL64ASMFLAGS = -m64 -ffreestanding -z max-page-size=0x1000 -mno-red-zone -mno-mmx\
		   -mno-sse -mno-sse2 -std=gnu99 -Wall -Wextra -mcmodel=large -Iinclude
KERNEL64CFLAGS = -m64 -O2 -ffreestanding -z max-page-size=0x1000 -mno-red-zone -mno-mmx\
		 -mno-sse -mno-sse2 -std=gnu99 -Wall -Wextra -mcmodel=large -Iinclude
LD64FLAGS = -ffreestanding -O2 -nostdlib -lgcc -mcmodel=large -z max-page-size=0x1000

QEMUARGS = -m 4G -machine q35 -cpu core2duo\
	   -drive if=pflash,format=raw,unit=0,file=$(OVMFDIR)/OVMF_CODE.fd,readonly=on\
	   -drive if=pflash,format=raw,unit=1,file=$(OVMFDIR)/OVMF_VARS.fd\
	   -cdrom noyau.iso

BUILD_DIR = build
SRC_DIR = src
OVMFDIR = meta/OVMF

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)

LOADER_ASM_FILES = $(SRC_DIR)/multiboot_entry.S $(SRC_DIR)/gdt64.S $(SRC_DIR)/longmode.S
LOADER_C_FILES = $(SRC_DIR)/printf.c $(SRC_DIR)/loader.c $(SRC_DIR)/psf1.c\
		 $(SRC_DIR)/loader_paging.c $(SRC_DIR)/string.c 

KERNEL_ASM_FILES = $(filter-out $(LOADER_ASM_FILES),$(ASM_FILES))
KERNEL_C_FILES = $(filter-out $(LOADER_C_FILES),$(C_FILES))

LOADER_OBJ_FILES = $(LOADER_C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c32.o)
LOADER_OBJ_FILES += $(LOADER_ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s32.o)
LOADER_OBJ_FILES += $(BUILD_DIR)/font.o

KERNEL_OBJ_FILES = $(KERNEL_C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
KERNEL_OBJ_FILES += $(KERNEL_ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

$(BUILD_DIR)/loader.bin: $(SRC_DIR)/linker.ld $(LOADER_OBJ_FILES) 
	$(I686GNU)-gcc $(LDFLAGS) -T $(SRC_DIR)/linker.ld -o $@ $(LOADER_OBJ_FILES)

$(BUILD_DIR)/kernel64.bin: $(SRC_DIR)/kernel_link.ld $(KERNEL_OBJ_FILES) 
	$(x86_64GNU)-gcc -T $(SRC_DIR)/kernel_link.ld -o $@ $(KERNEL_OBJ_FILES) $(LD64FLAGS)


#recipes to build 64 bit kernel
$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(x86_64GNU)-gcc -c $< -o $@ $(KERNEL64CFLAGS) 

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	mkdir -p $(@D)
	$(x86_64GNU)-gcc -c $< -o $@ $(KERNEL64ASMFLAGS)

# recipes to build 32 bit loader
$(BUILD_DIR)/%_c32.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(I686GNU)-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%_s32.o: $(SRC_DIR)/%.S
	mkdir -p $(@D)
	$(I686GNU)-gcc $(ASMFLAGS) -c $< -o $@

$(BUILD_DIR)/font.o: meta/font.psf
	objcopy -O elf32-i386 -B i386 -I binary $< $(BUILD_DIR)/font.o

all: $(BUILD_DIR)/loader.bin $(BUILD_DIR)/kernel64.bin

clean:
	rm -rf $(BUILD_DIR)

image: $(BUILD_DIR)/loader.bin $(BUILD_DIR)/kernel64.bin
	cp $(BUILD_DIR)/loader.bin iso/boot/
	cp $(BUILD_DIR)/kernel64.bin iso/boot/
	grub-mkrescue -o noyau.iso iso

run: image
	qemu-system-x86_64 $(QEMUARGS) 

run_legacy: image
	qemu-system-i386 -m 4G -machine q35 -cpu core2duo -cdrom noyau.iso

