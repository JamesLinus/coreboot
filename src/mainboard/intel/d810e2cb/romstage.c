/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2010 Joseph Smith <joe@settoplinux.org>
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

#include <stdint.h>
#include <stdlib.h>
#include <device/pci_def.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/romcc_io.h>
#include <arch/hlt.h>
#include <console/console.h>
#include "lib/ramtest.c"
#include "southbridge/intel/i82801bx/i82801bx.h"
#include "southbridge/intel/i82801bx/i82801bx_early_smbus.c"
#include "northbridge/intel/i82810/raminit.h"
#include "lib/debug.c"
#include "pc80/udelay_io.c"
#include "lib/delay.c"
#include "cpu/x86/bist.h"
#include "superio/smsc/smscsuperio/smscsuperio_early_serial.c"
#include "gpio.c"

#define SERIAL_DEV PNP_DEV(0x4e, SMSCSUPERIO_SP1)

static inline int spd_read_byte(unsigned int device, unsigned int address)
{
	return smbus_read_byte(device, address);
}

#include "northbridge/intel/i82810/raminit.c"
/* #include "northbridge/intel/i82810/debug.c" */

void main(unsigned long bist)
{
	/* Set southbridge and superio gpios */
	mb_gpio_init();

	smscsuperio_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
	uart_init();
	console_init();

	report_bist_failure(bist);
	enable_smbus();
	/* dump_spd_registers(); */
	sdram_set_registers();
	sdram_set_spd_registers();
	sdram_enable();
	/* ram_check(0, 640 * 1024); */
}
