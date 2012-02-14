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
#ifndef _NX_XPORT_H_
#define _NX_XPORT_H__

#include <linux/module.h>
#include <linux/version.h>

#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
#define MODVERSIONS
#endif

#if defined(MODVERSIONS) && !defined(__GENKSYMS__)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
#if (!defined(__VMKERNEL_MODULE__) && !defined(__VMKLNX__))
#include <linux/modversions.h>
#endif /* ESX */
#endif
#endif

#ifndef MODULE
#include <asm/i387.h>
#endif
#include <linux/pci.h>
#include "nx_hw.h"
#include "nic_cmn.h"
#include "unm_inc.h"
#include "nxhal_nic_api.h"
#include "unm_brdcfg.h"

#define HDR_CP 64

#ifndef in_atomic
#define in_atomic()     (1)
#endif

/* XXX try to reduce this so struct sk_buff + data fits into
 * 2k pool
 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,28)
#define PCI_DEVICE(vend,dev) \
         .vendor = (vend), .device = (dev), \
         .subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID
#define netdev_priv(dev) (dev->priv)
#endif

#define BAR_0            0
#define PCI_DMA_64BIT    0xffffffffffffffffULL
#define PCI_DMA_32BIT    0x00000000ffffffffULL

#define ADDR_IN_WINDOW0(off)    \
       ((off >= UNM_CRB_PCIX_HOST) && (off < UNM_CRB_PCIX_HOST2)) ? 1 : 0
#define ADDR_IN_WINDOW1(off)   \
       ((off > UNM_CRB_PCIX_HOST2) && (off < UNM_CRB_MAX)) ? 1 : 0

/*Do not use CRB_NORMALIZE if this is true*/
#define ADDR_IN_BOTH_WINDOWS(off)\
       ((off >= UNM_CRB_PCIX_HOST) && (off < UNM_CRB_DDR_NET)) ? 1 : 0

#if defined(CONFIG_X86_64) || defined(CONFIG_64BIT) || ((defined(ESX) && CPU == x86-64))
typedef unsigned long uptr_t;
#else
typedef unsigned uptr_t;
#endif

#define FIRST_PAGE_GROUP_START	0
#define FIRST_PAGE_GROUP_END	0x100000

#define SECOND_PAGE_GROUP_START	0x6000000
#define SECOND_PAGE_GROUP_END	0x68BC000

#define THIRD_PAGE_GROUP_START	0x70E4000
#define THIRD_PAGE_GROUP_END	0x8000000

#define FIRST_PAGE_GROUP_SIZE	FIRST_PAGE_GROUP_END - FIRST_PAGE_GROUP_START
#define SECOND_PAGE_GROUP_SIZE	SECOND_PAGE_GROUP_END - SECOND_PAGE_GROUP_START
#define THIRD_PAGE_GROUP_SIZE	THIRD_PAGE_GROUP_END - THIRD_PAGE_GROUP_START

/* normalize a 64MB crb address to 32MB PCI window
 * To use CRB_NORMALIZE, window _must_ be set to 1
 */
#define CRB_NORMAL(reg)	\
	(reg) - UNM_CRB_PCIX_HOST2 + UNM_CRB_PCIX_HOST
#define CRB_NORMALIZE(adapter, reg) \
	(void *)(uptr_t)(pci_base_offset(adapter, CRB_NORMAL(reg)))

#define DB_NORMALIZE(adapter, off) \
        (void *)(uptr_t)(adapter->ahw.db_base + (off))

#define UNM_WRITE_LOCK(lock) \
        do { \
                write_lock((lock)); \
        } while (0)

#define UNM_WRITE_UNLOCK(lock) \
        do { \
                write_unlock((lock)); \
        } while (0)

#define UNM_READ_LOCK(lock) \
        do { \
                read_lock((lock)); \
        } while (0)

#define UNM_READ_UNLOCK(lock) \
        do { \
                read_unlock((lock)); \
        } while (0)

#define UNM_WRITE_LOCK_IRQS(lock, flags)\
        do { \
                write_lock_irqsave((lock), flags); \
        } while (0)

#define UNM_WRITE_UNLOCK_IRQR(lock, flags)\
        do { \
                write_unlock_irqrestore((lock), flags); \
        } while (0)



#define	NX_NIC_EMERG	0
#define	NX_NIC_ALERT	1
#define	NX_NIC_CRIT	2
#define	NX_NIC_ERR	3
#define	NX_NIC_WARNING	4
#define	NX_NIC_NOTICE	5
#define	NX_NIC_INFO	6
#define	NX_NIC_DEBUG	7

