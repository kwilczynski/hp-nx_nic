ifneq ($(KERNELRELEASE),)
obj-y	:= driver/
else
#RPM_BUILD is set to 1 while building UNM NIC rpm. Hence the rules which
#should not be executed while building the rpm should be enclosed in
#      ifeq ($(RPM_BUILD),0)
#      endif

.PHONY :	all clean clobber install driver tools uninstall
.PHONY :        nxflash phanmon lib nxdiag nxtool nxlic cleantools

vmwddk_dir             =       $(shell [ -e "/opt/vmware/vmwddk" ] && echo "/opt/vmware/vmwddk")
vmwddk_tools		= 	$(shell [ -e "/build/toolchain" ] && echo "/build/toolchain")
KERNEL_VERSION    	=       $(shell uname -r)
KERNEL_BASE		=	$(PREFIX)/lib/modules/$(KERNEL_VERSION)
KMAJORVERSION    	:=       $(shell echo $(KERNEL_VERSION) | sed 's/[^0-9]/ /g' | cut -f 1 -d ' ')
KMINORVERSION    	:=       $(shell echo $(KERNEL_VERSION) | sed 's/[^0-9]/ /g' | cut -f 2 -d ' ')
KSUBVERSION    		:=       $(shell echo $(KERNEL_VERSION) | sed 's/[^0-9]/ /g' | cut -f 3 -d ' ')
KMAINVERSION    	:=       $(KMAJORVERSION).$(KMINORVERSION)
KERNEL_DIR		?=	$(shell \
                                  if [ -d $(KERNEL_BASE)/build/include ]; \
                                      then echo $(KERNEL_BASE)/build; \
                                      else echo $(KERNEL_BASE)/source; fi)
DRIVER_INSTALL_DIR	=	$(shell \
                                  if [ "$$(uname -r | egrep "(xen|xen0|kdump)$$")" != "" ]; \
				      then echo $(KERNEL_BASE)/extra; \
				  else \
                                      echo $(KERNEL_BASE)/updates; \
                                  fi)
UNMSRC			=	$(shell pwd)
DEPMOD			=	/sbin/depmod
BINARY_INSTALL_DIR	=	$(PREFIX)/usr/local/bin

export UNMSRC
export vmwddk_dir
export vmwddk_tools

ifneq ($(strip $(vmwddk_dir)),)
DRIVER_SUFFIX          =       o
else
ifeq ($(KMAINVERSION),2.4)
DRIVER_SUFFIX		=	o
else
DRIVER_SUFFIX		=	ko
endif
endif

MANPATH := $(shell if [ ! "$(MANDIR)" = "" ] ; then echo $(MANDIR) ; \
	     else (manpath 2>/dev/null || echo $MANPATH) | sed 's/:/ /g' ; \
             fi )
ifneq ($(MANPATH),)
    test_dir = $(findstring $(dir), $(MANPATH))
else
    test_dir = $(shell [ -e $(dir) ] && echo $(dir))
endif
MANDIR := /usr/share/man /usr/man /usr/local/man
MANDIR := $(firstword $(foreach dir, $(MANDIR), $(test_dir)))

ifeq ($(MANDIR),)
  MANDIR := /usr/man
endif
ifneq ($(PREFIX),)
  MANDIR := $(PREFIX)$(MANDIR)
endif

dirlist = /lib/firmware/$(KERNEL_VERSION) /lib/firmware /usr/lib/hotplug/firmware
dirlist += /lib/modules
testdir = $(shell [ -d $(dir) ] && echo $(dir))
FIRMWARE_DIR := $(PREFIX)$(firstword $(foreach dir,$(dirlist),$(testdir)))
FIRMWARE_IMAGES = nx3fwct.bin nx3fwmn.bin nxromimg.bin

filelist = /etc/modprobe.d/blacklist /etc/hotplug/blacklist
filelist += /etc/modprobe.conf.local /etc/modprobe.conf /etc/modules.conf
testfile = $(shell [ -f $(file) ] && echo $(file))
BLACKLIST_FILE := $(firstword $(foreach file,$(filelist),$(testfile)))
ifneq ($(BLACKLIST_FILE),/etc/hotplug/blacklist)
    BLACKLIST_ENTRY := $(shell echo "blacklist netxen_nic")
else
    BLACKLIST_ENTRY := netxen_nic
endif
NETXEN_DRIVER = /lib/modules/$(KERNEL_VERSION)/kernel/drivers/net/netxen/netxen_nic.ko

