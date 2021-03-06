/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2006 Jon Dufresne <jon.dufresne@gmail.com>
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

#ifndef NORTHBRIDGE_INTEL_I855_RAMINIT_H
#define NORTHBRIDGE_INTEL_I855_RAMINIT_H

/* i855 Northbridge PCI device */
#define NORTHBRIDGE         PCI_DEV(0, 0, 0)
#define NORTHBRIDGE_MMC     PCI_DEV(0, 0, 1)

/* The i855 supports max. 2 dual-sided SO-DIMMs. */
#define DIMM_SOCKETS 2

/* DIMM0 is at 0x50, DIMM1 is at 0x51. */
#define DIMM_SPD_BASE   0x50

struct mem_controller {
  device_t d0;
  uint16_t channel0[DIMM_SOCKETS];
};

void sdram_initialize(int controllers, const struct mem_controller *ctrl);


#endif /* NORTHBRIDGE_INTEL_I855_RAMINIT_H */
