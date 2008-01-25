/*
 * This file is part of the coreboot project.
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

#include <console/console.h>
#include <arch/io.h>
#include <stdint.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/hypertransport.h>
#include <stdlib.h>
#include <string.h>
#include <bitops.h>
#include <cpu/cpu.h>

#include <cpu/x86/lapic.h>

#if CONFIG_LOGICAL_CPUS==1
#include <cpu/amd/quadcore.h>
#include <pc80/mc146818rtc.h>
#endif

#include "chip.h"
#include "root_complex/chip.h"
#include "northbridge.h"

#include "amdfam10.h"

#if HW_MEM_HOLE_SIZEK != 0
#include <cpu/amd/model_10xxx_rev.h>
#endif

#include <cpu/amd/amdfam10_sysconf.h>

struct amdfam10_sysconf_t sysconf;

#define FX_DEVS NODE_NUMS
static device_t __f0_dev[FX_DEVS];
static device_t __f1_dev[FX_DEVS];
static device_t __f2_dev[FX_DEVS];
static device_t __f4_dev[FX_DEVS];

device_t get_node_pci(u32 nodeid, u32 fn)
{
#if NODE_NUMS == 64
	if(nodeid<32) {
		return dev_find_slot(CBB, PCI_DEVFN(CDB + nodeid, fn));
	} else {
		return dev_find_slot(CBB-1, PCI_DEVFN(CDB + nodeid - 32, fn));
	}

#else
	return dev_find_slot(CBB, PCI_DEVFN(CDB + nodeid, fn));
#endif

}
static void get_fx_devs(void)
{
	int i;
	if (__f1_dev[0]) {
		return;
	}
	for(i = 0; i < FX_DEVS; i++) {
		__f0_dev[i] = get_node_pci(i, 0);
		__f1_dev[i] = get_node_pci(i, 1);
		__f2_dev[i] = get_node_pci(i, 2);
		__f4_dev[i] = get_node_pci(i, 4);
	}
	if (!__f1_dev[0]) {
		printk_err("Cannot find %02x:%02x.1", CBB, CDB);
		die("Cannot go on\n");
	}
}

static u32 f1_read_config32(u32 reg)
{
	get_fx_devs();
	return pci_read_config32(__f1_dev[0], reg);
}

static void f1_write_config32(u32 reg, u32 value)
{
	int i;
	get_fx_devs();
	for(i = 0; i < FX_DEVS; i++) {
		device_t dev;
		dev = __f1_dev[i];
		if (dev && dev->enabled) {
			pci_write_config32(dev, reg, value);
		}
	}
}


static u32 amdfam10_nodeid(device_t dev)
{
#if NODE_NUMS == 64
	unsigned busn;
	busn = dev->bus->secondary;
	if(busn != CBB) {
		return (dev->path.u.pci.devfn >> 3) - CDB + 32;
	} else {
		return (dev->path.u.pci.devfn >> 3) - CDB;
	}

#else
	return (dev->path.u.pci.devfn >> 3) - CDB;
#endif
}

#include "amdfam10_conf.c"

static void set_vga_enable_reg(u32 nodeid, u32 linkn)
{
	u32 val;

	val =  1 | (nodeid<<4) | (linkn<<12);
	/* it will routing (1)mmio  0xa0000:0xbffff (2) io 0x3b0:0x3bb,
	 0x3c0:0x3df */
	f1_write_config32(0xf4, val);

}

static u32 amdfam10_scan_chain(device_t dev, u32 nodeid, u32 link, u32 sblink,
				u32 max, u32 offset_unitid)
{
//	I want to put sb chain in bus 0 can I?


		u32 link_type;
		int i;
		u32 ht_c_index;
		u32 ht_unitid_base[4]; // here assume only 4 HT device on chain
		u32 max_bus;
		u32 min_bus;
		u32 is_sublink1 = (link>3);
		device_t devx;
		u32 busses;
		u32 segn = max>>8;
		u32 busn = max&0xff;
		u32 max_devfn;

#if HT3_SUPPORT==1
		if(is_sublink1) {
			u32 regpos;
			u32 reg;
			regpos = 0x170 + 4 * (link&3); // it is only on sublink0
			reg = pci_read_config32(dev, regpos);
			if(reg & 1) return max; // already ganged no sblink1
			devx = get_node_pci(nodeid, 4);
		} else
#endif
			devx = dev;


		dev->link[link].cap = 0x80 + ((link&3) *0x20);
		do {
			link_type = pci_read_config32(devx, dev->link[link].cap + 0x18);
		} while(link_type & ConnectionPending);
		if (!(link_type & LinkConnected)) {
			return max;
		}
		do {
			link_type = pci_read_config32(devx, dev->link[link].cap + 0x18);
		} while(!(link_type & InitComplete));
		if (!(link_type & NonCoherent)) {
			return max;
		}
		/* See if there is an available configuration space mapping
		 * register in function 1.
		 */
		ht_c_index = get_ht_c_index(nodeid, link, &sysconf);

#if EXT_CONF_SUPPORT == 0
		if(ht_c_index>=4) return max;
#endif

		/* Set up the primary, secondary and subordinate bus numbers.
		 * We have no idea how many busses are behind this bridge yet,
		 * so we set the subordinate bus number to 0xff for the moment.
		 */

#if SB_HT_CHAIN_ON_BUS0 > 0
		// first chain will on bus 0
		if((nodeid == 0) && (sblink==link)) { // actually max is 0 here
			 min_bus = max;
		}
	#if SB_HT_CHAIN_ON_BUS0 > 1
		// second chain will be on 0x40, third 0x80, forth 0xc0
		// i would refined that to  2, 3, 4 ==> 0, 0x, 40, 0x80, 0xc0
		//			    >4 will use	 more segments, We can have 16 segmment and every segment have 256 bus, For that case need the kernel support mmio pci config.
		else {
			min_bus = ((busn>>3) + 1) << 3; // one node can have 8 link and segn is the same
		}
		max = min_bus | (segn<<8);
	 #else
		//other ...
		else {
			min_bus = ++max;
		}
	 #endif
#else
		min_bus = ++max;
#endif
		max_bus = 0xfc | (segn<<8);

		dev->link[link].secondary = min_bus;
		dev->link[link].subordinate = max_bus;
		/* Read the existing primary/secondary/subordinate bus
		 * number configuration.
		 */
		busses = pci_read_config32(devx, dev->link[link].cap + 0x14);

		/* Configure the bus numbers for this bridge: the configuration
		 * transactions will not be propagates by the bridge if it is
		 * not correctly configured
		 */
		busses &= 0xffff00ff;
		busses |= ((u32)(dev->link[link].secondary) << 8);
		pci_write_config32(devx, dev->link[link].cap + 0x14, busses);


		/* set the config map space */

		set_config_map_reg(nodeid, link, ht_c_index, dev->link[link].secondary, dev->link[link].subordinate, sysconf.segbit, sysconf.nodes);

		/* Now we can scan all of the subordinate busses i.e. the
		 * chain on the hypertranport link
		 */
		for(i=0;i<4;i++) {
			ht_unitid_base[i] = 0x20;
		}

		//if ext conf is enabled, only need use 0x1f
		if (min_bus == 0)
			max_devfn = (0x17<<3) | 7;
		else
			max_devfn = (0x1f<<3) | 7;

		max = hypertransport_scan_chain(&dev->link[link], 0, max_devfn, max, ht_unitid_base, offset_unitid);


		/* We know the number of busses behind this bridge.  Set the
		 * subordinate bus number to it's real value
		 */
		if(ht_c_index>3) { // clear the extend reg
			clear_config_map_reg(nodeid, link, ht_c_index, (max+1)>>sysconf.segbit, (dev->link[link].subordinate)>>sysconf.segbit, sysconf.nodes);
		}

		dev->link[link].subordinate = max;
		set_config_map_reg(nodeid, link, ht_c_index, dev->link[link].secondary, dev->link[link].subordinate, sysconf.segbit, sysconf.nodes);
		sysconf.ht_c_num++;

		{
		// config config_reg, and ht_unitid_base to update hcdn_reg;
			u32 temp = 0;
			for(i=0;i<4;i++) {
				temp |= (ht_unitid_base[i] & 0xff) << (i*8);
			}

			sysconf.hcdn_reg[ht_c_index] = temp;

		}

		store_ht_c_conf_bus(nodeid, link, ht_c_index, dev->link[link].secondary, dev->link[link].subordinate, &sysconf);


	return max;
}