all: driver

driver : nx_nic.4.gz nxflash.4.gz nxudiag.4.gz
ifneq ($(strip $(vmwddk_dir)),)
	$(MAKE) -C driver
else
ifeq ($(KMAINVERSION),2.4)
	$(MAKE) -C nx_xport
	cp $(UNMSRC)/nx_xport/nx_xport.o $(UNMSRC)/bin ;
	$(MAKE) -C driver
else
	set -e; \
	if [ -d pegnet ]; then \
		$(MAKE) -C $(KERNEL_DIR) SUBDIRS=`pwd` ; \
	else \
		cd driver; $(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(UNMSRC)/driver modules ; \
		cd $(UNMSRC); \
		cd nx_xport; $(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(UNMSRC)/nx_xport modules ; \
		cp $(UNMSRC)/nx_xport/nx_xport.ko $(UNMSRC)/bin ; \
	fi
endif
endif
	@if [ -d lib ] ; then $(MAKE) tools; fi
	@if [ "`grep -ic SuSE /proc/version`" == "0" ]; then \
		rm -f new_pci.ids; \
		./bin/patch_pciids.sh /usr/share/hwdata/pci.ids pci.updates.netxen new_pci.ids; \
		if [ -e /usr/share/hwdata/pcitable ]; then \
			if [ "`grep -ic nx_lsa /lib/modules/$(KERNEL_VERSION)/modules.dep`" == "0" ]; then \
				if [ "`grep -ic nx_nic /usr/share/hwdata/pcitable`" == "0" ]; then \
					rm -f new_pcitable; \
					./bin/patch_pcitbl.sh /usr/share/hwdata/pcitable pci.updates.netxen new_pcitable nx_nic; \
				fi \
			fi \
		fi \
	else \
		rm -f new_pci.ids new_pcitable; \
		./bin/patch_pciids.sh /usr/share/pci.ids pci.updates.netxen new_pci.ids; \
	fi

clean clobber:
ifneq ($(strip $(vmwddk_dir)),)
	$(MAKE) -C driver $@
else
ifeq ($(KMAINVERSION),2.4)
	$(MAKE) -C nx_xport $@
	$(MAKE) -C driver $@
else
	cd driver; $(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(UNMSRC)/driver clean
	cd $(UNMSRC);
	cd nx_xport; $(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(UNMSRC)/nx_xport clean

endif
endif
	@if [ -d lib ] ; then $(MAKE) cleantools; fi
	@if [ -f new_pci.ids ] ; then rm new_pci.ids; fi
	@if [ -f new_pcitable ] ; then rm new_pcitable; fi
	@if [[ -f nx_nic.4.gz || -f nxlic.4.gz || -f nxflash.4.gz || -f nxudiag.4.gz ]] ; then rm *.gz; fi  	

pciupdate:
	@if [ "`grep -ic SuSE /proc/version`" == "0" ]; then \
		cp new_pci.ids /usr/share/hwdata/pci.ids; \
		if [ -e /usr/share/hwdata/pcitable ]; then \
			if [ "`grep -ic nx_nic /usr/share/hwdata/pcitable`" == "0" ]; then \
				if [ -e new_pcitable ]; then \
					cp new_pcitable /usr/share/hwdata/pcitable; \
				fi \
			else \
				if [ "grep unm_nic /usr/share/hwdata/pcitable" ]; then \
					sed -i 's/unm_nic/nx_nic/g' /usr/share/hwdata/pcitable; \
				fi \
			fi \
		fi \
	else \
		cp new_pci.ids /usr/share/pci.ids; \
	fi
	
uninstall:
	@for image in $(FIRMWARE_IMAGES); do \
	     rm -f $(FIRMWARE_DIR)/$$image; \
	 done
	@rm -f $(DRIVER_INSTALL_DIR)/nx_nic.$(DRIVER_SUFFIX)
	@rm -f $(MANDIR)/man4/nx_nic.4.gz
	@rm -f $(MANDIR)/man4/nxlic.4.gz
	@rm -f $(MANDIR)/man4/nxudiag.4.gz
	@rm -f $(MANDIR)/man4/nxflash.4.gz
ifeq ($(PREFIX),)
	@if [ -f $(NETXEN_DRIVER) ]; then \
	    if grep -q "$(BLACKLIST_ENTRY)" $(BLACKLIST_FILE); then \
	        grep -v "$(BLACKLIST_ENTRY)" $(BLACKLIST_FILE) > $(BLACKLIST_FILE).tmp$$; \
	        mv $(BLACKLIST_FILE).tmp$$ $(BLACKLIST_FILE); \
	    fi; \
	fi
endif

install: nx_nic.4.gz nxflash.4.gz nxudiag.4.gz pciupdate
	if [ ! -d $(DRIVER_INSTALL_DIR) ]; then \
	    echo "Could not find place to install driver !\n"; \
	    exit 1 ; \
	else \
	    rm -f $(DRIVER_INSTALL_DIR)/unm_nic.$(DRIVER_SUFFIX); \
	    cp driver/nx_nic.$(DRIVER_SUFFIX) $(DRIVER_INSTALL_DIR); \
	fi
	if [ $(KMAJORVERSION) -gt 2 ] \
           || [ $(KMAJORVERSION) -eq 2 -a $(KMINORVERSION) -gt 4 ] \
           || [ "$(KMAINVERSION)" = "2.4" -a $(KSUBVERSION) -ge 28 ]; then \
	    for image in $(FIRMWARE_IMAGES); do \
	        if [ -f bin/$$image ]; then \
	            cp bin/$$image $(FIRMWARE_DIR); \
	        fi; \
	    done; \
	fi
ifeq ($(PREFIX),)
ifeq ($(NOBLACKLIST),)
	if [ -f $(NETXEN_DRIVER) ]; then \
	    if ! grep -q "$(BLACKLIST_ENTRY)" $(BLACKLIST_FILE); then \
	        cp $(BLACKLIST_FILE) $(BLACKLIST_FILE).saved; \
	        echo "$(BLACKLIST_ENTRY)" >> $(BLACKLIST_FILE); \
	    fi; \
	fi
endif
	if [ -d $(DRIVER_INSTALL_DIR) ]; then \
	    $(DEPMOD); \
	fi
endif
	install -D -m 644 nx_nic.4.gz $(MANDIR)/man4/nx_nic.4.gz
	if [ -e nxudiag.4.gz ]; then \
	    install -D -m 644 nxudiag.4.gz $(MANDIR)/man4/nxudiag.4.gz; \
	fi
	if [ -e nxflash.4.gz ]; then \
	    install -D -m 644 nxflash.4.gz $(MANDIR)/man4/nxflash.4.gz; \
	fi
	@man -c -P'cat > /dev/null' nx_nic || true

tools : nxlic nxflash phanmon nxdiag nxtool

lib:
	$(MAKE) -C lib

nxflash: nxflash.4.gz lib
	$(MAKE) -C nxflash
	cp nxflash/nxflash.bin bin/

phanmon: lib
	$(MAKE) -C phanmon

nxdiag: nxudiag.4.gz lib
	$(MAKE) -C nxdiag
	cp nxdiag/nxdiag bin/
	cp nxdiag/nxudiag bin/

nxtool: lib
	$(MAKE) -C nxtool
	cp nxtool/nxtool bin/

nxlic: lib
	$(MAKE) -C nxlic
	cp nxlic/nxlic bin/
	
phancore_ESX35:
	$(MAKE) -C lib/ -f Makefile.uw ESX_ARCH=ESX_35
	$(MAKE) -C phantom-core/ -f Makefile.uw ESX_ARCH=ESX_35

phancore_ESX40:
	$(MAKE) -C lib/ -f Makefile.uw ESX_ARCH=ESX_40
	$(MAKE) -C phantom-core/ -f Makefile.uw ESX_ARCH=ESX_40

nx_xport_ESX40:
	make -C nx_xport -f Makefile.vmware.kl SOURCE_PATH=/esx-cs/console-os26 
	cp nx_xport/nx_xport.ko bin
nx_xport_ESX35: 
	make -C nx_xport -f Makefile.vmware.35 SOURCE_PATH=/esx-cs/console-os
	cp nx_xport/nx_xport.o bin
nx_nic.4.gz: docs/nx_nic.4
	@gzip -c $< > $@

nxudiag.4.gz: docs/nxudiag.4
	@gzip -c $< > $@

nxflash.4.gz: docs/nxflash.4
	@gzip -c $< > $@

cleantools:
	$(MAKE) -C lib clean
	$(MAKE) -C nxflash clean
	$(MAKE) -C phanmon clean
	$(MAKE) -C nxdiag clean
	$(MAKE) -C nxtool clean
	$(MAKE) -C nxlic clean
	$(MAKE) -C libphantomrom clean
	$(MAKE) -C libmd5 clean
endif
