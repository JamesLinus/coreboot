// This pirq table is dummy and only here for existance.
// It MUST be replaced as soon as LinuxBIOS is operable on this board.

#include <arch/pirq_routing.h>

const struct irq_routing_table intel_irq_routing_table = {
	PIRQ_SIGNATURE, /* u32 signature */
	PIRQ_VERSION,   /* u16 version   */
	32+16*5,        /* there can be total 5 devices on the bus */
	0,              /* Where the interrupt router lies (bus) */
	0x88,           /* Where the interrupt router lies (dev) */
	0x1c20,         /* IRQs devoted exclusively to PCI usage */
	0x1106,         /* Vendor */
	0x8231,         /* Device */
	0,              /* Crap (miniport) */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* u8 rfu[11] */
	0x5e,         /*  u8 checksum , this hase to set to some value that would give 0 after the sum of all bytes for this structure (including checksum) */
	{
		/* 8231 ethernet */
		{0,0x90, {{0x1, 0xdeb8}, {0x2, 0xdeb8}, {0x3, 0xdeb8}, {0x4, 0xdeb8}}, 0x1, 0},
		/* 8231 internal */
		{0,0x88, {{0x2, 0xdeb8}, {0x3, 0xdeb8}, {0x4, 0xdeb8}, {0x1, 0xdeb8}}, 0x2, 0},
		/* PCI slot */
		{0,0xa0, {{0x3, 0xdeb8}, {0x4, 0xdeb8}, {0x1, 0xdeb8}, {0x2, 0xdeb8}}, 0, 0},
		{0,0x50, {{0x4, 0xdeb8}, {0x3, 0xdeb8}, {0x2, 0xdeb8}, {0x1, 0xdeb8}}, 0x3, 0},
		{0,0x98, {{0x4, 0xdeb8}, {0x3, 0xdeb8}, {0x2, 0xdeb8}, {0x1, 0xdeb8}}, 0x4, 0},
	}
};
unsigned long write_pirq_routing_table(unsigned long addr)
{
        return copy_pirq_routing_table(addr);
}