static u32 amdfam10_scan_chains(device_t dev, u32 max)
{
	u32 nodeid;
	u32 link;
	u32 sblink = sysconf.sblk;
	u32 offset_unitid = 0;

	nodeid = amdfam10_nodeid(dev);


// Put sb chain in bus 0
#if SB_HT_CHAIN_ON_BUS0 > 0
	if(nodeid==0) {
	#if ((HT_CHAIN_UNITID_BASE != 1) || (HT_CHAIN_END_UNITID_BASE != 0x20))
		offset_unitid = 1;
	#endif
		max = amdfam10_scan_chain(dev, nodeid, sblink, sblink, max, offset_unitid ); // do sb ht chain at first, in case s2885 put sb chain (8131/8111) on link2, but put 8151 on link0
	}
#endif


#if PCI_BUS_SEGN_BITS
	max = check_segn(dev, max, sysconf.nodes, &sysconf);
#endif


	for(link = 0; link < dev->links; link++) {
#if SB_HT_CHAIN_ON_BUS0 > 0
		if( (nodeid == 0) && (sblink == link) ) continue; //already done
#endif
		offset_unitid = 0;
		#if ((HT_CHAIN_UNITID_BASE != 1) || (HT_CHAIN_END_UNITID_BASE != 0x20))
			#if SB_HT_CHAIN_UNITID_OFFSET_ONLY == 1
			if((nodeid == 0) && (sblink == link))
			#endif
				offset_unitid = 1;
		#endif

		max = amdfam10_scan_chain(dev, nodeid, link, sblink, max, offset_unitid);
	}
	return max;
}


static int reg_useable(u32 reg,device_t goal_dev, u32 goal_nodeid,
			u32 goal_link)
{
	struct resource *res;
	u32 nodeid, link;
	int result;
	res = 0;
	for(nodeid = 0; !res && (nodeid < NODE_NUMS); nodeid++) {
		device_t dev;
		dev = __f0_dev[nodeid];
		for(link = 0; !res && (link < 8); link++) {
			res = probe_resource(dev, 0x1000 + reg + (link<<16)); // 8 links, 0x1000 man f1,
		}
	}
	result = 2;
	if (res) {
		result = 0;
		if (	(goal_link == (link - 1)) &&
			(goal_nodeid == (nodeid - 1)) &&
			(res->flags <= 1)) {
			result = 1;
		}
	}
	return result;
}

static struct resource *amdfam10_find_iopair(device_t dev, u32 nodeid, u32 link)
{
	struct resource *resource;
	u32 free_reg, reg;
	resource = 0;
	free_reg = 0;
	for(reg = 0xc0; reg <= 0xd8; reg += 0x8) {
		int result;
		result = reg_useable(reg, dev, nodeid, link);
		if (result == 1) {
			/* I have been allocated this one */
			break;
		}
		else if (result > 1) {
			/* I have a free register pair */
			free_reg = reg;
		}
	}
	if (reg > 0xd8) {
		reg = free_reg; // if no free, the free_reg still be 0
	}

	//Ext conf space
	if(!reg) {
		//because of Extend conf space, we will never run out of reg, but we need one index to differ them. so same node and same link can have multi range
		u32 index = get_io_addr_index(nodeid, link);
		reg = 0x110+ (index<<24) + (4<<20); // index could be 0, 255
	}

	resource = new_resource(dev, 0x1000 + reg + (link<<16));

	return resource;
}

static struct resource *amdfam10_find_mempair(device_t dev, u32 nodeid, u32 link)
{
	struct resource *resource;
	u32 free_reg, reg;
	resource = 0;
	free_reg = 0;
	for(reg = 0x80; reg <= 0xb8; reg += 0x8) {
		int result;
		result = reg_useable(reg, dev, nodeid, link);
		if (result == 1) {
			/* I have been allocated this one */
			break;
		}
		else if (result > 1) {
			/* I have a free register pair */
			free_reg = reg;
		}
	}
	if (reg > 0xb8) {
		reg = free_reg;
	}

