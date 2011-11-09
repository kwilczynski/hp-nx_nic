/*
 * Copyright (C) 2003 - 2009 NetXen, Inc.
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 * 
 * The full GNU General Public License is included in this distribution
 * in the file called LICENSE.
 * 
 * Contact Information:
 * licensing@netxen.com
 * NetXen, Inc.
 * 18922 Forge Drive
 * Cupertino, CA 95014
 */
/*
 * Copyright (C) 2003 - 2009 NetXen, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 * The full GNU General Public License is included in this distribution
 * in the file called LICENSE.
 *
 * Contact Information:
 * licensing@netxen.com
 * NetXen, Inc.
 * 18922 Forge Drive
 * Cupertino, CA 95014
 */
#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/types.h>
//#include <linux/kgdb-defs.h>
#include <linux/compiler.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/pci.h>

#include <linux/in.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/highmem.h>

#include <linux/mm.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
#include <linux/wrapper.h>
#endif
#include <asm/system.h>
#include <asm/io.h>
#include <asm/byteorder.h>
#include <asm/uaccess.h>
#include <asm/pgtable.h>

#include "kernel_compatibility.h"
#include "nx_hw.h"
#include "unm_nic_config.h"
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
#include <net/net_namespace.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13) && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
#include <linux/cdev.h>
#endif

#include "nx_xport.h"

#define DEFINE_GLOBAL_RECV_CRB
#include "nic_phan_reg.h"

#include "nic_cmn.h"

#include "unm_version.h"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION("NetXen Xport Driver");
MODULE_LICENSE("GPL");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
MODULE_VERSION(UNM_NIC_LINUX_VERSIONID);
#endif

/* Char Device Interface */
int unm_pci_major = 0;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,11) && LINUX_VERSION_CODE  >= KERNEL_VERSION(2,6,8)
static int use_msi_x = 1;
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,8)
static int use_msi_x = 0;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13) && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
dev_t unm_drv_dev;
#define MAX_DEVS 1
struct cdev *unm_drv_cdev = NULL;
static struct class *unm_drv_class;
static struct class_device *unm_drv_class_device = NULL;
#endif

char nx_driver_name[] = "nx_xport";
struct unm_adapter_s *all_adapters[128]; /* Global array of adapters */
int current_adapter_index = 0;

char unm_nic_driver_string[] = DRIVER_VERSION_STRING
  UNM_NIC_LINUX_VERSIONID
  "-" UNM_NIC_BUILD_NO " generated " UNM_NIC_TIMESTAMP;
uint8_t nx_nic_msglvl = NX_NIC_NOTICE;
char *nx_nic_kern_msgs[] = {
	KERN_EMERG,
	KERN_ALERT,
	KERN_CRIT,
	KERN_ERR,
	KERN_WARNING,
	KERN_NOTICE,
	KERN_INFO,
	KERN_DEBUG
};

//static int adapter_count = 0;

#ifndef ARCH_KMALLOC_MINALIGN
#define ARCH_KMALLOC_MINALIGN 0
#endif
#ifndef ARCH_KMALLOC_FLAGS
#define ARCH_KMALLOC_FLAGS SLAB_HWCACHE_ALIGN
#endif

#define UNM_DB_MAPSIZE_BYTES    0x1000

/* Local functions to UNM NIC driver */
static int __devinit nx_pci_probe(struct pci_dev *pdev,
				   const struct pci_device_id *ent);
static void __devexit nx_pci_remove(struct pci_dev *pdev);
static void unm_pci_release_regions(struct pci_dev *pdev);

static int nx_pci_open(struct inode *inode, struct file *file);
static int nx_pci_release(struct inode *inode, struct file *file);
static void nx_sort_adapters(void);	

/*  PCI Device ID Table  */
#define NETXEN_PCI_ID(device_id)   PCI_DEVICE(PCI_VENDOR_ID_NX, device_id)

