/* PCI: Interrupt Routing Table found at 0x4010f000 size = 176 */

#include <arch/pirq_routing.h>

const struct irq_routing_table intel_irq_routing_table = {
	0x52495024, /* u32 signature */
	0x0100,     /* u16 version   */
	272,        /* u16 Table size 32+(15*devices)  */
	0x00,       /* u8 Bus 0 */
	0xf8,       /* u8 Device 1, Function 0 */
	0x0000,     /* u16 reserve IRQ for PCI */
	0x8086,     /* u16 Vendor */
	0x24d0,     /* Device ID */
	0x00000000, /* u32 miniport_data */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* u8 rfu[11] */
	0xc4,   /*  u8 checksum - mod 256 checksum must give zero */
	{  /* bus, devfn, {link, bitmap}, {link, bitmap}, {link, bitmap}, {link, bitmap}, slot, rfu  */
	    {0x00, (0x01<<3)|0, {{0x60, 0xdcf8}, {0x00, 0x0000}, {0x00, 0x0000}, {0x00, 0x0000}}, 0x00,  0x00},
	    {0x00, (0x02<<3)|0, {{0x60, 0xdcf8}, {0x00, 0x0000}, {0x00, 0x0000}, {0x00, 0x0000}}, 0x00,  0x00},
	    {0x00, (0x03<<3)|0, {{0x60, 0xdcf8}, {0x00, 0x0000}, {0x00, 0x0000}, {0x00, 0x0000}}, 0x00,  0x00},
	    {0x00, (0x04<<3)|0, {{0x60, 0xdcf8}, {0x00, 0x0000}, {0x00, 0x0000}, {0x00, 0x0000}}, 0x00,  0x00},
	    {0x00, (0x06<<3)|0, {{0x60, 0xdcf8}, {0x00, 0x0000}, {0x00, 0x0000}, {0x00, 0x0000}}, 0x00,  0x00},
	    {0x00, (0x1d<<3)|0, {{0x60, 0xdcf8}, {0x63, 0xdcf8}, {0x62, 0xdc78}, {0x6b, 0xdcf8}}, 0x00,  0x00},
	    {0x00, (0x1d<<3)|1, {{0x63, 0xdcf8}, {0x00, 0x0000}, {0x00, 0x0000}, {0x00, 0x0000}}, 0x00,  0x00},
	    {0x00, (0x1d<<3)|2, {{0x62, 0xdcf8}, {0x00, 0x0000}, {0x00, 0x0000}, {0x00, 0x0000}}, 0x00,  0x00},
	    {0x00, (0x1d<<3)|3, {{0x60, 0xdcf8}, {0x60, 0xdcf8}, {0x00, 0x0000}, {0x00, 0x0000}}, 0x00,  0x00},
	    {0x00, (0x1f<<3)|0, {{0x62, 0xdc78}, {0x61, 0xdcf8}, {0x00, 0x0000}, {0x00, 0x0000}}, 0x00,  0x00},
	    {0x00, (0x1f<<3)|1, {{0x62, 0xdc78}, {0x00, 0x0000}, {0x00, 0x0000}, {0x00, 0x0000}}, 0x00,  0x00},
	    {0x04, (0x02<<3)|0, {{0x62, 0xdc78}, {0x63, 0xdcf8}, {0x00, 0x0000}, {0x00, 0x0000}}, 0x00,  0x00},
	    {0x04, (0x02<<3)|1, {{0x62, 0xdc78}, {0x63, 0xdcf8}, {0x00, 0x0000}, {0x00, 0x0000}}, 0x00,  0x00},
	    {0x06, (0x02<<3)|0, {{0x60, 0xdc78}, {0x00, 0x0000}, {0x00, 0x0000}, {0x00, 0x0000}}, 0x06,  0x00},
	    {0x07, (0x02<<3)|0, {{0x60, 0xdc78}, {0x00, 0x0000}, {0x00, 0x0000}, {0x00, 0x0000}}, 0x07,  0x00}
	}
};