	//Ext conf space
	if(!reg) {
		//because of Extend conf space, we will never run out of reg,
		// but we need one index to differ them. so same node and
		// same link can have multi range
		u32 index = get_mmio_addr_index(nodeid, link);
		reg = 0x110+ (index<<24) + (6<<20); // index could be 0, 63

	}
	resource = new_resource(dev, 0x1000 + reg + (link<<16));
	return resource;
}


static void amdfam10_link_read_bases(device_t dev, u32 nodeid, u32 link)
{
	struct resource *resource;

	/* Initialize the io space constraints on the current bus */
	resource =  amdfam10_find_iopair(dev, nodeid, link);
	if (resource) {
		u32 align;
#if EXT_CONF_SUPPORT == 1
		if((resource->index & 0x1fff) == 0x1110) { // ext
			align = 8;
		}
		else
#endif
			align = log2(HT_IO_HOST_ALIGN);
		resource->base	= 0;
		resource->size	= 0;
		resource->align = align;
		resource->gran	= align;
		resource->limit = 0xffffUL;
		resource->flags = IORESOURCE_IO;
		compute_allocate_resource(&dev->link[link], resource,
					IORESOURCE_IO, IORESOURCE_IO);
	}

	/* Initialize the prefetchable memory constraints on the current bus */
	resource = amdfam10_find_mempair(dev, nodeid, link);
	if (resource) {
		resource->base	= 0;
		resource->size	= 0;
		resource->align = log2(HT_MEM_HOST_ALIGN);
		resource->gran	= log2(HT_MEM_HOST_ALIGN);
		resource->limit = 0xffffffffffULL;
		resource->flags = IORESOURCE_MEM | IORESOURCE_PREFETCH;
		compute_allocate_resource(&dev->link[link], resource,
			IORESOURCE_MEM | IORESOURCE_PREFETCH,
			IORESOURCE_MEM | IORESOURCE_PREFETCH);

#if EXT_CONF_SUPPORT == 1
		if((resource->index & 0x1fff) == 0x1110) { // ext
			normalize_resource(resource);
		}
#endif

	}

	/* Initialize the memory constraints on the current bus */
	resource = amdfam10_find_mempair(dev, nodeid, link);
	if (resource) {
		resource->base	= 0;
		resource->size	= 0;
		resource->align = log2(HT_MEM_HOST_ALIGN);
		resource->gran	= log2(HT_MEM_HOST_ALIGN);
		resource->limit = 0xffffffffffULL;
		resource->flags = IORESOURCE_MEM;
		compute_allocate_resource(&dev->link[link], resource,
			IORESOURCE_MEM | IORESOURCE_PREFETCH,
			IORESOURCE_MEM);

#if EXT_CONF_SUPPORT == 1
		if((resource->index & 0x1fff) == 0x1110) { // ext
			normalize_resource(resource);
		}
#endif

	}
}


static void amdfam10_read_resources(device_t dev)
{
	u32 nodeid, link;

	nodeid = amdfam10_nodeid(dev);
	for(link = 0; link < dev->links; link++) {
		if (dev->link[link].children) {
			amdfam10_link_read_bases(dev, nodeid, link);
		}
	}
}


static void amdfam10_set_resource(device_t dev, struct resource *resource,
				u32 nodeid)
{
	resource_t rbase, rend;
	unsigned reg, link;
	char buf[50];

	/* Make certain the resource has actually been set */
	if (!(resource->flags & IORESOURCE_ASSIGNED)) {
		return;
	}

	/* If I have already stored this resource don't worry about it */
	if (resource->flags & IORESOURCE_STORED) {
		return;
	}

	/* Only handle PCI memory and IO resources */
	if (!(resource->flags & (IORESOURCE_MEM | IORESOURCE_IO)))
		return;

	/* Ensure I am actually looking at a resource of function 1 */
	if ((resource->index & 0xffff) < 0x1000) {
		return;
	}
	/* Get the base address */
	rbase = resource->base;

	/* Get the limit (rounded up) */
	rend  = resource_end(resource);

	/* Get the register and link */
	reg  = resource->index & 0xfff; // 4k
	link = ( resource->index>> 16)& 0x7; // 8 links

	if (resource->flags & IORESOURCE_IO) {
		compute_allocate_resource(&dev->link[link], resource,
			IORESOURCE_IO, IORESOURCE_IO);

		set_io_addr_reg(dev, nodeid, link, reg, rbase>>8, rend>>8);
		store_conf_io_addr(nodeid, link, reg, (resource->index >> 24), rbase>>8, rend>>8);
	}
	else if (resource->flags & IORESOURCE_MEM) {
		compute_allocate_resource(&dev->link[link], resource,
			IORESOURCE_MEM | IORESOURCE_PREFETCH,
			resource->flags & (IORESOURCE_MEM | IORESOURCE_PREFETCH));
		set_mmio_addr_reg(nodeid, link, reg, (resource->index >>24), rbase>>8, rend>>8, sysconf.nodes) ;// [39:8]
		store_conf_mmio_addr(nodeid, link, reg, (resource->index >>24), rbase>>8, rend>>8);
	}
	resource->flags |= IORESOURCE_STORED;
	sprintf(buf, " <node %02x link %02x>",
		nodeid, link);
	report_resource_stored(dev, resource, buf);
}

/**
 *
 * I tried to reuse the resource allocation code in amdfam10_set_resource()
 * but it is too diffcult to deal with the resource allocation magic.
 */
#if CONFIG_CONSOLE_VGA_MULTI == 1
extern device_t vga_pri;	// the primary vga device, defined in device.c
#endif

static void amdfam10_create_vga_resource(device_t dev, unsigned nodeid)
{
	unsigned link;

	/* find out which link the VGA card is connected,
	 * we only deal with the 'first' vga card */
	for (link = 0; link < dev->links; link++) {
		if (dev->link[link].bridge_ctrl & PCI_BRIDGE_CTL_VGA) {
#if CONFIG_CONSOLE_VGA_MULTI == 1
			printk_debug("VGA: vga_pri bus num = %d dev->link[link] bus range [%d,%d]\n", vga_pri->bus->secondary,
				dev->link[link].secondary,dev->link[link].subordinate);
			/* We need to make sure the vga_pri is under the link */
			if((vga_pri->bus->secondary >= dev->link[link].secondary ) &&
				(vga_pri->bus->secondary <= dev->link[link].subordinate )
			)
#endif
			break;
		}
	}

	/* no VGA card installed */
	if (link == dev->links)
		return;

	printk_debug("VGA: %s (aka node %d) link %d has VGA device\n", dev_path(dev), nodeid, link);
	set_vga_enable_reg(nodeid, link);
}