static struct pci_device_id unm_pci_tbl[] __devinitdata = {
	{NETXEN_PCI_ID(PCI_DEVICE_ID_NX_QG)},
	{NETXEN_PCI_ID(PCI_DEVICE_ID_NX_XG)},
	{NETXEN_PCI_ID(PCI_DEVICE_ID_NX_CX4)},
	{NETXEN_PCI_ID(PCI_DEVICE_ID_NX_IMEZ)},
	{NETXEN_PCI_ID(PCI_DEVICE_ID_NX_HMEZ)},
	{NETXEN_PCI_ID(PCI_DEVICE_ID_NX_IMEZ_DUP)},
	{NETXEN_PCI_ID(PCI_DEVICE_ID_NX_HMEZ_DUP)},
	{NETXEN_PCI_ID(PCI_DEVICE_ID_NX_P3_XG)},
	{0,}
};

MODULE_DEVICE_TABLE(pci, unm_pci_tbl);

static struct file_operations unm_pci_fops = {
	llseek: 	NULL,
	owner:          THIS_MODULE,
	open:           nx_pci_open,
	ioctl:		nx_xport_ioctl,
	release:	nx_pci_release,
};

static void nx_sort_adapters(void)
{
	struct unm_adapter_s * temp = NULL;
	int i, j;

	for (i = 0; i < (current_adapter_index - 1); ++i)
	{
		for (j = 0; j < current_adapter_index - 1 - i; ++j )
		{
			if ((all_adapters[j]->pdev->bus->number > all_adapters[j+1]->pdev->bus->number) ||
			((all_adapters[j]->pdev->bus->number == all_adapters[j+1]->pdev->bus->number) &&
			 (PCI_FUNC(all_adapters[j]->pdev->devfn) > PCI_FUNC(all_adapters[j+1]->pdev->devfn))))
			{
				temp = all_adapters[j+1];
				all_adapters[j+1] = all_adapters[j];
				all_adapters[j] = temp;
				all_adapters[j]->adapter_index = j;
				all_adapters[j+1]->adapter_index = j + 1;
			}
		}
	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0) && LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 29)
static struct module* find_module(char *modname)
{
        struct module *mod = NULL;
        struct module *modules = THIS_MODULE;

        list_for_each_entry(mod, &modules->list, list) {
                if (strcmp(mod->name, modname) == 0)
                        return mod;
        }
        return NULL;
}
#endif

static int
nx_pci_open(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    if (!try_module_get(THIS_MODULE))
        return -EINVAL;
#else
    MOD_INC_USE_COUNT;
#endif
    return 0;
}

static int
nx_pci_release(struct inode *inode, struct file *file)
{
        /*
         * This function must release the resources associated with this
         * instance (file), not those associated with the module.
         */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        module_put(THIS_MODULE);
#else
        MOD_DEC_USE_COUNT;
#endif
        return 0;
}

static void cleanup_adapter(struct unm_adapter_s *adapter)
{
	struct pci_dev *pdev;

	if (!adapter) {
		return;
	}
	pdev = adapter->ahw.pdev;

	if (pdev == NULL) {
		return;
	}
	if (adapter->ahw.pci_base0) {
		iounmap((uint8_t *) adapter->ahw.pci_base0);
		adapter->ahw.pci_base0 = 0UL;
	}
	if (adapter->ahw.pci_base1) {
		iounmap((uint8_t *) adapter->ahw.pci_base1);
		adapter->ahw.pci_base1 = 0UL;
	}
	if (adapter->ahw.pci_base2) {
		iounmap((uint8_t *) adapter->ahw.pci_base2);
		adapter->ahw.pci_base2 = 0UL;
	}
	if(adapter)
		kfree(adapter);
	unm_pci_release_regions(pdev);
	pci_disable_device(pdev);
}

static inline int unm_pci_region_offset(struct pci_dev *pdev, int region)
{
	unsigned long val;
	u32 control;

	switch (region) {
	case 0:
		val = 0;
		break;
	case 1:
		pci_read_config_dword(pdev, UNM_PCI_REG_MSIX_TBL, &control);
		val = control + UNM_MSIX_TBL_SPACE;
		break;
	}
	return val;
}

static inline int unm_pci_region_len(struct pci_dev *pdev, int region)
{
	unsigned long val;
	u32 control;
	switch (region) {
	case 0:
		pci_read_config_dword(pdev, UNM_PCI_REG_MSIX_TBL, &control);
		val = control;
		break;
	case 1:
		val = pci_resource_len(pdev, 0) -
		    unm_pci_region_offset(pdev, 1);
		break;
	}
	return val;
}

