/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007 Advanced Micro Devices, Inc.
 * Copyright (C) 2008 LiPPERT Embedded Computers GmbH
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

/* Based on romstage.c from AMD's DB800 and DBM690T mainboards. */

#define ASSEMBLY 1


#include <stdlib.h>
#include <stdint.h>
#include <spd.h>
#include <device/pci_def.h>
#include <arch/io.h>
#include <device/pnp_def.h>
#include <arch/hlt.h>
#include "pc80/serial.c"
#include "arch/i386/lib/console.c"
#include "lib/ramtest.c"
#include "cpu/x86/bist.h"
#include "cpu/x86/msr.h"
#include <cpu/amd/lxdef.h>
#include <cpu/amd/geode_post_code.h>
#include "southbridge/amd/cs5536/cs5536.h"

#define POST_CODE(x) outb(x, 0x80)

#include "southbridge/amd/cs5536/cs5536_early_smbus.c"
#include "southbridge/amd/cs5536/cs5536_early_setup.c"
#include "superio/ite/it8712f/it8712f_early_serial.c"

/* Bit0 enables Spread Spectrum, bit1 makes on-board SSD act as IDE slave. */
#define SMC_CONFIG 0x01

#define ManualConf 1		/* No automatic strapped PLL config */
#define PLLMSRhi 0x0000059C	/* Manual settings for the PLL */
#define PLLMSRlo 0x00DE6001
#define DIMM0 0xA0
#define DIMM1 0xA2

static const unsigned char spdbytes[] = {	// 4x Promos V58C2512164SA-J5I
	0xFF, 0xFF,				// only values used by Geode-LX raminit.c are set
	[SPD_MEMORY_TYPE]		= SPD_MEMORY_TYPE_SDRAM_DDR,	// (Fundamental) memory type
	[SPD_NUM_ROWS]			= 0x0D,	// Number of row address bits [13]
	[SPD_NUM_COLUMNS]		= 0x0A,	// Number of column address bits [10]
	[SPD_NUM_DIMM_BANKS]		= 1,	// Number of module rows (banks)
	0xFF, 0xFF, 0xFF,
	[SPD_MIN_CYCLE_TIME_AT_CAS_MAX]	= 0x50,	// SDRAM cycle time (highest CAS latency), RAS access time (tRAC) [5.0 ns in BCD]
	0xFF, 0xFF,
	[SPD_REFRESH]			= 0x82,	// Refresh rate/type [Self Refresh, 7.8 us]
	[SPD_PRIMARY_SDRAM_WIDTH]	= 64,	// SDRAM width (primary SDRAM) [64 bits]
	0xFF, 0xFF, 0xFF,
	[SPD_NUM_BANKS_PER_SDRAM]	= 4,	// SDRAM device attributes, number of banks on SDRAM device
	[SPD_ACCEPTABLE_CAS_LATENCIES]	= 0x1C,	// SDRAM device attributes, CAS latency [3, 2.5, 2]
	0xFF, 0xFF,
	[SPD_MODULE_ATTRIBUTES]		= 0x20,	// SDRAM module attributes [differential clk]
	[SPD_DEVICE_ATTRIBUTES_GENERAL]	= 0x40,	// SDRAM device attributes, general [Concurrent AP]
	[SPD_SDRAM_CYCLE_TIME_2ND]	= 0x60,	// SDRAM cycle time (2nd highest CAS latency) [6.0 ns in BCD]
	0xFF,
	[SPD_SDRAM_CYCLE_TIME_3RD]	= 0x75,	// SDRAM cycle time (3rd highest CAS latency) [7.5 ns in BCD]
	0xFF,
	[SPD_tRP]			= 60,	// Min. row precharge time [15 ns in units of 0.25 ns]
	[SPD_tRRD]			= 40,	// Min. row active to row active [10 ns in units of 0.25 ns]
	[SPD_tRCD]			= 60,	// Min. RAS to CAS delay [15 ns in units of 0.25 ns]
	[SPD_tRAS]			= 40,	// Min. RAS pulse width = active to precharge delay [40 ns]
	[SPD_BANK_DENSITY]		= 0x40,	// Density of each row on module [256 MB]
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	[SPD_tRFC]			= 70	// SDRAM Device Minimum Auto Refresh to Active/Auto Refresh [70 ns]
};

static inline int spd_read_byte(unsigned int device, unsigned int address)
{
	if (device != DIMM0)
		return 0xFF;	/* No DIMM1, don't even try. */

#if CONFIG_DEBUG
	if (address >= sizeof(spdbytes) || spdbytes[address] == 0xFF) {
		print_err("ERROR: spd_read_byte(DIMM0, 0x");
		print_err_hex8(address);
		print_err(") returns 0xff\r\n");
	}
#endif

	/* Fake SPD ROM value */
	return (address < sizeof(spdbytes)) ? spdbytes[address] : 0xFF;
}