static void amdfam10_set_resources(device_t dev)
{
	u32 nodeid, link;
	int i;

	/* Find the nodeid */
	nodeid = amdfam10_nodeid(dev);

	amdfam10_create_vga_resource(dev, nodeid);

	/* Set each resource we have found */
	for(i = 0; i < dev->resources; i++) {
		amdfam10_set_resource(dev, &dev->resource[i], nodeid);
	}

	for(link = 0; link < dev->links; link++) {
		struct bus *bus;
		bus = &dev->link[link];
		if (bus->children) {
			assign_resources(bus);
		}
	}
}


static void amdfam10_enable_resources(device_t dev)
{
	pci_dev_enable_resources(dev);
	enable_childrens_resources(dev);
}

static void mcf0_control_init(struct device *dev)
{
}

static struct device_operations northbridge_operations = {
	.read_resources	  = amdfam10_read_resources,
	.set_resources	  = amdfam10_set_resources,
	.enable_resources = amdfam10_enable_resources,
	.init		  = mcf0_control_init,
	.scan_bus	  = amdfam10_scan_chains,
	.enable		  = 0,
	.ops_pci	  = 0,
};


static struct pci_driver mcf0_driver __pci_driver = {
	.ops	= &northbridge_operations,
	.vendor = PCI_VENDOR_ID_AMD,
	.device = 0x1200,
};

#if CONFIG_CHIP_NAME == 1

struct chip_operations northbridge_amd_amdfam10_ops = {
	CHIP_NAME("AMD FAM10 Northbridge")
	.enable_dev = 0,
};

#endif

static void pci_domain_read_resources(device_t dev)
{
	struct resource *resource;
	unsigned reg;
	unsigned link;

	/* Find the already assigned resource pairs */
	get_fx_devs();
	for(reg = 0x80; reg <= 0xd8; reg+= 0x08) {
		u32 base, limit;
		base  = f1_read_config32(reg);
		limit = f1_read_config32(reg + 0x04);
		/* Is this register allocated? */
		if ((base & 3) != 0) {
			unsigned nodeid, link;
			device_t dev;
			if(reg<0xc0) { // mmio
				nodeid = (limit & 0xf) + (base&0x30);
			} else { // io
				nodeid =  (limit & 0xf) + ((base>>4)&0x30);
			}
			link   = (limit >> 4) & 7;
			dev = __f0_dev[nodeid];
			if (dev) {
				/* Reserve the resource	 */
				struct resource *resource;
				resource = new_resource(dev, 0x1000 + reg + (link<<16));
				if (resource) {
					resource->flags = 1;
				}
			}
		}
	}
	/* FIXME: do we need to check extend conf space?
	   I don't believe that much preset value */

#if CONFIG_PCI_64BIT_PREF_MEM == 0
	/* Initialize the system wide io space constraints */
	resource = new_resource(dev, IOINDEX_SUBTRACTIVE(0, 0));
	resource->base	= 0x400;
	resource->limit = 0xffffUL;
	resource->flags = IORESOURCE_IO | IORESOURCE_SUBTRACTIVE | IORESOURCE_ASSIGNED;

	/* Initialize the system wide memory resources constraints */
	resource = new_resource(dev, IOINDEX_SUBTRACTIVE(1, 0));
	resource->limit = 0xfcffffffffULL;
	resource->flags = IORESOURCE_MEM | IORESOURCE_SUBTRACTIVE | IORESOURCE_ASSIGNED;
#else
	for(link=0; link<dev->links; link++) {
		/* Initialize the system wide io space constraints */
		resource = new_resource(dev, 0|(link<<2));
		resource->base	= 0x400;
		resource->limit = 0xffffUL;
		resource->flags = IORESOURCE_IO;
		compute_allocate_resource(&dev->link[link], resource,
			IORESOURCE_IO, IORESOURCE_IO);

		/* Initialize the system wide prefetchable memory resources constraints */
		resource = new_resource(dev, 1|(link<<2));
		resource->limit = 0xfcffffffffULL;
		resource->flags = IORESOURCE_MEM | IORESOURCE_PREFETCH;
		compute_allocate_resource(&dev->link[link], resource,
			IORESOURCE_MEM | IORESOURCE_PREFETCH,
			IORESOURCE_MEM | IORESOURCE_PREFETCH);

		/* Initialize the system wide memory resources constraints */
		resource = new_resource(dev, 2|(link<<2));
		resource->limit = 0xfcffffffffULL;
		resource->flags = IORESOURCE_MEM;
		compute_allocate_resource(&dev->link[link], resource,
			IORESOURCE_MEM | IORESOURCE_PREFETCH,
			IORESOURCE_MEM);
	}
#endif
}

static void ram_resource(device_t dev, unsigned long index,
	resource_t basek, resource_t sizek)
{
	struct resource *resource;

	if (!sizek) {
		return;
	}
	resource = new_resource(dev, index);
	resource->base	= basek << 10;
	resource->size	= sizek << 10;
	resource->flags =  IORESOURCE_MEM | IORESOURCE_CACHEABLE | \
		IORESOURCE_FIXED | IORESOURCE_STORED | IORESOURCE_ASSIGNED;
}

static void tolm_test(void *gp, struct device *dev, struct resource *new)
{
	struct resource **best_p = gp;
	struct resource *best;
	best = *best_p;
	if (!best || (best->base > new->base)) {
		best = new;
	}
	*best_p = best;
}

static u32 find_pci_tolm(struct bus *bus, u32 tolm)
{
	struct resource *min;
	min = 0;
	search_bus_resources(bus, IORESOURCE_MEM, IORESOURCE_MEM, tolm_test, &min);
	if (min && tolm > min->base) {
		tolm = min->base;
	}
	return tolm;
}