static int unm_pci_request_regions(struct pci_dev *pdev, char *res_name)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,11)
	return pci_request_regions(pdev, res_name);
#else
	struct resource *res;
	unsigned int len;

	/*
	 * In P3 these memory regions might change and need to be fixed.
	 */
	len = pci_resource_len(pdev, 0);
	if (len <= NX_MSIX_MEM_REGION_THRESHOLD || !use_msi_x ||
	    unm_pci_region_len(pdev, 0) == 0 ||
	    unm_pci_region_len(pdev, 1) == 0) {
		res = request_mem_region(pci_resource_start(pdev, 0),
					 len, res_name);
		goto done;
	}

	/* In case of MSI-X  pci_request_regions() is not useful, because
	   pci_enable_msix() tries to reserve part of card's memory space for
	   MSI-X table entries and fails due to conflict, since nx_nic module
	   owns entire region.
	   soln : request region(s) leaving area needed for MSI-X alone */
	res = request_mem_region(pci_resource_start(pdev, 0) +
				 unm_pci_region_offset(pdev, 0),
				 unm_pci_region_len(pdev, 0), res_name);
	if (res == NULL) {
		goto done;
	}
	res = request_mem_region(pci_resource_start(pdev, 0) +
				 unm_pci_region_offset(pdev, 1),
				 unm_pci_region_len(pdev, 1), res_name);

	if (res == NULL) {
		release_mem_region(pci_resource_start(pdev, 0) +
				   unm_pci_region_offset(pdev, 0),
				   unm_pci_region_len(pdev, 0));
	}
      done:
	return (res == NULL);
#endif
}

static void unm_pci_release_regions(struct pci_dev *pdev)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,11)
	pci_release_regions(pdev);
#else
	unsigned int len;

	len = pci_resource_len(pdev, 0);
	if (len <= NX_MSIX_MEM_REGION_THRESHOLD || !use_msi_x ||
	    unm_pci_region_len(pdev, 0) == 0 ||
	    unm_pci_region_len(pdev, 1) == 0) {
		release_mem_region(pci_resource_start(pdev, 0), len);
		return;
	}

	release_mem_region(pci_resource_start(pdev, 0) +
			   unm_pci_region_offset(pdev, 0),
			   unm_pci_region_len(pdev, 0));
	release_mem_region(pci_resource_start(pdev, 0) +
			   unm_pci_region_offset(pdev, 1),
			   unm_pci_region_len(pdev, 1));
#endif
}

/*
 * Linux system will invoke this after identifying the vendor ID and device Id
 * in the pci_tbl where this module will search for UNM vendor and device ID
 * for quad port adapter.
 */
