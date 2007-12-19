/*
 * This file is part of the LinuxBIOS project.
 *
 * Copyright (C) 2007 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
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

/*
 * Make sure HC_NUMS and HC_POSSIBLE_NUM setting is consistent to this file
 */

DefinitionBlock ("SSDT.aml", "SSDT", 1, "AMD-FAM10", "AMD-ACPI", 100925440)
{
	/*
	 * These objects were referenced but not defined in this table
	 */
	External (\_SB_.PCI0, DeviceObj)

	Scope (\_SB.PCI0)
	{
		Name (BUSN, Package (0x20) /* HC_NUMS */
		{
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888,
			0x99999999,
			0xaaaaaaaa,
			0xbbbbbbbb,
			0xcccccccc,
			0xdddddddd,
			0xeeeeeeee,
			0x10101010,
			0x11111111,
			0x12121212,
			0x13131313,
			0x14141414,
			0x15151515,
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888,
			0x99999999,
			0xaaaaaaaa,
			0xbbbbbbbb,
			0xcccccccc
		})
		Name (MMIO, Package (0x80) /* HC_NUMS * 4 */
		{
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888,
			0x99999999,
			0xaaaaaaaa,
			0xbbbbbbbb,
			0xcccccccc,
			0xdddddddd,
			0xeeeeeeee,
			0x11111111,
			0x22222222,
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888,
			0x99999999,
			0xaaaaaaaa,
			0xbbbbbbbb,
			0xcccccccc,
			0xdddddddd,
			0xeeeeeeee,
			0x11111111,
			0x22222222,
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888,
			0x99999999,
			0xaaaaaaaa,
			0xbbbbbbbb,
			0xcccccccc,
			0xdddddddd,
			0xeeeeeeee,
			0x11111111,
			0x22222222,
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888,
			0x99999999,
			0xaaaaaaaa,
			0xbbbbbbbb,
			0xcccccccc,
			0xdddddddd,
			0xeeeeeeee,
			0x11111111,
			0x22222222,
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888,
			0x99999999,
			0xaaaaaaaa,
			0xbbbbbbbb,
			0xcccccccc,
			0xdddddddd,
			0xeeeeeeee,
			0x11111111,
			0x22222222,
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888,
			0x99999999,
			0xaaaaaaaa,
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888,
			0x99999999,
			0xaaaaaaaa,
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888,
			0x99999999,
			0xaaaaaaaa,
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888,
			0x99999999,
			0xaaaaaaaa,
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888
		})
		Name (PCIO, Package (0x40) /* HC_NUMS * 2 */
		{
			0x77777777,
			0x88888888,
			0x99999999,
			0xaaaaaaaa,
			0xbbbbbbbb,
			0xcccccccc,
			0xdddddddd,
			0xeeeeeeee,
			0x77777777,
			0x88888888,
			0x99999999,
			0xaaaaaaaa,
			0xbbbbbbbb,
			0xcccccccc,
			0xdddddddd,
			0xeeeeeeee,
			0x77777777,
			0x88888888,
			0x99999999,
			0xaaaaaaaa,
			0xbbbbbbbb,
			0xcccccccc,
			0xdddddddd,
			0xeeeeeeee,
			0x77777777,
			0x88888888,
			0x99999999,
			0xaaaaaaaa,
			0xbbbbbbbb,
			0xcccccccc,
			0xdddddddd,
			0xeeeeeeee,
			0xaaaaaaaa,
			0xbbbbbbbb,
			0xcccccccc,
			0xdddddddd,
			0xeeeeeeee,
			0x77777777,
			0x88888888,
			0x99999999,
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888,
			0x99999999,
			0xaaaaaaaa,
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888,
			0x99999999,
			0xaaaaaaaa,
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444
		})
		Name (SBLK, 0x11)
		Name (TOM1, 0xaaaaaaaa)
		Name (SBDN, 0xbbbbbbbb)
		Name (HCLK, Package (0x20) /* HC_POSSIBLE_NUM */
		{
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888,
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888,
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888,
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888
		})
		Name (HCDN, Package (0x20) /* HC_POSSIBLE_NUM */
		{
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888,
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888,
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888,
			0x11111111,
			0x22222222,
			0x33333333,
			0x44444444,
			0x55555555,
			0x66666666,
			0x77777777,
			0x88888888
		})
		Name (CBB, 0x99)
		Name (CBST, 0x88)
		Name (CBB2, 0x77)
		Name (CBS2, 0x66)

	}
}