#define	nx_nic_print(LVL, ADAPTER, FORMAT, ...)				\
	do {								\
		if (ADAPTER) {						\
			struct unm_adapter_s *TMP_ADAP;			\
			TMP_ADAP = (struct unm_adapter_s *)ADAPTER;	\
			if (LVL <= TMP_ADAP->msglvl) {			\
				printk("%s%s: " FORMAT,		\
				       nx_nic_kern_msgs[LVL],		\
				       nx_driver_name,		\
				       ##__VA_ARGS__);			\
			}						\
		} else if (LVL <= nx_nic_msglvl) {			\
			printk("%s%s: " FORMAT, nx_nic_kern_msgs[LVL],	\
			       nx_driver_name, ##__VA_ARGS__);	\
		}							\
	} while (0)

#define	NX_NIC_DEBUG_LVL	6
#if	NX_NIC_DEBUG_LVL >= 7
#define	nx_nic_print7(ADAPTER, FORMAT, ...)		\
        nx_nic_print(7, ADAPTER, FORMAT, ##__VA_ARGS__)
#else
#define	nx_nic_print7(ADAPTER, FORMAT, ...)
#endif
#if	NX_NIC_DEBUG_LVL >= 6
#define	nx_nic_print6(ADAPTER, FORMAT, ...)		\
        nx_nic_print(6, ADAPTER, FORMAT, ##__VA_ARGS__)
#else
#define	nx_nic_print6(ADAPTER, FORMAT, ...)
#endif
#define	nx_nic_print5(ADAPTER, FORMAT, ...)		\
        nx_nic_print(5, ADAPTER, FORMAT, ##__VA_ARGS__)
#define	nx_nic_print4(ADAPTER, FORMAT, ...)		\
        nx_nic_print(4, ADAPTER, FORMAT, ##__VA_ARGS__)
#define	nx_nic_print3(ADAPTER, FORMAT, ...)		\
        nx_nic_print(3, ADAPTER, FORMAT, ##__VA_ARGS__)
#define	nx_nic_print2(ADAPTER, FORMAT, ...)		\
        nx_nic_print(2, ADAPTER, FORMAT, ##__VA_ARGS__)
#define	nx_nic_print1(ADAPTER, FORMAT, ...)		\
        nx_nic_print(1, ADAPTER, FORMAT, ##__VA_ARGS__)
#define	nx_nic_print0(ADAPTER, FORMAT, ...)		\
        nx_nic_print(0, ADAPTER, FORMAT, ##__VA_ARGS__)



#define	nx_print(LEVEL, FORMAT, ...)					\
        printk(LEVEL "%s: " FORMAT, nx_driver_name, ##__VA_ARGS__)

#if	1			/* defined(NDEBUG) */
#define	nx_print1(LEVEL, FORMAT, ...)					\
        printk(LEVEL "%s: " FORMAT, nx_driver_name, ##__VA_ARGS__)
#else
#define	nx_print1(format, ...)
#endif

/* Note: Make sure to not call this before adapter->port is valid */
#define DPRINTK(nlevel, klevel, fmt, args...)  do { \
    } while (0)

/* only works for sizes that are powers of 2 */
#define UNM_ROUNDUP(i, size) ((i) = (((i) + (size) - 1) & ~((size) - 1)))

/*
 * One hardware_context{} per adapter
 * contains interrupt info as well shared hardware info.
 */
typedef struct _hardware_context {
	struct pci_dev *pdev;
	unsigned long pci_base0;
	unsigned long pci_len0;
	unsigned long pci_base1;
	unsigned long pci_len1;
	unsigned long pci_base2;
	unsigned long pci_len2;
	unsigned long first_page_group_end;
	unsigned long first_page_group_start;
	uint16_t vendor_id;
	uint16_t device_id;
	uint8_t revision_id;
	int pci_func;
	struct unm_adapter *adapter;

	uint32_t crb_base;

	int		qdr_sn_window;
	int		ddr_mn_window;
	unsigned long mn_win_crb;
	unsigned long ms_win_crb;
} hardware_context, *phardware_context;

#define UNM_MSIX_TBL_SPACE              8192
#define UNM_PCI_REG_MSIX_TBL            0x44


/* Following structure is for specific port information */
typedef struct unm_adapter_s
{
	hardware_context ahw;
	uint16_t portnum;	/* port number        */
	uint16_t physical_port;	/* port number        */
	struct pci_dev *pdev;
	rwlock_t adapter_lock;
	spinlock_t lock;
	uint32_t curr_window;
	uint32_t crb_win; 	/* should replace curr_window */
	int pci_using_dac;
	int adapter_index;
	uint8_t			msglvl;
        void (*unm_nic_pci_change_crbwindow)(struct unm_adapter_s *, uint32_t);
        int (*unm_nic_hw_write_wx)(struct unm_adapter_s *, u64, void *, int);
        int (*unm_nic_hw_write_ioctl)(struct unm_adapter_s *, u64,
				      void *, int);
	int (*unm_nic_hw_read_wx)(struct unm_adapter_s *, u64, void *, int);
	int (*unm_nic_hw_read_ioctl)(struct unm_adapter_s *, u64, void *, int);
	int (*unm_crb_writelit_adapter)(struct unm_adapter_s *,
					unsigned long, int);
	int (*unm_nic_pci_mem_read)(struct unm_adapter_s *adapter, u64 off,
				    void *data, int size);

	int (*unm_nic_pci_mem_write)(struct unm_adapter_s *adapter, u64 off,
				     void *data, int size);
	int (*unm_nic_pci_write_immediate)(struct unm_adapter_s *adapter,
					   u64 off, u32 *data);
	int (*unm_nic_pci_read_immediate)(struct unm_adapter_s *adapter,
					  u64 off, u32 *data);
	void (*unm_nic_pci_write_normalize)(struct unm_adapter_s *adapter,
					    u64 off, u32 data);
	u32 (*unm_nic_pci_read_normalize)(struct unm_adapter_s *adapter,
					  u64 off);
	unsigned long (*unm_nic_pci_set_window) (struct unm_adapter_s *adapter,
						 unsigned long long addr);
} unm_adapter;			/* unm_adapter structure */

extern struct unm_adapter_s *all_adapters[];
extern int current_adapter_index;
extern char	nx_driver_name[];
extern uint8_t	nx_nic_msglvl;
extern char	*nx_nic_kern_msgs[];

void unm_nic_pci_change_crbwindow_128M(unm_adapter *adapter, uint32_t wndw);

#define PCI_OFFSET_FIRST_RANGE(adapter, off)	\
	((adapter)->ahw.pci_base0 + off)
#define PCI_OFFSET_SECOND_RANGE(adapter, off)	\
	((adapter)->ahw.pci_base1 + off - SECOND_PAGE_GROUP_START)
#define PCI_OFFSET_THIRD_RANGE(adapter, off)	\
	((adapter)->ahw.pci_base2 + off - THIRD_PAGE_GROUP_START)

#define pci_base_offset(adapter, off)	\
	((((off) < ((adapter)->ahw.first_page_group_end)) &&                                            \
          ((off) >= ((adapter)->ahw.first_page_group_start))) ?			                        \
		((adapter)->ahw.pci_base0 + (off)) :							\
		((((off) < SECOND_PAGE_GROUP_END) && ((off) >= SECOND_PAGE_GROUP_START)) ?		\
			((adapter)->ahw.pci_base1 + (off) - SECOND_PAGE_GROUP_START) :			\
			((((off) < THIRD_PAGE_GROUP_END) && ((off) >= THIRD_PAGE_GROUP_START)) ?	\
				((adapter)->ahw.pci_base2 + (off) - THIRD_PAGE_GROUP_START) :		\
				0)))

#define pci_base(adapter, off)	\
	((((off) < ((adapter)->ahw.first_page_group_end)) &&                                            \
          ((off) >= ((adapter)->ahw.first_page_group_start))) ?			                        \
		((adapter)->ahw.pci_base0) :								\
		((((off) < SECOND_PAGE_GROUP_END) && ((off) >= SECOND_PAGE_GROUP_START)) ?		\
			((adapter)->ahw.pci_base1) :							\
			((((off) < THIRD_PAGE_GROUP_END) && ((off) >= THIRD_PAGE_GROUP_START)) ?	\
				((adapter)->ahw.pci_base2) :						\
				0)))

static inline void
unm_nic_reg_write(unm_adapter * adapter, u64 off, __uint32_t val)
{				//Only for window 1
	adapter->unm_nic_hw_write_wx(adapter, off, &val, 4);
}

static inline int unm_nic_reg_read(unm_adapter * adapter, u64 off)
{				//Only for window 1
        int val;
	adapter->unm_nic_hw_read_wx(adapter, off, &val, 4);
	return val;
}

static inline void
unm_nic_write_w0(unm_adapter * adapter, uint32_t index, uint32_t value)
{				//Change the window to 0, write and change back to window 1.
	adapter->unm_nic_hw_write_wx(adapter, index, &value, 4);
}

static inline void
unm_nic_read_w0(unm_adapter * adapter, uint32_t index, uint32_t * value)
{				//Change the window to 0, read and change back to window 1.
	adapter->unm_nic_hw_read_wx(adapter, index, value, 4);
}

long nx_xport_ioctl(struct file *file, unsigned int cmd, unsigned long u_data);


/* Functions available from unm_nic_hw.c */
int unm_nic_reg_read(unm_adapter * adapter, u64 off);
int _unm_nic_hw_write(struct unm_adapter_s *adapter,
		      u64 off, void *data, int len);
int unm_nic_hw_write(struct unm_adapter_s *adapter,
		     u64 off, void *data, int len);
void unm_nic_reg_write(unm_adapter * adapter, u64 off, __uint32_t val);
int _unm_nic_hw_read(struct unm_adapter_s *adapter,
		     u64 off, void *data, int len);
int unm_nic_hw_read(struct unm_adapter_s *adapter,
		    u64 off, void *data, int len);
void _unm_nic_hw_block_read(struct unm_adapter_s *adapter,
			    u64 off, void *data, int num_words);
void unm_nic_hw_block_read(struct unm_adapter_s *adapter,
			   u64 off, void *data, int num_words);
void _unm_nic_hw_block_write(struct unm_adapter_s *adapter,
			     u64 off, void *data, int num_words);
void unm_nic_hw_block_write(struct unm_adapter_s *adapter,
			    u64 off, void *data, int num_words);
int  unm_nic_pci_mem_write_128M(struct unm_adapter_s *adapter,
			  u64 off, void *data, int size);
int  unm_nic_pci_mem_read_128M(struct unm_adapter_s *adapter,
			 u64 off, void *data, int size);
void unm_nic_mem_block_read(struct unm_adapter_s *adapter, u64 off,
			    void *data, int num_words);
void unm_nic_mem_block_write(struct unm_adapter_s *adapter, u64 off,
			     void *data, int num_words);

int unm_nic_hw_read_wx_128M(unm_adapter *adapter, u64 off, void *data, int len);
int unm_nic_hw_write_wx_128M(unm_adapter *adapter, u64 off, void *data, int len);
int unm_nic_hw_read_wx_2M(unm_adapter *adapter, u64 off, void *data, int len);
int unm_nic_hw_write_wx_2M(unm_adapter *adapter, u64 off, void *data, int len);
unsigned long unm_nic_pci_set_window_128M (struct unm_adapter_s *adapter, unsigned long long addr);
unsigned long unm_nic_pci_set_window_2M (struct unm_adapter_s *adapter, unsigned long long addr);
int unm_nic_hw_read_ioctl_128M(unm_adapter *adapter, u64 off, void *data, int len);
int unm_nic_hw_write_ioctl_128M(unm_adapter *adapter, u64 off, void *data, int len);
int unm_nic_hw_read_ioctl_2M(unm_adapter *adapter, u64 off, void *data, int len);
int unm_nic_hw_write_ioctl_2M(unm_adapter *adapter, u64 off, void *data, int len);

int unm_nic_pci_mem_read_2M(struct unm_adapter_s *adapter, u64 off,
                            void *data, int size);
int unm_nic_pci_mem_write_2M(struct unm_adapter_s *adapter, u64 off,
                             void *data, int size);


/*
 * Note : only 32-bit reads and writes !
 */
void unm_nic_pci_write_normalize_128M(unm_adapter *adapter, u64 off, u32 data);
u32  unm_nic_pci_read_normalize_128M(unm_adapter *adapter, u64 off);
int unm_nic_pci_write_immediate_128M(unm_adapter *adapter, u64 off, u32 *data);
int unm_nic_pci_read_immediate_128M(unm_adapter *adapter, u64 off, u32 *data);

void unm_nic_pci_write_normalize_2M(unm_adapter *adapter, u64 off, u32 data);
u32  unm_nic_pci_read_normalize_2M(unm_adapter *adapter, u64 off);
int unm_nic_pci_write_immediate_2M(unm_adapter *adapter, u64 off, u32 *data);
int unm_nic_pci_read_immediate_2M(unm_adapter *adapter, u64 off, u32 *data);

int unm_nic_pci_get_crb_addr_2M(unm_adapter *adapter, u64 *off, int len);

int unm_crb_write_adapter(unsigned long off, void *data,
			  struct unm_adapter_s *adapter);
int unm_crb_writelit_adapter_128M(struct unm_adapter_s *, unsigned long, int);
int unm_crb_writelit_adapter_2M(struct unm_adapter_s *, unsigned long, int);

#endif