static int __devinit nx_pci_probe(struct pci_dev *pdev,
				   const struct pci_device_id *ent)
{
	struct unm_adapter_s *adapter = NULL;
	uint8_t *mem_ptr0 = NULL;
	uint8_t *mem_ptr1 = NULL;
	uint8_t *mem_ptr2 = NULL;
	unsigned long mem_base, mem_len, pci_len0;
	unsigned long first_page_group_start, first_page_group_end;
	int err;
	int pci_func_id = PCI_FUNC(pdev->devfn);
	uint8_t revision_id;

        if (pdev->class != 0x020000) {
                nx_nic_print3(NULL, "function %d, class 0x%x will not be "
			      "enabled.\n", pci_func_id, pdev->class);
                return -ENODEV;
        }

        if ((err = pci_enable_device(pdev))) {
                nx_nic_print3(NULL, "Cannot enable PCI device. Error[%d]\n",
			      err);
                return err;
        }

        if (!(pci_resource_flags(pdev, 0) & IORESOURCE_MEM)) {
                nx_nic_print3(NULL, "Cannot find proper PCI device "
			      "base address, aborting. %p\n", pdev);
                err = -ENODEV;
		pci_disable_device(pdev);
		pci_set_drvdata(pdev, NULL);
		return err;

        }

        if ((err = unm_pci_request_regions(pdev, nx_driver_name))) {
                nx_nic_print3(NULL, "Cannot find proper PCI resources. "
			      "Error[%d]\n", err);
		pci_disable_device(pdev);
		pci_set_drvdata(pdev, NULL);
		return err;

        }

	adapter = kmalloc(sizeof(struct unm_adapter_s), GFP_KERNEL);
	if(!adapter) {
		err = -ENOMEM;
		unm_pci_release_regions(pdev);
		pci_disable_device(pdev);
		pci_set_drvdata(pdev, NULL);
		return err;
	}
	pci_set_master(pdev);

	pci_read_config_byte(pdev, PCI_REVISION_ID, &revision_id);

        nx_nic_print6(NULL, "Probe: revision ID = 0x%x\n", revision_id);

	memset(adapter, 0, sizeof(struct unm_adapter_s));
	adapter->ahw.pdev = pdev;
	adapter->ahw.pci_func = pci_func_id;
	adapter->ahw.revision_id = revision_id;
	rwlock_init(&adapter->adapter_lock);
	spin_lock_init(&adapter->lock);
	adapter->ahw.qdr_sn_window = -1;
	adapter->ahw.ddr_mn_window = -1;
	adapter->pdev = pdev;
	adapter->msglvl = NX_NIC_NOTICE;

	/* remap phys address */
	mem_base = pci_resource_start(pdev, 0);	/* 0 is for BAR 0 */
	mem_len = pci_resource_len(pdev, 0);

	/* 128 Meg of memory */
	nx_nic_print6(NULL, "ioremap from %lx a size of %lx\n", mem_base,
		      mem_len);

        if (NX_IS_REVISION_P2(revision_id) &&
	    (mem_len == UNM_PCI_128MB_SIZE || mem_len == UNM_PCI_32MB_SIZE)) {

		adapter->unm_nic_hw_write_wx = &unm_nic_hw_write_wx_128M;
		adapter->unm_nic_hw_write_ioctl =
			&unm_nic_hw_write_ioctl_128M;
		adapter->unm_nic_hw_read_wx =
			&unm_nic_hw_read_wx_128M;
		adapter->unm_nic_hw_read_ioctl =
			&unm_nic_hw_read_ioctl_128M;
		adapter->unm_crb_writelit_adapter =
			&unm_crb_writelit_adapter_128M;
		adapter->unm_nic_pci_set_window =
			&unm_nic_pci_set_window_128M;
		adapter->unm_nic_pci_mem_read =
			&unm_nic_pci_mem_read_128M;
		adapter->unm_nic_pci_mem_write =
			&unm_nic_pci_mem_write_128M;
		adapter->unm_nic_pci_write_immediate =
			&unm_nic_pci_write_immediate_128M;
		adapter->unm_nic_pci_read_immediate =
			&unm_nic_pci_read_immediate_128M;
		adapter->unm_nic_pci_write_normalize =
			&unm_nic_pci_write_normalize_128M;
		adapter->unm_nic_pci_read_normalize =
			&unm_nic_pci_read_normalize_128M;

		if (mem_len == UNM_PCI_128MB_SIZE) {
			mem_ptr0 = ioremap(mem_base, FIRST_PAGE_GROUP_SIZE);
			pci_len0 = FIRST_PAGE_GROUP_SIZE;
			mem_ptr1 = ioremap(mem_base + SECOND_PAGE_GROUP_START,
					   SECOND_PAGE_GROUP_SIZE);
			mem_ptr2 = ioremap(mem_base + THIRD_PAGE_GROUP_START,
					   THIRD_PAGE_GROUP_SIZE);
			first_page_group_start = FIRST_PAGE_GROUP_START;
			first_page_group_end   = FIRST_PAGE_GROUP_END;
		} else {
			pci_len0 = 0;
			mem_ptr1 = ioremap(mem_base, SECOND_PAGE_GROUP_SIZE);
			mem_ptr2 = ioremap(mem_base + THIRD_PAGE_GROUP_START -
					   SECOND_PAGE_GROUP_START,
					   THIRD_PAGE_GROUP_SIZE);
			first_page_group_start = 0;
			first_page_group_end   = 0;
		}

        } else if (NX_IS_REVISION_P3(revision_id) &&
		   mem_len == UNM_PCI_2MB_SIZE) {

		adapter->unm_nic_hw_read_wx = &unm_nic_hw_read_wx_2M;
		adapter->unm_nic_hw_write_wx = &unm_nic_hw_write_wx_2M;
		adapter->unm_nic_hw_read_ioctl = &unm_nic_hw_read_ioctl_2M;
		adapter->unm_nic_hw_write_ioctl = &unm_nic_hw_write_ioctl_2M;
		adapter->unm_crb_writelit_adapter =
			&unm_crb_writelit_adapter_2M;
		adapter->unm_nic_pci_set_window =
			&unm_nic_pci_set_window_2M;
		adapter->unm_nic_pci_mem_read = &unm_nic_pci_mem_read_2M;
		adapter->unm_nic_pci_mem_write = &unm_nic_pci_mem_write_2M;
		adapter->unm_nic_pci_write_immediate =
			&unm_nic_pci_write_immediate_2M;
		adapter->unm_nic_pci_read_immediate =
			&unm_nic_pci_read_immediate_2M;
		adapter->unm_nic_pci_write_normalize =
			&unm_nic_pci_write_normalize_2M;
		adapter->unm_nic_pci_read_normalize =
			&unm_nic_pci_read_normalize_2M;
		mem_ptr0 = ioremap(mem_base, mem_len);
		pci_len0 = mem_len;
		first_page_group_start = 0;
		first_page_group_end   = 0;

		adapter->ahw.ddr_mn_window = 0;
		adapter->ahw.qdr_sn_window = 0;

		adapter->ahw.mn_win_crb = (0x100000 +
					   PCIE_MN_WINDOW_REG(pci_func_id));

		adapter->ahw.ms_win_crb = (0x100000 +
					   PCIE_SN_WINDOW_REG(pci_func_id));

        } else {
		nx_nic_print3(NULL, "Invalid PCI memmap[0x%lx] for chip "
			      "revision[%u]\n", mem_len, revision_id);
		err = -EIO;
		goto err_ret;
        }

	nx_nic_print6(NULL, "ioremapped at 0 -> %p, 1 -> %p, 2 -> %p\n",
		      mem_ptr0, mem_ptr1, mem_ptr2);

	adapter->ahw.pci_base0 = (unsigned long)mem_ptr0;
	adapter->ahw.pci_len0 = pci_len0;
	adapter->ahw.first_page_group_start = first_page_group_start;
	adapter->ahw.first_page_group_end = first_page_group_end;
	adapter->ahw.pci_base1 = (unsigned long)mem_ptr1;
	adapter->ahw.pci_len1 = SECOND_PAGE_GROUP_SIZE;
	adapter->ahw.pci_base2 = (unsigned long)mem_ptr2;
	adapter->ahw.pci_len2 = THIRD_PAGE_GROUP_SIZE;
	adapter->ahw.crb_base =
	    PCI_OFFSET_SECOND_RANGE(adapter, UNM_PCI_CRBSPACE);

	/*
	 * Set the CRB window to invalid. If any register in window 0 is
	 * accessed it should set the window to 0 and then reset it to 1.
	 */
	adapter->curr_window = 255;


	if ((mem_len != UNM_PCI_2MB_SIZE) &&
	    (((mem_ptr0 == 0UL) && (mem_len == UNM_PCI_128MB_SIZE)) ||
	    (mem_ptr1 == 0UL) || (mem_ptr2 == 0UL))) {
		nx_nic_print3(NULL, "Cannot remap adapter memory aborting.:"
			      "0 -> %p, 1 -> %p, 2 -> %p\n",
			      mem_ptr0, mem_ptr1, mem_ptr2);
		err = -EIO;
		goto err_ret;
	}

	/* This will be reset for mezz cards  */
	adapter->portnum = pci_func_id;
	adapter->ahw.vendor_id = pdev->vendor;
	adapter->ahw.device_id = pdev->device;
	pci_set_drvdata(pdev, adapter);

	adapter->adapter_index = current_adapter_index;	
	all_adapters[current_adapter_index ++ ] = adapter;
	return 0;

err_ret:
	cleanup_adapter(adapter);
	return err;
}

