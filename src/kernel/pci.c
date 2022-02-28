#include <acpi.h>
#include <pci.h>
#include <stdint.h>
#include <stddef.h>
#include <printf.h>
#include <string.h>
#include <pgtable.h>

const char* classes[] =  {
        "Unclassified",
        "Mass Storage Controller",
        "Network Controller",
        "Display Controller",
        "Multimedia Controller",
        "Memory Controller",
        "Bridge Device",
        "Simple Communication Controller",
        "Base System Peripheral",
        "Input Device Controller",
        "Docking Station", 
        "Processor",
        "Serial Bus Controller",
        "Wireless Controller",
        "Intelligent Controller",
        "Satellite Communication Controller",
        "Encryption Controller",
        "Signal Processing Controller",
        "Processing Accelerator",
        "Non Essential Instrumentation"
};

const char *get_class_name(uint8_t class)
{
	if (class > 0x13 && class < 0xFF)
		return "Reserved";
	else if (class == 0xFF)
		return "Device does not fit in any defined class";
	else 
		return classes[class];
}

void enumerate_pci(struct ACPIMCFG *mcfg)
{
	if (mcfg == NULL) {
		/*TODO: in this case, only type 1 access (0xCF8 and 0xCFC) is available.
		 * implement this */
		printk("\ncannot enumerate PCI buses: only I/O port configuration access method is available\n");
		return;
	}

	size_t segment_groups = (mcfg->sdtheader.Length - sizeof(struct ACPISDTHeader)) / 16;
	//printk("\nnumber of PCI segment groups: %u\n", segment_groups);

	for (size_t i = 0; i < segment_groups; i++) {
		struct ACPIMCFG_BAR *base_addr = (struct ACPIMCFG_BAR *)((void *)mcfg->entries + (sizeof(struct ACPIMCFG_BAR) * i));			
		/*printk("\nPCI segment group %u\n", base_addr->segment);
		printk("base address = 0x%lx\n", base_addr->base);
		printk("start bus: %u\n", base_addr->bus_start);
		printk("end bus: %u\n", base_addr->bus_end);*/

		printk("\nfunctions found in this PCI segment group:\n");
		for (uint8_t bus = base_addr->bus_start; bus < base_addr -> bus_end; bus++) {
			for (uint8_t dev = 0; dev < 32; dev++) {
				for (uint8_t func = 0; func < 8; func++) {
					struct pci_common_header *hdr = phys_to_virt((void *)(base_addr->base + (bus << 20)
							+ (dev << 15) + (func << 12)));
					if (hdr->vendor_id != 0xFFFF) {
						char *vendor_string;
						uint8_t type = hdr->header_type & 0x7F;

						switch(hdr->vendor_id) {
							case 0x8086:
								vendor_string = "Intel Corp.";
								break;
							case 0x10EC:
								vendor_string = "Realtek Semiconductor Co., Ltd.";
								break;
							case 0x1022:
								vendor_string = "Advanced Micro Devices, Inc. [AMD/ATI]";
								break;
							case 0xc0a9:
								vendor_string = "Micron/Crucial Technology";
								break;
							case 0x1344:
								vendor_string = "Micron Technology Inc";
								break;
							case 0x15b7:
								vendor_string = "Sandisk Corp";
								break;
							default:
								vendor_string = "";
						}
						printk("%04d:%04d.%04d 0x%04x 0x%02x 0x%02x 0x%02x %s %s | ", bus, dev, func, hdr->vendor_id, 
						       hdr->class, hdr->subclass, hdr->progif, vendor_string, get_class_name(hdr->class));
						if (type == 0x0) {
							struct pci_header_type0 *hdr0 = (struct pci_header_type0 *) hdr;
							printk("interrupt pin = %d line = %d\n", hdr0->intpin, hdr0->intline);
						} 
						//printk("this is a type %d header\n", type);
						else if (type == 0x1) {
							printk("PCI-PCI Bridge");
							struct pci_header_type1 *hdr1 = (struct pci_header_type1 *) hdr;
							printk(" secondary bus = %d subordinate bus = %d\n", hdr1->secondary_bus, hdr1->subordinate_bus);
						}
					}
				}
			}
		}
	}
}