#if CONFIG_PCI_64BIT_PREF_MEM == 1
#define BRIDGE_IO_MASK (IORESOURCE_IO | IORESOURCE_MEM | IORESOURCE_PREFETCH)
#endif

#if HW_MEM_HOLE_SIZEK != 0

struct hw_mem_hole_info {
	unsigned hole_startk;
	int node_id;
};

static struct hw_mem_hole_info get_hw_mem_hole_info(void)
{
		struct hw_mem_hole_info mem_hole;
		int i;

		mem_hole.hole_startk = HW_MEM_HOLE_SIZEK;
		mem_hole.node_id = -1;

		for (i = 0; i < sysconf.nodes; i++) {
			struct dram_base_mask_t d;
			u32 hole;
			d = get_dram_base_mask(i);
			if(!(d.mask & 1)) continue; // no memory on this node

			hole = pci_read_config32(__f1_dev[i], 0xf0);
			if(hole & 1) { // we find the hole
				mem_hole.hole_startk = (hole & (0xff<<24)) >> 10;
				mem_hole.node_id = i; // record the node No with hole
				break; // only one hole
			}
		}

		//We need to double check if there is speical set on base reg and limit reg are not continous instead of hole, it will find out it's hole_startk
		if(mem_hole.node_id==-1) {
			resource_t limitk_pri = 0;
			for(i=0; i<sysconf.nodes; i++) {
				struct dram_base_mask_t d;
				resource_t base_k, limit_k;
				d = get_dram_base_mask(i);
				if(!(d.base & 1)) continue;

				base_k = ((resource_t)(d.base & 0x1fffff00)) <<9;
				if(base_k > 4 *1024 * 1024) break; // don't need to go to check
				if(limitk_pri != base_k) { // we find the hole
					mem_hole.hole_startk = (unsigned)limitk_pri; // must beblow 4G
					mem_hole.node_id = i;
					break; //only one hole
				}

				limit_k = ((resource_t)((d.mask + 0x00000100) & 0x1fffff00)) << 9;
				limitk_pri = limit_k;
			}
		}
		return mem_hole;
}


#if CONFIG_AMDMCT == 0
static void disable_hoist_memory(unsigned long hole_startk, int i)
{
	int ii;
	device_t dev;
	struct dram_base_mask_t d;
	u32 sel_m;
	u32 sel_hi_en;
	u32 hoist;
	u32 hole_sizek;

	u32 one_DCT;
	struct sys_info *sysinfox = (struct sys_info *)((CONFIG_LB_MEM_TOPK<<10) - DCACHE_RAM_GLOBAL_VAR_SIZE); // in RAM
	struct mem_info *meminfo;
	meminfo = &sysinfox->meminfo[i];

	one_DCT = get_one_DCT(meminfo);

	// 1. find which node has hole
	// 2. change limit in that node.
	// 3. change base and limit in later node
	// 4. clear that node f0

	// if there is not mem hole enabled, we need to change it's base instead

	hole_sizek = (4*1024*1024) - hole_startk;

	for(ii=NODE_NUMS-1;ii>i;ii--) {

		d = get_dram_base_mask(ii);

		if(!(d.mask & 1)) continue;

		d.base -= (hole_sizek>>9);
		d.mask -= (hole_sizek>>9);
		set_dram_base_mask(ii, d, sysconf.nodes);

		if(get_DctSelHiEn(ii) & 1) {
			sel_m = get_DctSelBaseAddr(ii);
			sel_m -= hole_startk>>10;
			set_DctSelBaseAddr(ii, sel_m);
		}
	}

	d = get_dram_base_mask(i);
	dev = __f1_dev[i];
	hoist = pci_read_config32(dev, 0xf0);
	sel_hi_en = get_DctSelHiEn(i);

	if(sel_hi_en & 1) {
		sel_m = get_DctSelBaseAddr(i);
	}

	if(hoist & 1) {
		pci_write_config32(dev, 0xf0, 0);
		d.mask -= (hole_sizek>>9);
		set_dram_base_mask(i, d, sysconf.nodes);
		if(one_DCT || (sel_m >= (hole_startk>>10))) {
			if(sel_hi_en & 1) {
				sel_m -= hole_startk>>10;
				set_DctSelBaseAddr(i, sel_m);
			}
		}
		if(sel_hi_en & 1) {
			set_DctSelBaseOffset(i, 0);
		}
	}
	else {
		d.base -= (hole_sizek>>9);
		d.mask -= (hole_sizek>>9);
		set_dram_base_mask(i, d, sysconf.nodes);

		if(sel_hi_en & 1) {
			sel_m -= hole_startk>>10;
			set_DctSelBaseAddr(i, sel_m);
		}
	}

}
#endif

#endif

