# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# 
# vmwddk.inc
# This file should work with minimal modifications for any of the drivers
#
# Used tools
SED	:= /build/toolchain/lin32/sed-4.1.5/bin/sed
RM	:= /build/toolchain/lin32/coreutils-5.97/bin/rm
MKDIR	:= /build/toolchain/lin32/coreutils-5.97/bin/mkdir
MV	:= /build/toolchain/lin32/coreutils-5.97/bin/mv
CP	:= /build/toolchain/lin32/coreutils-5.97/bin/cp
FALSE	:= false

export CURRENT_DIR ?= $(shell pwd)

ifndef PARTNER_DEV_BUILD
	GCC_WRAP := $(CURRENT_DIR)/gcc_wrapper.pl
	LD_WRAP := $(CURRENT_DIR)/ld_wrapper.pl
        CLEAN := clean
else
	GCC_WRAP :=
	LD_WRAP :=
        CLEAN :=
endif 

AR := PATH="/build/toolchain/lin32/binutils-2.16.1-vt/i686-linux/bin:$(PATH)" \
	/build/toolchain/lin32/binutils-2.16.1-vt/i686-linux/bin/ar

LD := PATH="/build/toolchain/lin32/binutils-2.16.1-vt/i686-linux/bin:$(PATH)" \
	$(LD_WRAP) /build/toolchain/lin32/binutils-2.16.1-vt/i686-linux/bin/ld

RPM := PATH="/build/toolchain/lin32/rpm-4.0.5/bin:$(PATH)" \
	/build/toolchain/lin32/rpm-4.0.5/bin/rpm --rcfile /build/toolchain/lin32/rpm-4.0.5/lib/rpm/rpmrc

RPMBUILD := PATH="/build/toolchain/lin32/rpm-4.2.3-30_nonptl/bin/:$(PATH)" \
	/build/toolchain/lin32/rpm-4.2.3-30_nonptl/bin/rpmbuild --rcfile /build/toolchain/lin32/rpm-4.0.5/lib/rpm/rpmrc

CC_GCC_3_3 := \
   GCC_EXEC_PREFIX="/build/toolchain/lin32/gcc-3.3.3/lib/gcc-lib/" \
   PATH="/build/toolchain/lin32/gcc-3.3.3/bin:/build/toolchain/lin32/binutils-2.16.1-vt/i686-linux/bin:$(PATH)" \
   $(GCC_WRAP) /build/toolchain/lin32/gcc-3.3.3/bin/i686-linux-gcc

STDINC         	:= -nostdinc

CC_GCC_3_3_INCLUDE := -idirafter /build/toolchain/lin32/gcc-3.3.3/lib/gcc-lib/i686-linux/3.3.3/include 

VMW_CC 	:= $(CC_GCC_3_3) $(STDINC) $(CC_GCC_3_3_INCLUDE)

CC_GLOBALOPTS := -O2

CC_DBG_GLOBALOPTS := -g3

CC_KFLAGS := -D__KERNEL__ -fno-strength-reduce -fno-common

CC_KFLAGS += -march=i586

CC_KFLAGS += -falign-loops=4 -falign-jumps=4 -falign-functions=4

CC_KFLAGS += -fno-strict-aliasing

CC_KFLAGS += -fno-omit-frame-pointer -DCPU=586

CC_KFLAGS += -D__VMWARE__ -D__VMKERNEL_MODULE__ -DVMKERNEL_MODULE

# This is for beta builds only
DEBUG_DIR_DEFS := -DVMX86_DEBUG -DVMX86_BETA -DVMX86_FPU_SAFETY

# This is for release builds only
RELEASE_DIR_DEFS :=  -DVMX86_RELEASE

DIR_DEFS := -DVMX86_SERVER

GLOBAL_DEFS := -DVMX86_LIMITS
GLOBAL_DEFS += -DESX3_NETWORKING_NOT_DONE_YET
GLOBAL_DEFS += -DESX2_NET_SUPPORT
GLOBAL_DEFS += -DBUILT_BY_$(USER)
GLOBAL_DEFS += -DUSE_SSL

CC_WARNINGS := -Wall -Wstrict-prototypes
CC_WARNINGS += -Werror

CC_DEFS := -DMODULE -DSMP_CAPABLE_VMK_DRIVER

VMW_DBG_FLAGS 	:= $(CC_GLOBALOPTS) $(CC_DBG_GLOBALOPTS) $(CC_KFLAGS) $(DEBUG_DIR_DEFS) $(DIR_DEFS) $(GLOBAL_DEFS) $(CC_DEFS) $(CC_WARNINGS)

VMW_REL_FLAGS 	:= $(CC_GLOBALOPTS) $(CC_KFLAGS) $(RELEASE_DIR_DEFS) $(DIR_DEFS) $(GLOBAL_DEFS) $(CC_DEFS) $(CC_WARNINGS)

VMWINC :=	-I /opt/vmware/vmwddk/include 		\
	-I /opt/vmware/vmwddk/include/vmware/version		 \
	-I /opt/vmware/vmwddk/include/vmware/public 		\
	-I /opt/vmware/vmwddk/include/vmware/distribute		 \
	-I /opt/vmware/vmwddk/include/vmware/vm 		\
	-I /opt/vmware/vmwddk/include/vmware/driver		 \
	-I /opt/vmware/vmwddk/include/vmware/vmkapi		\
	-I /opt/vmware/vmwddk/include/vmware/vmklinux 		 \
	-I /opt/vmware/vmwddk/include/scsi/drivers		\
	-I /opt/vmware/vmwddk/include/scsi 			

VMW_GEN_FILES := $(CURRENT_DIR)/driver.sig
