I686GNU ?= i686-elf
x86_64GNU ?= x86_64-elf

# flags for 32 bit loader
LOADERCFLAGS = -O2 -std=gnu99 -ffreestanding -nostdlib -Wall -Wextra -Iinclude -DPRINTF_DISABLE_SUPPORT_FLOAT -DLOADER
LOADERASMFLAGS = -Iinclude -ffreestanding -nostdlib
LOADERLDFLAGS = -ffreestanding -lgcc -nostdlib -lgcc

# flags for 64 bit kernel
KERNEL64ASMFLAGS = -m64 -ffreestanding -z max-page-size=0x1000 -mno-red-zone -mno-mmx\
		   -mno-sse -mno-sse2 -std=gnu99 -Wall -Wextra -mcmodel=large -Iinclude
KERNEL64CFLAGS = -m64 -O2 -ffreestanding -z max-page-size=0x1000 -mno-red-zone -mno-mmx -DPRINTF_DISABLE_SUPPORT_FLOAT\
		 -mno-sse -mno-sse2 -std=gnu99 -Wall -Wextra -mcmodel=large -Iinclude
LD64FLAGS = -ffreestanding -O2 -nostdlib -lgcc -mcmodel=large -z max-page-size=0x1000

QEMUARGS =  -m 4G -machine q35 -cpu EPYC\
	   -drive if=pflash,format=raw,unit=0,file=$(OVMFDIR)/OVMF_CODE.fd,readonly=on\
	   -drive if=pflash,format=raw,unit=1,file=$(OVMFDIR)/OVMF_VARS.fd\
	   -cdrom noyau.iso

BUILD_DIR = build
SRC_DIR = src
OVMFDIR = meta/OVMF

LOADER_DIR = $(SRC_DIR)/kernel/loader
KERNEL_DIR = $(SRC_DIR)/kernel
LIB_DIR = $(SRC_DIR)/lib

LOADER_ASM_FILES = $(wildcard $(LOADER_DIR)/*.S)
LOADER_C_FILES = $(wildcard $(LOADER_DIR)/*.c)

KERNEL_ASM_FILES = $(wildcard $(KERNEL_DIR)/*.S)
KERNEL_C_FILES = $(wildcard $(KERNEL_DIR)/*.c)

LIB_C_FILES = $(wildcard $(LIB_DIR)/*.c)
LIB_OBJ_FILES = $(LIB_C_FILES:$(LIB_DIR)/%.c=$(BUILD_DIR)/%_c.o)

LOADER_LIB_FILES =  $(LIB_DIR)/string.c $(LIB_DIR)/psf1.c $(LIB_DIR)/printf.c

LOADER_OBJ_FILES = $(LOADER_C_FILES:$(LOADER_DIR)/%.c=$(BUILD_DIR)/%_c32.o)
LOADER_OBJ_FILES += $(LOADER_LIB_FILES:$(LIB_DIR)/%.c=$(BUILD_DIR)/%_c32.o)
LOADER_OBJ_FILES += $(LOADER_ASM_FILES:$(LOADER_DIR)/%.S=$(BUILD_DIR)/%_s32.o)
LOADER_OBJ_FILES += $(BUILD_DIR)/font32.o

KERNEL_OBJ_FILES = $(KERNEL_C_FILES:$(KERNEL_DIR)/%.c=$(BUILD_DIR)/%_c.o)
KERNEL_OBJ_FILES += $(LIB_OBJ_FILES)
KERNEL_OBJ_FILES += $(KERNEL_ASM_FILES:$(KERNEL_DIR)/%.S=$(BUILD_DIR)/%_s.o)
KERNEL_OBJ_FILES += $(BUILD_DIR)/font64.o

#recipes to build 64 bit kernel
$(BUILD_DIR)/%_c.o: $(KERNEL_DIR)/%.c
	mkdir -p $(@D)
	$(x86_64GNU)-gcc -c $< -o $@ $(KERNEL64CFLAGS) 

$(BUILD_DIR)/%_c.o: $(LIB_DIR)/%.c
	mkdir -p $(@D)
	$(x86_64GNU)-gcc -c $< -o $@ $(KERNEL64CFLAGS) 

$(BUILD_DIR)/%_s.o: $(KERNEL_DIR)/%.S
	mkdir -p $(@D)
	$(x86_64GNU)-gcc -c $< -o $@ $(KERNEL64ASMFLAGS)

# recipes to build 32 bit loader
$(BUILD_DIR)/%_c32.o: $(LOADER_DIR)/%.c
	mkdir -p $(@D)
	$(I686GNU)-gcc $(LOADERCFLAGS) -c $< -o $@

$(BUILD_DIR)/%_c32.o: $(LIB_DIR)/%.c
	mkdir -p $(@D)
	$(I686GNU)-gcc $(LOADERCFLAGS) -c $< -o $@

$(BUILD_DIR)/%_s32.o: $(LOADER_DIR)/%.S
	mkdir -p $(@D)
	$(I686GNU)-gcc $(LOADERASMFLAGS) -c $< -o $@

$(BUILD_DIR)/font32.o: meta/font.psf
	mkdir -p $(@D)
	objcopy -O elf32-i386 -B i386 -I binary $< $(BUILD_DIR)/font32.o

$(BUILD_DIR)/font64.o: meta/font.psf
	mkdir -p $(@D)
	objcopy -O elf64-x86-64 -B i386 -I binary $< $(BUILD_DIR)/font64.o

$(BUILD_DIR)/loader.bin: $(LOADER_DIR)/linker.ld $(LOADER_OBJ_FILES) 
	$(I686GNU)-gcc $(LOADERLDFLAGS) -T $(LOADER_DIR)/linker.ld -o $@ $(LOADER_OBJ_FILES)

$(BUILD_DIR)/kernel64.bin: $(KERNEL_DIR)/kernel_link.ld $(KERNEL_OBJ_FILES) 
	$(x86_64GNU)-gcc -T $(KERNEL_DIR)/kernel_link.ld -o $@ $(KERNEL_OBJ_FILES) $(LD64FLAGS)



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
	qemu-system-x86_64 -m 4G -machine q35 -cpu EPYC -cdrom noyau.iso