static void pci_domain_set_resources(device_t dev)
{
#if CONFIG_PCI_64BIT_PREF_MEM == 1
	struct resource *io, *mem1, *mem2;
	struct resource *resource, *last;
#endif
	unsigned long mmio_basek;
	u32 pci_tolm;
	int i, idx;
	u32 link;
#if HW_MEM_HOLE_SIZEK != 0
	struct hw_mem_hole_info mem_hole;
	u32 reset_memhole = 1;
#endif

#if CONFIG_PCI_64BIT_PREF_MEM == 1

	for(link=0; link<dev->links; link++) {
		/* Now reallocate the pci resources memory with the
		 * highest addresses I can manage.
		 */
		mem1 = find_resource(dev, 1|(link<<2));
		mem2 = find_resource(dev, 2|(link<<2));

		printk_debug("base1: 0x%08Lx limit1: 0x%08Lx size: 0x%08Lx align: %d\n",
			mem1->base, mem1->limit, mem1->size, mem1->align);
		printk_debug("base2: 0x%08Lx limit2: 0x%08Lx size: 0x%08Lx align: %d\n",
			mem2->base, mem2->limit, mem2->size, mem2->align);

		/* See if both resources have roughly the same limits */
		if (((mem1->limit <= 0xffffffff) && (mem2->limit <= 0xffffffff)) ||
			((mem1->limit > 0xffffffff) && (mem2->limit > 0xffffffff)))
		{
			/* If so place the one with the most stringent alignment first
			 */
			if (mem2->align > mem1->align) {
				struct resource *tmp;
				tmp = mem1;
				mem1 = mem2;
				mem2 = tmp;
			}
			/* Now place the memory as high up as it will go */
			mem2->base = resource_max(mem2);
			mem1->limit = mem2->base - 1;
			mem1->base = resource_max(mem1);
		}
		else {
			/* Place the resources as high up as they will go */
			mem2->base = resource_max(mem2);
			mem1->base = resource_max(mem1);
		}

		printk_debug("base1: 0x%08Lx limit1: 0x%08Lx size: 0x%08Lx align: %d\n",
			mem1->base, mem1->limit, mem1->size, mem1->align);
		printk_debug("base2: 0x%08Lx limit2: 0x%08Lx size: 0x%08Lx align: %d\n",
			mem2->base, mem2->limit, mem2->size, mem2->align);
	}

	last = &dev->resource[dev->resources];
	for(resource = &dev->resource[0]; resource < last; resource++)
	{
		resource->flags |= IORESOURCE_ASSIGNED;
		resource->flags &= ~IORESOURCE_STORED;
		link = (resource>>2) & 3;
		compute_allocate_resource(&dev->link[link], resource,
			BRIDGE_IO_MASK, resource->flags & BRIDGE_IO_MASK);

		resource->flags |= IORESOURCE_STORED;
		report_resource_stored(dev, resource, "");

	}
#endif

	pci_tolm = 0xffffffffUL;
	for(link=0;link<dev->links; link++) {
		pci_tolm = find_pci_tolm(&dev->link[link], pci_tolm);
	}

#warning "FIXME handle interleaved nodes"
	mmio_basek = pci_tolm >> 10;
	/* Round mmio_basek to something the processor can support */
	mmio_basek &= ~((1 << 6) -1);

#warning "FIXME improve mtrr.c so we don't use up all of the mtrrs with a 64M MMIO hole"
	/* Round the mmio hold to 64M */
	mmio_basek &= ~((64*1024) - 1);

#if HW_MEM_HOLE_SIZEK != 0
/* if the hw mem hole is already set in raminit stage, here we will compare
 * mmio_basek and hole_basek. if mmio_basek is bigger that hole_basek and will
 * use hole_basek as mmio_basek and we don't need to reset hole.
 * otherwise We reset the hole to the mmio_basek
 */

	mem_hole = get_hw_mem_hole_info();

	// Use hole_basek as mmio_basek, and we don't need to reset hole anymore
	if ((mem_hole.node_id !=  -1) && (mmio_basek > mem_hole.hole_startk)) {
		mmio_basek = mem_hole.hole_startk;
		reset_memhole = 0;
	}

	#if CONFIG_AMDMCT == 0
	//mmio_basek = 3*1024*1024; // for debug to meet boundary

	if(reset_memhole) {
		if(mem_hole.node_id!=-1) {
		/* We need to select HW_MEM_HOLE_SIZEK for raminit, it can not
		    make hole_startk to some basek too!
		   We need to reset our Mem Hole, because We want more big HOLE
		    than we already set
		   Before that We need to disable mem hole at first, becase
		    memhole could already be set on i+1 instead
		 */
			disable_hoist_memory(mem_hole.hole_startk, mem_hole.node_id);
		}

	#if HW_MEM_HOLE_SIZE_AUTO_INC == 1
		// We need to double check if the mmio_basek is valid for hole
		// setting, if it is equal to basek, we need to decrease it some
		resource_t basek_pri;
		for (i = 0; i < sysconf.nodes; i++) {
			struct dram_base_mask_t d;
			resource_t basek;
			d = get_dram_base_mask(i);

			if(!(d.mask &1)) continue;

			basek = ((resource_t)(d.base & 0x1fffff00)) << 9;
			if(mmio_basek == (u32)basek) {
				mmio_basek -= (uin32_t)(basek - basek_pri); // increase mem hole size to make sure it is on middle of pri node
				break;
			}
			basek_pri = basek;
		}
	#endif
	}
	#endif


#endif

	idx = 0x10;
	for(i = 0; i < sysconf.nodes; i++) {
		struct dram_base_mask_t d;
		resource_t basek, limitk, sizek; // 4 1T
		d = get_dram_base_mask(i);

		if(!(d.mask & 1)) continue;
		basek = ((resource_t)(d.base & 0x1fffff00)) << 9; // could overflow, we may lost 6 bit here
		limitk = ((resource_t)((d.mask + 0x00000100) & 0x1fffff00)) << 9 ;
		sizek = limitk - basek;

		/* see if we need a hole from 0xa0000 to 0xbffff */
		if ((basek < ((8*64)+(8*16))) && (sizek > ((8*64)+(16*16)))) {
			ram_resource(dev, (idx | i), basek, ((8*64)+(8*16)) - basek);
			idx += 0x10;
			basek = (8*64)+(16*16);
			sizek = limitk - ((8*64)+(16*16));

		}

//		printk_debug("node %d : mmio_basek=%08x, basek=%08x, limitk=%08x\n", i, mmio_basek, basek, limitk);

		/* split the region to accomodate pci memory space */
		if ( (basek < 4*1024*1024 ) && (limitk > mmio_basek) ) {
			if (basek <= mmio_basek) {
				unsigned pre_sizek;
				pre_sizek = mmio_basek - basek;
				if(pre_sizek>0) {
					ram_resource(dev, (idx | i), basek, pre_sizek);
					idx += 0x10;
					sizek -= pre_sizek;
				}
				#if CONFIG_AMDMCT == 0
				#if HW_MEM_HOLE_SIZEK != 0
				if(reset_memhole) {
					struct sys_info *sysinfox = (struct sys_info *)((CONFIG_LB_MEM_TOPK<<10) - DCACHE_RAM_GLOBAL_VAR_SIZE); // in RAM
					struct mem_info *meminfo;
					meminfo = &sysinfox->meminfo[i];
					sizek += hoist_memory(mmio_basek,i, get_one_DCT(meminfo), sysconf.nodes);
				}
				#endif
				#endif

				basek = mmio_basek;
			}
			if ((basek + sizek) <= 4*1024*1024) {
				sizek = 0;
			}
			else {
				basek = 4*1024*1024;
				sizek -= (4*1024*1024 - mmio_basek);
			}
		}
		ram_resource(dev, (idx | i), basek, sizek);
		idx += 0x10;
	}

	for(link = 0; link < dev->links; link++) {
		struct bus *bus;
		bus = &dev->link[link];
		if (bus->children) {
			assign_resources(bus);
		}
	}
}

