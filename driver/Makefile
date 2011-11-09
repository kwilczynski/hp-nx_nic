# Copyright (C) 2003 - 2009 NetXen, Inc.
# All rights reserved.
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston,
# MA 02111-1307, USA.
# 
# The full GNU General Public License is included in this distribution
# in the file called LICENSE.
# 
# Contact Information:
# licensing@netxen.com
# NetXen, Inc.
# 18922 Forge Drive
# Cupertino, CA 95014
#
# Makefile for the UNM  NIC Driver
#

UNM_ARCH ?= P3
ifeq ($(UNM_ARCH),P3)
P3_MODE ?= P3_LEGACY
endif

KERNEL_VERSION=$(shell uname -r | cut -c1-3)
ifneq ($(strip $(vmwddk_dir)),)

ifneq ($(strip $(vmwddk_tools)),)
include Makefile.vmware.kl
else
include Makefile.vmware
endif

else
ifeq ($(KERNEL_VERSION), 2.4)

include Makefile.rules

NIC_OBJS = unm_nic_hw.o unm_nic_main.o unm_nic_init.o unm_nic_ethtool.o xge_mdio.o niu.o nxhal.o nxhal_v34.o unm_nic_procfs.o unm_nic_snmp.o unm_nic_tool.o unm_nic_lro.o nx_hash_table.o nx_mem_pool.o nx_pexq.o

CSRCS	 = $(NIC_OBJS:%.o=%.c)

NIC_MODULE = unm_nic.o

MODULES=$(NIC_MODULE)

ifndef CONFIG_MODULES
$(error Kernel must be configured for module support)
endif

.DEFAULT:	all

all: depend libs bins

libs: $(MODULES)

bins:	libs $(MODULES)

$(NIC_MODULE): $(NIC_OBJS)
	$(LD) -r $(NIC_OBJS) -o $@

export.ver: unm_nic_hw.c unm_nic_main.c
	$(CC) $(CFLAGS) -E -D__GENKSYMS__ unm_nic_hw.c |\
	$(GENKSYMS) -k $(GENKSYMS_KERNEL_VER) > $@
	$(CC) $(CFLAGS) -E -D__GENKSYMS__ unm_nic_main.c |\
	$(GENKSYMS) -k $(GENKSYMS_KERNEL_VER) >> $@

clean:
	rm -f $(NIC_OBJS) $(NIC_MODULE)

clobber:	clean
	$(RM) $(TARGET) $(SIMTARG) $(DIRT) .dependencies

ifdef CONFIG_MODVERSIONS
depend:		$(CSRCS) $(SIMSRCS) #export.ver
else
depend:		$(CSRCS) $(SIMSRCS)
endif
	@-$(RM) -f .dependencies
	$(MAKEDEPEND) -f- -- $(CFLAGS) -- $(CSRCS) $(SIMSRCS) > .dependencies
-include .dependencies

else

EXTRA_CFLAGS = -I$(UNMSRC) -I$(UNMSRC)/include

FEDORA                 ?=      $(shell uname -r |grep -o FC.)
ifeq ($(strip $(FEDORA)),)
EXTRA_CFLAGS += -DFEDORA=NOFC
else
EXTRA_CFLAGS += -DFEDORA=$(FEDORA)
endif

ifneq ($(strip $(shell uname -r | grep -i xen)),)
XEN_PVM_GUEST=1

xen_sysfs_dir = /sys/hypervisor/version
xen_sysfs_dir_major = $(xen_sysfs_dir)/major
xen_sysfs_dir_minor = $(xen_sysfs_dir)/minor
xen_sysfs_dir_extra = $(xen_sysfs_dir)/extra

maj=$(shell cat $(xen_sysfs_dir_major))
min=$(shell cat $(xen_sysfs_dir_minor))
ext=$(shell cat $(xen_sysfs_dir_extra) | grep -o '[[:digit:]]\+' | head -n 1)

XEN_VERSION = $(shell echo $$[$(maj)*(1<<16) + $(min)*(1<<8) + $(ext)])
endif

HOST_CPU=$(shell uname -m)
ifeq ($(HOST_CPU),x86_64)
EXTRA_CFLAGS += -DHOST_CPU_x86_64
endif

SKBUFF_H = $(srctree)/include/linux/skbuff.h
gso_size_field = 'unsigned.*gso_size.*'

GOT_GSO = $(shell grep -o $(gso_size_field) $(SKBUFF_H))

ifneq ($(GOT_GSO),)
EXTRA_CFLAGS += -DUSE_GSO_SIZE=1
endif

DO_BOUNCE = 0

ifdef XEN_PVM_GUEST
xen_304_version = $(shell echo $$[3*(1<<16) + 0*(1<<8) + 4])
DO_BOUNCE = $(shell echo $$[$(XEN_VERSION) > $(xen_304_version)])
endif

ifeq ($(DO_BOUNCE), 1)
EXTRA_CFLAGS += -DBOUNCE_BUFFERS
endif

DO_PORT_SHARING = 0
ifeq ($(DO_PORT_SHARING), 1)
EXTRA_CFLAGS += -DPORT_SHARING 
endif

EXTRA_CFLAGS += -DHOST_CPU=$(HOST_CPU) -D$(UNM_ARCH) -DPEGNET_NIC
ifeq ($(UNM_ARCH),P3)
EXTRA_CFLAGS += -D$(P3_MODE)
endif
EXTRA_CFLAGS += -DUNM_CONF_PROCESSOR=UNM_CONF_X86
EXTRA_CFLAGS += -DUNM_CONF_OS=UNM_CONF_LINUX -DUNM_HAL_NATIVE
EXTRA_CFLAGS += -I$(UNMSRC)/include
EXTRA_CFLAGS += -DUNM_X_HARDWARE=UNM_X_ASIC
EXTRA_CFLAGS += -DUNM_NIC_SNMP -DUNM_NIC_SNMP_TRAP

obj-m := nx_nic.o

nx_nic-y := unm_nic_hw.o unm_nic_main.o unm_nic_init.o \
	unm_nic_ethtool.o xge_mdio.o niu.o nxhal.o nxhal_v34.o unm_nic_procfs.o unm_nic_snmp.o \
	unm_nic_lro.o unm_nic_tool.o nx_hash_table.o nx_mem_pool.o nx_pexq.o

endif
endif