static void __devexit nx_pci_remove(struct pci_dev *pdev)
{
	struct unm_adapter_s *adapter;

	adapter = pci_get_drvdata(pdev);
	if (adapter == NULL) {
		return;
	}
	all_adapters[adapter->adapter_index] = NULL;
	cleanup_adapter(adapter);
	pci_set_drvdata(pdev, NULL);
}

static struct pci_driver nx_driver = {
	.name = nx_driver_name,
	.id_table = unm_pci_tbl,
	.probe = nx_pci_probe,
	.remove = __devexit_p(nx_pci_remove),
};

/* Driver Registration on UNM card    */
static int __init unm_init_module(void)
{
	int rv;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
        if (find_module("nx_nic")) {
                printk("unm_init_module: Remove the nx_nic driver first\n");
                return -1;
        }
        if (find_module("netxen_nic")) {
                printk("unm_nit_module: Remove the netxen_nic driver first\n");
                return -1;
        }
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13) && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
	rv = alloc_chrdev_region (&unm_drv_dev, 0, MAX_DEVS, "nx_pci");
        if (rv < 0) {
                        printk(KERN_ERR "alloc_chrdev_region:Unable to Alloc chrdev region");
                        return rv;
        }
        unm_drv_cdev = cdev_alloc();
        if (unm_drv_cdev == NULL) {
                        printk(KERN_ERR "cdev_alloc:Unable to Alloc chrdev cdev");
                        rv = -ENOMEM;
                        goto free_region;
        }
        /* initialize a new cdev and add to kernel*/
        cdev_init(unm_drv_cdev, &unm_pci_fops);
        kobject_set_name(&unm_drv_cdev->kobj,"nx_pci");
        unm_drv_cdev->owner = THIS_MODULE;
        rv = cdev_add(unm_drv_cdev, unm_drv_dev, MAX_DEVS);
        if (rv < 0) {
                        printk(KERN_ERR "cdev_add:Unable to add unm_drv_cdev in kernel\n");
                        goto free_cdev;
        }
	unm_pci_major = MAJOR(unm_drv_dev);
	/* Initilize Sys-Fs Stuff  */
        unm_drv_class = class_create(THIS_MODULE, "unm_pci_class");
        if (IS_ERR(unm_drv_class)) {
                        printk(KERN_ERR "Unable to create class\n");
                        goto free_cdev;

        }

        unm_drv_class_device = class_device_create (unm_drv_class, NULL, unm_drv_dev, NULL, "nx_pci");
        if (IS_ERR(unm_drv_class_device)) {
                        printk(KERN_ERR "Unable to create class device \n");
                        goto free_class;
        }
