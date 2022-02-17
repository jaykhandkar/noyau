#ifndef _ACPI_H
#define _ACPI_H
#include <stdint.h>

struct RSDPDescriptor {
	char Signature[8];
	uint8_t Checksum;
	char OEMID[6];
	uint8_t Revision;
	uint32_t RsdtAddress;
} __attribute__ ((packed));

struct RSDPDescriptor20 {
	struct RSDPDescriptor firstPart;

	uint32_t Length;
	uint64_t XsdtAddress;
	uint8_t ExtendedChecksum;
	uint8_t reserved[3];
} __attribute__ ((packed));

struct ACPISDTHeader {
	char Signature[4];
	uint32_t Length;
	uint8_t Revision;
	uint8_t Checksum;
	char OEMID[6];
	char OEMTableID[8];
	uint32_t OEMRevision;
	uint32_t CreatorID;
	uint32_t CreatorRevision;
} __attribute__((packed));

struct ACPIMCFG {
	struct ACPISDTHeader sdtheader;
	uint64_t reserved;
	uint8_t entries[0];
} __attribute__((packed));

struct ACPIMCFG_BAR {
	uint64_t base; /* Base address of advanced configuration mechanism */
	uint16_t segment; /* PCI segment group number */
	uint8_t bus_start; /* Start PCI bus number encoded by this segment */
	uint8_t bus_end; /*End PCI bus number encoded by this segment */
	uint32_t reserved;
} __attribute__((packed));

struct ACPI_MADT {
	struct ACPISDTHeader header;
	uint32_t lapic_base;
	uint32_t flags;
	uint8_t ic_structure;
} __attribute((packed));

#endif
