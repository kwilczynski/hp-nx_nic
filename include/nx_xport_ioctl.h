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
#ifndef __NX_XPORT_IOCTL_H__
#define __NX_XPORT_IOCTL_H__

#define UNM_XPORT_CMD  			1

#define UNM_CMD_NONE 			0
#define UNM_CMD_PCI_READ 		1
#define UNM_CMD_PCI_WRITE 		2
#define UNM_CMD_PCI_MEM_READ 		3
#define UNM_CMD_PCI_MEM_WRITE 		4
#define UNM_CMD_PCI_CONFIG_READ 	5
#define UNM_CMD_PCI_CONFIG_WRITE 	6
#define UNM_CMD_GET_VERSION 		11
#define UNM_CMD_GET_MAX_FUNC		12
#define UNM_CMD_GET_PORT_NUM		13
#define UNM_CMD_EFUSE_CHIP_ID		14

typedef struct {
        __uint32_t cmd;
        __uint32_t unused1;
        __uint64_t off;
        __uint32_t size;
        __uint32_t rv;
        char u[64];
	__uint64_t ptr;
} nx_xport_ioctl_data_t;

#endif
