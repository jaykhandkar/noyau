#include <fb.h>
#include <printf.h>
#include <multiboot2.h>
#include <acpi.h>
#include <pci.h>
#include <string.h>
#include <pgtable.h>

void setup_kernel_pgtables();
void clear(struct rgb_framebuffer *);

#define MB (1 << 20)

struct rgb_framebuffer rgb_fb;

void kernel_entry(unsigned long addr)
{
	struct multiboot_tag *tag;
	struct multiboot_tag *rsdp_tag = NULL;
	struct multiboot_tag_mmap *mmap_tag = NULL;

	setup_kernel_pgtables();

	addr = (unsigned long) phys_to_virt((void *)addr);
	for (tag = (struct multiboot_tag *) (addr + 8); tag->type != MULTIBOOT_TAG_TYPE_END;
			tag = (struct multiboot_tag *)((void *)tag + ((tag->size + 7) & ~7))) {
		switch (tag->type) {
			case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
			{
				struct multiboot_tag_framebuffer *tagfb = 
					(struct multiboot_tag_framebuffer *) tag;
				switch( tagfb->common.framebuffer_type) {
					case MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED:
						return;
					case MULTIBOOT_FRAMEBUFFER_TYPE_RGB:
						rgb_fb.base = (void *)tagfb->common.framebuffer_addr + 0xffff800000000000;
						rgb_fb.bpp = tagfb->common.framebuffer_bpp;
						rgb_fb.pitch = tagfb->common.framebuffer_pitch;
						rgb_fb.height = tagfb->common.framebuffer_height;
						rgb_fb.width = tagfb->common.framebuffer_width;
						rgb_fb.blue_pos = tagfb->framebuffer_blue_field_position;
						rgb_fb.blue_size = tagfb->framebuffer_blue_mask_size;
						rgb_fb.red_pos = tagfb->framebuffer_red_field_position;
						rgb_fb.red_size = tagfb->framebuffer_red_mask_size;
						rgb_fb.green_pos = tagfb->framebuffer_green_field_position;
						rgb_fb.green_size = tagfb->framebuffer_green_mask_size;
						break;
					case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT:
						return;
					default:
						return;
				}

			}
				break;
			case MULTIBOOT_TAG_TYPE_MMAP:
				mmap_tag = (struct multiboot_tag_mmap *) (tag);
				break;
			case MULTIBOOT_TAG_TYPE_ACPI_NEW:
			case MULTIBOOT_TAG_TYPE_ACPI_OLD:
				rsdp_tag = tag;
				break;
		}
	}


	clear(&rgb_fb);
	printk("hello from long mode.\n");
	
	if (mmap_tag != NULL) {
		multiboot_memory_map_t *mmap;
		char *mem_type[6] = { 0, "MULTIBOOT_MEMORY_AVAILABLE", "MULTIBOOT_MEMORY_RESERVED", "MULTIBOOT_MEMORY_ACPI_RECLAIMABLE",
			"MULTIBOOT_MEMORY_NVS", "MULTIBOOT_MEMORY_BADRAM" };
		uint64_t total_usable_mem = 0;

		printk("\nmemory map:\n");
		for (mmap = mmap_tag->entries;(void *) mmap < ((void *) mmap_tag + mmap_tag->size);
				mmap = (multiboot_memory_map_t *) ((void *)mmap + mmap_tag->entry_size)) {
			if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
				total_usable_mem += mmap->len;
			}

			printk("start addr: 0x%lx | length: 0x%lx | type: %s\n", mmap->addr, mmap->len, 
					mmap->type < 6 && mmap->type > 0 ? mem_type[mmap->type] : "(undefined)");	
		}

		printk("total usable memory = %d MB\n", total_usable_mem / MB);
	} else {
		printk("no memory map received from grub\n");
	}

	if (rsdp_tag == NULL) {
		printk("\nRSDP not received from GRUB\n");
		return;
	}

	struct ACPIMCFG *mcfg = NULL;
	struct ACPI_MADT *madt = NULL;

	if (rsdp_tag->type == MULTIBOOT_TAG_TYPE_ACPI_OLD) {
		struct multiboot_tag_new_acpi *old_acpi_tag = (struct multiboot_tag_new_acpi *) rsdp_tag;
		struct RSDPDescriptor *old_rsdp = (struct RSDPDescriptor *) old_acpi_tag->rsdp;
		
		if (strncmp(old_rsdp->Signature, "RSD PTR ", 8) != 0) {
			printk("invalid RSDP received\n");
			return;
		}

		struct ACPISDTHeader *rsdt = phys_to_virt((void *) (uint64_t)old_rsdp->RsdtAddress);
		size_t entries = (rsdt->Length - sizeof(struct ACPISDTHeader)) / 4;
		for (size_t i = 0; i < entries; i++) {
			struct ACPISDTHeader *hdr = (struct ACPISDTHeader *)phys_to_virt(
					(void *)(uint64_t)*(uint32_t *)((void *)rsdt + sizeof(struct ACPISDTHeader) + (i * 4)));
			if (strncmp(hdr->Signature, "MCFG", 4) == 0)
				mcfg = (struct ACPIMCFG *) hdr;
			if (strncmp(hdr->Signature, "APIC", 4) == 0)
				madt = (struct ACPI_MADT *)hdr;
		}

	} else {
		struct multiboot_tag_new_acpi *new_acpi_tag = (struct multiboot_tag_new_acpi *) rsdp_tag;
		struct RSDPDescriptor20 *rsdp = (struct RSDPDescriptor20 *) new_acpi_tag->rsdp;

		if (strncmp(rsdp->firstPart.Signature, "RSD PTR ", 8) != 0) {
			printk("invalid RSDP received\n");
			return;
		}
		struct ACPISDTHeader *xsdt_hdr = phys_to_virt((void *)rsdp->XsdtAddress);
		size_t entries = (xsdt_hdr->Length - sizeof(struct ACPISDTHeader)) / 8;

		for (size_t i = 0; i < entries; i ++) {
			struct ACPISDTHeader *hdr = (struct ACPISDTHeader *) phys_to_virt(
					(void *)*(uint64_t *) ((void *) xsdt_hdr + sizeof(struct ACPISDTHeader) + (i * 8)));
			if (strncmp(hdr->Signature, "MCFG", 4) == 0)
				mcfg = (struct ACPIMCFG *)hdr;
			if (strncmp(hdr->Signature, "APIC", 4) == 0)
				madt = (struct ACPI_MADT *)hdr;
		}
	}
	enumerate_pci(mcfg);

	if (madt != NULL) {
		printk("\nLAPIC base = 0x%x\n", madt->lapic_base);
		if (madt->flags == 0x1)
			printk("chipset has a legacy dual 8259\n");
	}
}