#else
	if ((rv = register_chrdev(unm_pci_major, "nx_pci",
	        &unm_pci_fops)) < 0) {
		printk("unm_pci_init: Cannot allocate major device number\n");
		return -1;
	}
	unm_pci_major = rv;
#endif
	printk("NX PCI driver: Major #%d\n", unm_pci_major);

	rv = PCI_MODULE_INIT(&nx_driver);
	if (rv)
		goto free_resources;
	nx_sort_adapters();

	return rv;

free_resources:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13) && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
                class_device_destroy(unm_drv_class, unm_drv_dev);
free_class:
                class_destroy(unm_drv_class);
free_cdev:
                cdev_del(unm_drv_cdev);
free_region:
                unregister_chrdev_region(unm_drv_dev, MAX_DEVS);
#else
                unregister_chrdev(unm_pci_major, "nx_pci");
#endif
                return rv;
}

static void __exit unm_exit_module(void)
{

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13) && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
        class_device_destroy(unm_drv_class, unm_drv_dev);
        class_destroy(unm_drv_class);
        cdev_del(unm_drv_cdev);
        unregister_chrdev_region(unm_drv_dev, MAX_DEVS);
#else
        unregister_chrdev(unm_pci_major, "nx_pci");
#endif

        pci_unregister_driver(&nx_driver);
}

module_init(unm_init_module);
module_exit(unm_exit_module);