/* Send config data to System Management Controller via SMB. */
static int smc_send_config(unsigned char config_data)
{
	if (smbus_check_stop_condition(SMBUS_IO_BASE))
		return 1;
	if (smbus_start_condition(SMBUS_IO_BASE))
		return 2;
	if (smbus_send_slave_address(SMBUS_IO_BASE, 0x50)) // SMC address
		return 3;
	if (smbus_send_command(SMBUS_IO_BASE, 0x28)) // set config data
		return 4;
	if (smbus_send_command(SMBUS_IO_BASE, 0x01)) // data length
		return 5;
	if (smbus_send_command(SMBUS_IO_BASE, config_data))
		return 6;
	smbus_stop_condition(SMBUS_IO_BASE);
	return 0;
}

#include "northbridge/amd/lx/raminit.h"
#include "northbridge/amd/lx/pll_reset.c"
#include "northbridge/amd/lx/raminit.c"
#include "lib/generic_sdram.c"
#include "cpu/amd/model_lx/cpureginit.c"
#include "cpu/amd/model_lx/syspreinit.c"

static void msr_init(void)
{
	msr_t msr;

	/* Setup access to the cache for under 1MB. */
	msr.hi = 0x24fffc02;
	msr.lo = 0x1000A000;	/* 0-A0000 write back */
	wrmsr(CPU_RCONF_DEFAULT, msr);

	msr.hi = 0x0;		/* Write back */
	msr.lo = 0x0;
	wrmsr(CPU_RCONF_A0_BF, msr);
	wrmsr(CPU_RCONF_C0_DF, msr);
	wrmsr(CPU_RCONF_E0_FF, msr);

	/* Setup access to the cache for under 640K. Note MC not setup yet. */
	msr.hi = 0x20000000;
	msr.lo = 0xfff80;
	wrmsr(MSR_GLIU0 + 0x20, msr);

	msr.hi = 0x20000000;
	msr.lo = 0x80fffe0;
	wrmsr(MSR_GLIU0 + 0x21, msr);

	msr.hi = 0x20000000;
	msr.lo = 0xfff80;
	wrmsr(MSR_GLIU1 + 0x20, msr);

	msr.hi = 0x20000000;
	msr.lo = 0x80fffe0;
	wrmsr(MSR_GLIU1 + 0x21, msr);
}

static const u16 sio_init_table[] = { // hi=data, lo=index
	0x0707,		// select LDN 7 (GPIO, SPI, watchdog, ...)
	0x1E2C,		// disable ATXPowerGood
	0x0423,		// don't delay POWerOK1/2
	0x9072,		// watchdog triggers POWOK, counts seconds
#if !CONFIG_USE_WATCHDOG_ON_BOOT
	0x0073, 0x0074,	// disable watchdog by setting timeout to 0
#endif
	0xBF25, 0x172A, 0xF326,	// select GPIO function for most pins
	0xFF27, 0xDF28, 0x2729,	// (GP45=SUSB, GP23,22,16,15=SPI, GP13=PWROK1)
	0x072C,		// VIN6=enabled?, FAN4/5 disabled, VIN7=internal, VIN3=internal
	0x66B8, 0x0CB9,	// enable pullups
	0x07C0,		// enable Simple-I/O for GP12-10= RS485_EN2,1, LIVE_LED
	0x07C8,		// config GP12-10 as output
	0x2DF5,		// map Hw Monitor Thermal Output to GP55
	0x08F8,		// map GP LED Blinking 1 to GP10=LIVE_LED (deactivate Simple I/O to use)
};

/* Early mainboard specific GPIO setup. */
static void mb_gpio_init(void)
{
	int i;

	/* Init Super I/O WDT, GPIOs. Done early, WDT init may trigger reset! */
	it8712f_enter_conf();
	for (i = 0; i < ARRAY_SIZE(sio_init_table); i++) {
		u16 val = sio_init_table[i];
		outb((u8)val, SIO_INDEX);
		outb(val >> 8, SIO_DATA);
	}
	it8712f_exit_conf();
}

void cache_as_ram_main(void)
{
	int err;
	POST_CODE(0x01);

	static const struct mem_controller memctrl[] = {
		{.channel0 = {(0xa << 3) | 0, (0xa << 3) | 1}}
	};

	SystemPreInit();
	msr_init();

	cs5536_early_setup();

	/*
	 * Note: Must do this AFTER the early_setup! It is counting on some
	 * early MSR setup for CS5536.
	 */
	it8712f_enable_serial(0, CONFIG_TTYS0_BASE); // Does not use its 1st parameter
	mb_gpio_init();
	uart_init();
	console_init();

	pll_reset(ManualConf);

	cpuRegInit();

	/* bit1 = on-board IDE is slave, bit0 = Spread Spectrum */
	if ((err = smc_send_config(SMC_CONFIG))) {
		print_err("ERROR ");
		print_err_char('0'+err);
		print_err(" sending config data to SMC\r\n");
	}

	sdram_initialize(1, memctrl);

	/* Check memory. */
	/* ram_check(0, 640 * 1024); */

	/* Memory is setup. Return to cache_as_ram.inc and continue to boot. */
	return;
}
