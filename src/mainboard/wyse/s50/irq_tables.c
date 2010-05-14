/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2010 Nils Jacobs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <arch/pirq_routing.h>

const struct irq_routing_table intel_irq_routing_table = {
	PIRQ_SIGNATURE,		/* u32 signature */
	PIRQ_VERSION,		/* u16 version */
	32 + 16 * CONFIG_IRQ_SLOT_COUNT,		/* there can be total CONFIG_IRQ_SLOT_COUNT devices on the bus */
	0x00,			/* Interrupt router bus */
	(0x0f << 3) | 0x0,	/* Interrupt router dev */
	0,			/* IRQs devoted exclusively to PCI usage */
	0x100b,			/* Vendor */
	0x2b,			/* Device */
	0,			/* Miniport */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* u8 rfu[11] */
	0xdc,			/* Checksum (has to be set to some value that
				 * would give 0 after the sum of all bytes
				 * for this structure (including checksum).
                                 */
	{
		/* bus,        dev | fn,   {link, bitmap}, {link, bitmap}, {link, bitmap}, {link, bitmap}, slot, rfu */
		{0x00, (0x0f << 3) | 0x0, {{0x01, 0x0800}, {0x02, 0x0020}, {0x03, 0x0400}, {0x04, 0x0400}}, 0x5, 0x0},
		{0x00, (0x0d << 3) | 0x0, {{0x04, 0x0400}, {0x03, 0x0400}, {0x02, 0x0020}, {0x01, 0x0800}}, 0x1, 0x0},
		{0x00, (0x0e << 3) | 0x0, {{0x01, 0x0800}, {0x02, 0x0020}, {0x03, 0x0400}, {0x04, 0x0400}}, 0x2, 0x0},
	}
};

unsigned long write_pirq_routing_table(unsigned long addr)
{
	return copy_pirq_routing_table(addr);
}
