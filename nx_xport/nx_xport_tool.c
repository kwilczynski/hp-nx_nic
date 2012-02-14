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

/*
 * Source file for tools routines
 */
#include <asm/types.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/smp_lock.h>

#include "nx_xport.h"
#include "nic_phan_reg.h"

#include "unm_version.h"
#include "nx_xport_ioctl.h"
#include "nic_cmn.h"

static int nx_xport_do_ioctl(unsigned long u_data)
{
	nx_xport_ioctl_data_t data;
	nx_xport_ioctl_data_t *up_data = (nx_xport_ioctl_data_t *)u_data;
	struct unm_adapter_s *adapter = NULL;
	int retval = 0;
	uint64_t efuse_chip_id = 0;
	
	if (copy_from_user(&data, up_data, sizeof(data))) {
		/* evil user tried to crash the kernel */
		printk("bad copy from userland: %d\n",
				(int)sizeof(data));
		retval = -EFAULT;
		goto error_out;
	}
	adapter = all_adapters[data.rv];
	
	if(!adapter)
	{
		printk("Adapter is NULL\n");
		retval = -EFAULT;
		goto error_out;
	}	

	/* Shouldn't access beyond legal limits of  "char u[64];" member */
	if (!data.ptr && (data.size > sizeof(data.u))) {
		/* evil user tried to crash the kernel */
		nx_nic_print6(adapter, "bad size: %d\n", data.size);
		retval = -EFAULT;
		goto error_out;
	}

	switch (data.cmd) {
		case UNM_CMD_PCI_READ:
			if ((retval = adapter->unm_nic_hw_read_ioctl(adapter, data.off,
							&(data.u), data.size)))
				goto error_out;
			if (copy_to_user((void *)&(up_data->u), &(data.u), data.size)) {
				nx_nic_print6(adapter, "bad copy to userland: %d\n",
						(int)sizeof(data));
				retval = -EFAULT;
				goto error_out;
			}
			data.rv = 0;
			break;

		case UNM_CMD_PCI_WRITE:
			data.rv = adapter-> unm_nic_hw_write_ioctl(adapter, data.off, &(data.u),
					data.size);
			break;

		case UNM_CMD_PCI_MEM_READ:
			nx_nic_print7(adapter, "doing unm_nic_cmd_pci_mm_rd\n");
			if ((adapter->unm_nic_pci_mem_read(adapter, data.off, &(data.u),
							data.size) != 0) ||
					(copy_to_user((void *)&(up_data->u), &(data.u),
						      data.size) != 0)) {
				nx_nic_print6(adapter, "bad copy to userland: %d\n",
						(int)sizeof(data));
				retval = -EFAULT;
				goto error_out;
			}
			data.rv = 0;
			nx_nic_print7(adapter, "read %lx\n", (unsigned long)data.u);
			break;

		case UNM_CMD_PCI_MEM_WRITE:
			if ((data.rv =
						adapter->unm_nic_pci_mem_write(adapter, data.off, &(data.u),
							data.size)) != 0) {
				retval = -EFAULT;
				goto error_out;
			}
			break;

		case UNM_CMD_PCI_CONFIG_READ:
			switch (data.size) {
				case 1:
					data.rv = pci_read_config_byte(adapter->ahw.pdev,
							data.off,
							(char *)&(data.u));
					break;
				case 2:
					data.rv = pci_read_config_word(adapter->ahw.pdev,
							data.off,
							(short *)&(data.u));
					break;
				case 4:
					data.rv = pci_read_config_dword(adapter->ahw.pdev,
							data.off,
							(u32 *) & (data.u));
					break;
			}
			if (copy_to_user((void *)&up_data->u, &data.u, data.size)) {
				nx_nic_print6(adapter, "bad copy to userland: %d\n",
						(int)sizeof(data));
				retval = -EFAULT;
				goto error_out;
			}
			break;

		case UNM_CMD_PCI_CONFIG_WRITE:
			switch (data.size) {
				case 1:
					data.rv = pci_write_config_byte(adapter->ahw.pdev,
							data.off,
							*(char *)&(data.u));
					break;
				case 2:
					data.rv = pci_write_config_word(adapter->ahw.pdev,
							data.off,
							*(short *)&(data.u));
					break;
				case 4:
					data.rv = pci_write_config_dword(adapter->ahw.pdev,
							data.off,
							*(u32 *) & (data.u));
					break;
			}
			break;

		case UNM_CMD_GET_VERSION:
			if (copy_to_user((void *)&(up_data->u), UNM_NIC_LINUX_VERSIONID,
						sizeof(UNM_NIC_LINUX_VERSIONID))) {
				nx_nic_print6(adapter, "bad copy to userland: %d\n",
						(int)sizeof(data));
				retval = -EFAULT;
				goto error_out;
			}
			break;

		case UNM_CMD_EFUSE_CHIP_ID:

			efuse_chip_id = 
				adapter->unm_nic_pci_read_normalize(adapter, 
						UNM_EFUSE_CHIP_ID_HIGH);
			efuse_chip_id <<= 32;
			efuse_chip_id |= 
				adapter->unm_nic_pci_read_normalize(adapter, 
						UNM_EFUSE_CHIP_ID_LOW);
			if(copy_to_user((void *) &up_data->u, &efuse_chip_id,
						sizeof(uint64_t))) {
				nx_nic_print4(adapter, "bad copy to userland: %d\n",
						(int)sizeof(data));
				retval = -EFAULT;
				goto error_out;
			}
			data.rv = 0;
			break;

		case UNM_CMD_GET_MAX_FUNC:
			if(copy_to_user((void *) &up_data->u, &current_adapter_index,
						sizeof(int))) {
				nx_nic_print4(adapter, "bad copy to userland: %d\n",
						(int)sizeof(data));
				retval = -EFAULT;
				goto error_out;
			}
			data.rv = 0;
			break;
		case UNM_CMD_GET_PORT_NUM:
			if(copy_to_user((void *) &up_data->u, &adapter->portnum,
						sizeof(int))) {
				nx_nic_print4(adapter, "bad copy to userland: %d\n",
						(int)sizeof(data));
				retval = -EFAULT;
				goto error_out;
			}
			data.rv = 0;
			break;

		default:
			nx_nic_print4(adapter, "bad command %d\n", data.cmd);
			retval = -EOPNOTSUPP;
			goto error_out;
	}
	put_user(data.rv, &(up_data->rv));
	nx_nic_print7(adapter, "done ioctl.\n");

error_out:
	return retval;

}
/*
 * nx_xport_ioctl ()    We provide the tcl/phanmon support through these
 * ioctls.
 */
long nx_xport_ioctl(struct file *file, unsigned int cmd, unsigned long u_data)
{
	int err = 0;

//        printk("doing ioctl\n");

	lock_kernel();

	if (!capable(CAP_NET_ADMIN))
		unlock_kernel();
		return -EPERM;

        switch (cmd) {
        case UNM_XPORT_CMD:
                err = nx_xport_do_ioctl(u_data);
                break;
	default:
                printk("ioctl cmd %x not supported\n", cmd);
		err = -EOPNOTSUPP;
		break;
	}

	unlock_kernel();
	return err;
}
