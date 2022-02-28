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
	uint8_t ic_structure[0];
} __attribute((packed));

/* interrupct controller structure common header */

struct MADT_ICS_HDR {
	uint8_t type;
	uint8_t length;
} __attribute__((packed));

/* processor local APIC */
struct MADT_LAPIC {
	struct MADT_ICS_HDR header;
	uint8_t acpi_processor_uid;
	uint8_t apic_id;
	uint32_t flags;
} __attribute__ ((packed));

/* IO APIC */
struct MADT_IOAPIC {
	struct MADT_ICS_HDR header;
	uint8_t ioapic_id;
	uint8_t reserved;
	uint32_t ioapic_addr;
	uint32_t gsi_base;
} __attribute__((packed));

/* interrupt source override entry */
struct MADT_ISO {
	struct MADT_ICS_HDR header;
	uint8_t bus;
	uint8_t source;
	uint32_t gsi;
	uint16_t flags;
} __attribute__((packed));

/* NMI Source */
struct MADT_NMI_SRC {
	struct MADT_ICS_HDR header;
	uint16_t flags;
	uint32_t gsi;
} __attribute__((packed));

/* Local APIC NMI structure (indicates which of LINT[0:1] are connected to NMI */
struct MADT_LAPIC_NMI {
	struct MADT_ICS_HDR header;
	uint8_t acpi_processor_uid;
	uint16_t flags;
	uint8_t lint;
} __attribute((packed));

/* LAPIC address override */
struct MADT_LAPIC_ADDR {
	struct MADT_ICS_HDR header;
	uint16_t reserved;
	uint64_t lapic_addr;
} __attribute__((packed));
#endif