static u32 pci_domain_scan_bus(device_t dev, u32 max)
{
	u32 reg;
	int i;
	/* Unmap all of the HT chains */
	for(reg = 0xe0; reg <= 0xec; reg += 4) {
		f1_write_config32(reg, 0);
	}
#if EXT_CONF_SUPPORT == 1
	// all nodes
	for(i = 0; i< sysconf.nodes; i++) {
		int index;
		for(index = 0; index < 64; index++) {
			pci_write_config32(__f1_dev[i], 0x110, index | (6<<28));
			pci_write_config32(__f1_dev[i], 0x114, 0);
		}

	}
#endif


	for(i=0;i<dev->links;i++) {
		max = pci_scan_bus(&dev->link[i], PCI_DEVFN(CDB, 0), 0xff, max);
	}

	/* Tune the hypertransport transaction for best performance.
	 * Including enabling relaxed ordering if it is safe.
	 */
	get_fx_devs();
	for(i = 0; i < FX_DEVS; i++) {
		device_t f0_dev;
		f0_dev = __f0_dev[i];
		if (f0_dev && f0_dev->enabled) {
			u32 httc;
			httc = pci_read_config32(f0_dev, HT_TRANSACTION_CONTROL);
			httc &= ~HTTC_RSP_PASS_PW;
			if (!dev->link[0].disable_relaxed_ordering) {
				httc |= HTTC_RSP_PASS_PW;
			}
			printk_spew("%s passpw: %s\n",
				dev_path(dev),
				(!dev->link[0].disable_relaxed_ordering)?
				"enabled":"disabled");
			pci_write_config32(f0_dev, HT_TRANSACTION_CONTROL, httc);
		}
	}
	return max;
}

static struct device_operations pci_domain_ops = {
	.read_resources	  = pci_domain_read_resources,
	.set_resources	  = pci_domain_set_resources,
	.enable_resources = enable_childrens_resources,
	.init		  = 0,
	.scan_bus	  = pci_domain_scan_bus,
#if MMCONF_SUPPORT
	.ops_pci_bus	  = &pci_ops_mmconf,
#else
	.ops_pci_bus	  = &pci_cf8_conf1,
#endif
};

static void sysconf_init(device_t dev) // first node
{
	sysconf.sblk = (pci_read_config32(dev, 0x64)>>8) & 7; // don't forget sublink1
	sysconf.segbit = 0;
	sysconf.ht_c_num = 0;

	unsigned ht_c_index;

	for(ht_c_index=0; ht_c_index<32; ht_c_index++) {
		sysconf.ht_c_conf_bus[ht_c_index] = 0;
	}

	sysconf.nodes = ((pci_read_config32(dev, 0x60)>>4) & 7) + 1;
#if CONFIG_MAX_PHYSICAL_CPUS > 8
	sysconf.nodes += (((pci_read_config32(dev, 0x160)>>4) & 7)<<3);
#endif

	sysconf.enabled_apic_ext_id = 0;
	sysconf.lift_bsp_apicid = 0;

	/* Find the bootstrap processors apicid */
	sysconf.bsp_apicid = lapicid();
	sysconf.apicid_offset = sysconf.bsp_apicid;

#if (ENABLE_APIC_EXT_ID == 1)
	if (pci_read_config32(dev, 0x68) & (HTTC_APIC_EXT_ID|HTTC_APIC_EXT_BRD_CST))
	{
		sysconf.enabled_apic_ext_id = 1;
	}
	#if (APIC_ID_OFFSET>0)
	if(sysconf.enabled_apic_ext_id) {
		if(sysconf.bsp_apicid == 0) {
			/* bsp apic id is not changed */
			sysconf.apicid_offset = APIC_ID_OFFSET;
		} else {
			sysconf.lift_bsp_apicid = 1;
		}

	}
	#endif
#endif

}

