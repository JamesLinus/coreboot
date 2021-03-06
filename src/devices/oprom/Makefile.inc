##
## This file is part of the coreboot project.
##
## Copyright (C) 2007-2010 coresystems GmbH
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; version 2 of the License.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
##

ramstage-$(CONFIG_PCI_OPTION_ROM_RUN_REALMODE) += x86.c
ramstage-$(CONFIG_PCI_OPTION_ROM_RUN_REALMODE) += x86_asm.S
ramstage-$(CONFIG_PCI_OPTION_ROM_RUN_REALMODE) += x86_interrupts.c

subdirs-$(CONFIG_PCI_OPTION_ROM_RUN_YABEL) += x86emu
subdirs-$(CONFIG_PCI_OPTION_ROM_RUN_YABEL) += yabel
