#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include "ich5r.h"

static void sata_init(struct device *dev)
{

	uint16_t word;

  printk_debug("SATA init\n");
	/* SATA configuration */
	pci_write_config8(dev, 0x04, 0x07);
	pci_write_config8(dev, 0x09, 0x8f);
	
	/* Set timmings */
	pci_write_config16(dev, 0x40, 0x0a307);
	pci_write_config16(dev, 0x42, 0x0a307);

	/* Sync DMA */
	pci_write_config16(dev, 0x48, 0x000f);
	pci_write_config16(dev, 0x4a, 0x1111);

	/* 66 mhz */
	pci_write_config16(dev, 0x54, 0xf00f);

	/* Combine ide - sata configuration */
	pci_write_config8(dev, 0x90, 0x0);
	
	/* port 0 & 1 enable */
	pci_write_config8(dev, 0x92, 0x33);
	
	/* initialize SATA  */
	pci_write_config16(dev, 0xa0, 0x0018);
	pci_write_config32(dev, 0xa4, 0x00000264);
	pci_write_config16(dev, 0xa0, 0x0040);
	pci_write_config32(dev, 0xa4, 0x00220043);

}

static struct device_operations sata_ops  = {
	.read_resources   = pci_dev_read_resources,
	.set_resources    = pci_dev_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init             = sata_init,
	.scan_bus         = 0,
	.ops_pci          = 0,
};

static struct pci_driver sata_driver __pci_driver = {
	.ops    = &sata_ops,
	.vendor = PCI_VENDOR_ID_INTEL,
	.device = PCI_DEVICE_ID_INTEL_82801ER_1F2_R,
};

static struct pci_driver sata_driver_nr __pci_driver = {
	.ops    = &sata_ops,
	.vendor = PCI_VENDOR_ID_INTEL,
	.device = PCI_DEVICE_ID_INTEL_82801ER_1F2,
};