static u32 cpu_bus_scan(device_t dev, u32 max)
{
	struct bus *cpu_bus;
	device_t dev_mc;
	device_t pci_domain;
	int i,j;
	int nodes;
	unsigned nb_cfg_54;
	unsigned siblings;
	int cores_found;
	int disable_siblings;
	unsigned ApicIdCoreIdSize;

	nb_cfg_54 = 0;
	ApicIdCoreIdSize = (cpuid_ecx(0x80000008)>>12 & 0xf);
	if(ApicIdCoreIdSize) {
		siblings = (1<<ApicIdCoreIdSize)-1;
	} else {
		siblings = 3; //quad core
	}

	disable_siblings = !CONFIG_LOGICAL_CPUS;
#if CONFIG_LOGICAL_CPUS == 1
	get_option(&disable_siblings, "quad_core");
#endif

	// for pre_e0, nb_cfg_54 can not be set, ( even set, when you read it
	//    still be 0)
	// How can I get the nb_cfg_54 of every node' nb_cfg_54 in bsp???
	//    and differ d0 and e0 single core

	nb_cfg_54 = read_nb_cfg_54();

#if CBB
	dev_mc = dev_find_slot(0, PCI_DEVFN(CDB, 0)); //0x00
	if(dev_mc && dev_mc->bus) {
		printk_debug("%s found", dev_path(dev_mc));
		pci_domain = dev_mc->bus->dev;
		if(pci_domain && (pci_domain->path.type == DEVICE_PATH_PCI_DOMAIN)) {
			printk_debug("\n%s move to ",dev_path(dev_mc));
			dev_mc->bus->secondary = CBB; // move to 0xff
			printk_debug("%s",dev_path(dev_mc));

		} else {
			printk_debug(" but it is not under pci_domain directly ");
		}
		printk_debug("\n");

	}
	dev_mc = dev_find_slot(CBB, PCI_DEVFN(CDB, 0));
	if(!dev_mc) {
		dev_mc = dev_find_slot(0, PCI_DEVFN(0x18, 0));
		if (dev_mc && dev_mc->bus) {
			printk_debug("%s found\n", dev_path(dev_mc));
			pci_domain = dev_mc->bus->dev;
			if(pci_domain && (pci_domain->path.type == DEVICE_PATH_PCI_DOMAIN)) {
				if((pci_domain->links==1) && (pci_domain->link[0].children == dev_mc)) {
					printk_debug("%s move to ",dev_path(dev_mc));
					dev_mc->bus->secondary = CBB; // move to 0xff
					printk_debug("%s\n",dev_path(dev_mc));
					while(dev_mc){
						printk_debug("%s move to ",dev_path(dev_mc));
						dev_mc->path.u.pci.devfn -= PCI_DEVFN(0x18,0);
						printk_debug("%s\n",dev_path(dev_mc));
						dev_mc = dev_mc->sibling;
					}
				}
			}
		}
	}

#endif

	dev_mc = dev_find_slot(CBB, PCI_DEVFN(CDB, 0));
	if (!dev_mc) {
		printk_err("%02x:%02x.0 not found", CBB, CDB);
		die("");
	}

	sysconf_init(dev_mc);

	nodes = sysconf.nodes;

#if CBB && (NODE_NUMS > 32)
	if(nodes>32) { // need to put node 32 to node 63 to bus 0xfe
		if(pci_domain->links==1) {
			pci_domain->links++; // from 1 to 2
			pci_domain->link[1].link = 1;
			pci_domain->link[1].dev = pci_domain;
			pci_domain->link[1].children = 0;
			printk_debug("%s links increase to %d\n", dev_path(pci_domain), pci_domain->links);
		}
		pci_domain->link[1].secondary = CBB - 1;
	}
#endif
	/* Find which cpus are present */
	cpu_bus = &dev->link[0];
	for(i = 0; i < nodes; i++) {
		device_t dev, cpu;
		struct device_path cpu_path;
		unsigned busn, devn;
		struct bus *pbus;

		busn = CBB;
		devn = CDB+i;
		pbus = dev_mc->bus;
#if CBB && (NODE_NUMS > 32)
		if(i>=32) {
			busn--;
			devn-=32;
			pbus = &(pci_domain->link[1]);
		}
#endif

		/* Find the cpu's pci device */
		dev = dev_find_slot(busn, PCI_DEVFN(devn, 0));
		if (!dev) {
			/* If I am probing things in a weird order
			 * ensure all of the cpu's pci devices are found.
			 */
			int j;
			for(j = 0; j <= 5; j++) { //FBDIMM?
				dev = pci_probe_dev(NULL, pbus,
					PCI_DEVFN(devn, j));
			}
			dev = dev_find_slot(busn, PCI_DEVFN(devn,0));
		}
		if(dev) {
			/* Ok, We need to set the links for that device.
			 * otherwise the device under it will not be scanned
			 */
			int j;
			int linknum;
#if HT3_SUPPORT==1
			linknum = 8;
#else
			linknum = 4;
#endif
			if(dev->links < linknum) {
				for(j=dev->links; j<linknum; j++) {
					 dev->link[j].link = j;
					 dev->link[j].dev = dev;
				}
				dev->links = linknum;
				printk_debug("%s links increase to %d\n", dev_path(dev), dev->links);
			}
		}

		cores_found = 0; // one core
		dev = dev_find_slot(busn, PCI_DEVFN(devn, 3));
		if (dev && dev->enabled) {
			j = pci_read_config32(dev, 0xe8);
			cores_found = (j >> 12) & 3; // dev is func 3
			printk_debug("  %s siblings=%d\n", dev_path(dev), cores_found);
		}

		u32 jj;
		if(disable_siblings) {
			jj = 0;
		} else
		{
			jj = cores_found;
		}

		for (j = 0; j <=jj; j++ ) {

			/* Build the cpu device path */
			cpu_path.type = DEVICE_PATH_APIC;
			cpu_path.u.apic.apic_id = i * (nb_cfg_54?(siblings+1):1) + j * (nb_cfg_54?1:64); // ?

			/* See if I can find the cpu */
			cpu = find_dev_path(cpu_bus, &cpu_path);

			/* Enable the cpu if I have the processor */
			if (dev && dev->enabled) {
				if (!cpu) {
					cpu = alloc_dev(cpu_bus, &cpu_path);
				}
				if (cpu) {
					cpu->enabled = 1;
				}
			}

			/* Disable the cpu if I don't have the processor */
			if (cpu && (!dev || !dev->enabled)) {
				cpu->enabled = 0;
			}

			/* Report what I have done */
			if (cpu) {
				cpu->path.u.apic.node_id = i;
				cpu->path.u.apic.core_id = j;
	#if (ENABLE_APIC_EXT_ID == 1) && (APIC_ID_OFFSET>0)
				 if(sysconf.enabled_apic_ext_id) {
					if(sysconf.lift_bsp_apicid) {
						cpu->path.u.apic.apic_id += sysconf.apicid_offset;
					} else
					{
						if (cpu->path.u.apic.apic_id != 0)
							cpu->path.u.apic.apic_id += sysconf.apicid_offset;
					 }
				}
	#endif
				printk_debug("CPU: %s %s\n",
					dev_path(cpu), cpu->enabled?"enabled":"disabled");
			}

		} //j
	}
	return max;
}


static void cpu_bus_init(device_t dev)
{
	initialize_cpus(&dev->link[0]);
}


static void cpu_bus_noop(device_t dev)
{
}


static struct device_operations cpu_bus_ops = {
	.read_resources	  = cpu_bus_noop,
	.set_resources	  = cpu_bus_noop,
	.enable_resources = cpu_bus_noop,
	.init		  = cpu_bus_init,
	.scan_bus	  = cpu_bus_scan,
};


static void root_complex_enable_dev(struct device *dev)
{
	/* Set the operations if it is a special bus type */
	if (dev->path.type == DEVICE_PATH_PCI_DOMAIN) {
		dev->ops = &pci_domain_ops;
	}
	else if (dev->path.type == DEVICE_PATH_APIC_CLUSTER) {
		dev->ops = &cpu_bus_ops;
	}
}

struct chip_operations northbridge_amd_amdfam10_root_complex_ops = {
	CHIP_NAME("AMD FAM10 Root Complex")
	.enable_dev = root_complex_enable_dev,
};