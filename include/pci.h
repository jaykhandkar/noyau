#ifndef _PCI_H
#define _PCI_H
#include <stdint.h>
#include <acpi.h>

/* common fields betwenn type 0 and type 1 header types */
struct pci_common_header {
	/* first dword */
	uint16_t vendor_id;
	uint16_t device_id;

	/*second dword... */
	uint16_t command;
	uint16_t status;

	uint8_t revision;
	uint8_t progif;
	uint8_t subclass;
/* this ensures the code cannot be compiled with a c++ compiler :) */
	uint8_t class;

	uint8_t cache_line;
	uint8_t latency_timer;
	uint8_t header_type;
	uint8_t BIST;
} __attribute__((packed));

/* type 0 header */
struct pci_header_type0 {
	struct pci_common_header common_header;

	uint32_t bar0;
	uint32_t bar1;
	uint32_t bar2;
	uint32_t bar3;
	uint32_t bar4;
	uint32_t bar5;

	uint32_t cardbus_cis;

	uint16_t subsytem_vendor_id;
	uint16_t subsytemd_id;

	uint32_t expansion_rom_base;

	uint8_t capabilities_ptr;
	uint8_t reserved[3];

	uint32_t reserved0;

	uint8_t intline;
	uint8_t intpin;
	uint8_t mingnt;
	uint8_t maxlat;
} __attribute__((packed));

struct pci_header_type1 {
	struct pci_common_header common_header;

	uint32_t bar0;
	uint32_t bar1;

	uint8_t primary_bus;
	uint8_t secondary_bus;
	uint8_t subordinate_bus;
	uint8_t subordinate_latency_timer;

	uint8_t io_base;
	uint8_t io_limit;
	uint16_t secondary_status;

	uint16_t mem_base;
	uint16_t mem_limit;

	uint16_t prefetch_mem_base;
	uint16_t prefetch_mem_limit;

	uint32_t prefetch_mem_base_high;
	uint32_t prefetch_mem_limit_high;

	uint16_t io_base_high;
	uint16_t io_limit_high;

	uint8_t capabilities_ptr;
	uint8_t reserved[3];

	uint32_t expansion_rom_base;

	uint8_t intline;
	uint8_t intpin;
	uint16_t bridge_control;
} __attribute__((packed));

void enumerate_pci(struct ACPIMCFG *);

#endif
