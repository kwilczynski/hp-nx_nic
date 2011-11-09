%define nx_name hp-nx_nic
%define nx_version 4.0.556
%define nx_release 1

Summary: HP NC-Series QLogic Driver for Linux
Name: %{nx_name}
Version: %{nx_version}
Release: %{nx_release}
Source0: %{name}-%{version}.tar.gz
Source1: hp_nx_nic_preamble
Vendor: QLogic, Corporation.
Packager: Hewlett-Packard Company
License: GPL
Group: System Environment/Kernel
Provides: %{name}
URL: http://www.qlogic.com
Requires: kernel, bash, %{name}-tools = %{version}-%{release}
Buildroot: %{_tmppath}/%{name}-%{version}/bldroot

%if %{!?KVER:0}%{?KVER:1}
%define kernel %{KVER}
%global kernel_version %{KVER}
%else
%define kernel %(echo `uname -r`)
%endif

%define packinfo "This package contains the Linux driver of the Netxen's Nic"
%define debug_package %{nil}

%define exec_script /usr/bin/nx_snmp.pl
%define netxen_driver /lib/modules/%{kernel}/kernel/drivers/net/netxen/netxen_nic.ko
%define snmpd_conf /etc/snmp/snmpd.conf
%define mid_obj_id .1.3.6.1.2.1.10.7.11.1
%define pass_entry %(echo "pass %{mid_obj_id} %{exec_script}")

# If a blacklist file exists put an entry in it otherwise put it in a modprobe file.
%define bl_files %(echo "/etc/modprobe.d/blacklist /etc/hotplug/blacklist %{bl_files2} %{bl_files3}")
%define bl_files2 %(echo "/etc/modprobe.conf.local /etc/modprobe.conf /etc/modules.conf")
%define bl_files3 %(echo "/etc/modprobe.d/blacklist.conf")
%define blacklist_file %(for f in %{bl_files}; do [ -f $f ] && echo $f && break; done)
%if "%{blacklist_file}" != "/etc/hotplug/blacklist"
%define blacklist_entry %(echo "blacklist netxen_nic")
%else
%define blacklist_entry netxen_nic
%endif

%define modprob_conf %(echo "/etc/modprobe.conf")

%if "%{_host_vendor}" == "suse"

%if %{!?KVER:1}%{?KVER:0}
%ifarch x86_64
%define flav default smp xen
%else
%define flav default smp bigsmp xen xenpae pae
%endif
%endif

%if %{!?KVER:0}%{?KVER:1}
%define flav %(echo %{KVER} | awk -F"-" '{print $3}')
%endif
%define kverrel ""
%endif

%if "%{_vendor}" == "redhat"

%if %{!?KVER:1}%{?KVER:0}
%define flav ""
%endif

%if %{!?KVER:0}%{?KVER:1}
%define flav %(echo %{KVER} | awk -F"el5" '{print $2}')
%if "%{flav}" == ""
%define flav default
%endif
%endif
%define kverrel %(echo `uname -r`|cut -f 3 -d "." | cut -f 1 -d "-")

%if "%{kverrel}" == "9"
%define rhel4 1
%else
%define rhel4 0
%endif

%if "%{kverrel}" == "32"
%define modprob_conf %(echo "/etc/modprobe.d/dist.conf")
%endif

%endif

%description
%{packinfo}

%define whatos %(echo `cat /etc/redhat-release 2> /dev/null | cut -f 1 -d " "`)
%define isddk %(echo `cat /etc/redhat-release 2> /dev/null | cut -f 2 -d " "`)

%define xen_srvr 0
%if "%{whatos}" == "XenServer"
%if "%{isddk}" == "DDK"
%define xen_srvr 1
%endif
%endif

%if "%{xen_srvr}" == "1"
%package kdump
summary: QLogic kdump driver for NIC
Group: System/kernel

%description kdump
%{packinfo}


%package xen
summary: QLogic kdump driver for NIC
Group: System/kernel

%description xen
%{packinfo}
%endif


%if "%{_host_vendor}" == "suse"
BuildRequires: kernel-source kernel-syms
%define kernel_source /usr/src/linux-obj/%{_target_cpu}
%{suse_kernel_module_package -x %flav -p %_sourcedir/hp_nx_nic_preamble}

%package KMP
summary: QLogic driver for NIC
Group: System/kernel

%description KMP
%{packinfo}

%else
%if "%{xen_srvr}" == "0"
BuildRequires: %kernel_module_package_buildreqs
%{kernel_module_package %flav -p %_sourcedir/hp_nx_nic_preamble}
%endif
%endif

%package tools
Summary: HP NC-Series QLogic user components for Linux driver
Group: System/Kernel

%description tools
Stuff for the QLogic Ethernet Kernel Module
for HP ProLiant systems.

%prep
%setup -n %{name}-%{version}
set -- *
mkdir source
mv "$@" source/
mkdir obj

%build
export EXTRA_CFLAGS='-DVERSION=\"%version\"'

%if "%{rhel4}" == "1"
%define kverrel %(echo %(echo `uname -r`| awk -F"EL" '{print $1}')"EL")
%define flavors_to_build %(echo `uname -r`| awk -F"EL" '{print $2}')
%endif

%if "%{xen_srvr}" == "1"
%define kverrel %(echo `uname -r`| awk -F"xen" '{print $1}')
%define flavors_to_build xen kdump
%endif

for flavor in %flavors_to_build; do
rm -rf obj/$flavor
cp -r source obj/$flavor
export UNMSRC=$PWD/obj/$flavor

%define compile_with %{kernel_source $flavor}

%if "%{rhel4}" == "1"
%define compile_with /lib/modules/%{kernel}/build/
%endif

%if "%{xen_srvr}" == "1"
%define compile_with /lib/modules/%{kverrel}$flavor/build/
%endif

make -C %{compile_with} modules M=$PWD/obj/$flavor
if [ -d obj/$flavor/nx_xport ]; then
make -C %{compile_with} modules M=$PWD/obj/$flavor/nx_xport
fi
cp $UNMSRC/nx_xport/nx_xport.ko $UNMSRC/bin
done
gzip < ./source/docs/nx_nic.4 > ./source/docs/nx_nic.4.gz

%install
export INSTALL_MOD_PATH=$RPM_BUILD_ROOT
export INSTALL_MOD_DIR=extra/%{name}
if [ "%{_host_vendor}" == "suse" ]; then
export INSTALL_MOD_DIR=updates/%{name}
fi
for flavor in %flavors_to_build; do
make  -C %{compile_with}  modules_install M=$PWD/obj/$flavor
make  -C %{compile_with} modules_install M=$PWD/obj/$flavor/nx_xport
done
install -D -m 644 ./source/docs/nx_nic.4.gz $RPM_BUILD_ROOT/%{_mandir}/man4/nx_nic.4.gz
install -D -m 755 ./source/bin/nx3fwct.bin $RPM_BUILD_ROOT/lib/firmware/nx3fwct.bin
install -D -m 755 ./source/bin/nx3fwmn.bin $RPM_BUILD_ROOT/lib/firmware/nx3fwmn.bin
install -D -m 755 ./source/bin/nxromimg.bin $RPM_BUILD_ROOT/lib/firmware/nxromimg.bin

%clean
rm -rf %{buildroot}

%post tools
# If the GPL Nic driver exists on this machine then blacklist it.
if [ -f %{netxen_driver} ]; then
    if ! grep -q "%{blacklist_entry}" %{blacklist_file}; then
        echo "%{blacklist_entry}" >> %{blacklist_file}
	echo "blacklist nx_xport" >> %{blacklist_file}
    fi
    if [ -f %{modprob_conf} ]; then
        if ! grep -q -E "install netxen_nic /sbin/modprobe nx_nic" %{modprob_conf}; then
            echo "install netxen_nic /sbin/modprobe nx_nic" >> %{modprob_conf}
	fi
	if ! grep -q -E "install nx_xport /bin/true" %{modprob_conf}; then
            echo "install nx_xport /bin/true" >> %{modprob_conf}
        fi
    fi
fi
depmod
if [ -f %{snmpd_conf} ]; then
    if ! grep -q "%{pass_entry}" %{snmpd_conf}; then
        echo "%{pass_entry}" >> %{snmpd_conf}
    fi
fi

%postun tools
# If the GPL Nic driver was blacklisted then remove that blacklisting.
if [ -f %{netxen_driver} ]; then
    grep -v "%{blacklist_entry}" %{blacklist_file} > %{blacklist_file}.tmp$$
    mv %{blacklist_file}.tmp$$ %{blacklist_file}
    grep -v "blacklist nx_xport" %{blacklist_file} > %{blacklist_file}.tmp$$
    mv %{blacklist_file}.tmp$$ %{blacklist_file}
fi
depmod
if [ -f %{snmpd_conf} ]; then
    grep -v "%{pass_entry}" %{snmpd_conf} > %{snmpd_conf}.tmp$$
    mv %{snmpd_conf}.tmp$$ %{snmpd_conf}
fi
if [ -f %{modprob_conf} ]; then
    cp %{modprob_conf} %{modprob_conf}.saved
    grep -v -E "install netxen_nic /sbin/modprobe nx_nic" %{modprob_conf}.saved > %{modprob_conf}
    cp %{modprob_conf} %{modprob_conf}.saved
    grep -v -E "install nx_xport /bin/true" %{modprob_conf}.saved > %{modprob_conf}
    rm %{modprob_conf}.saved
fi

%if "%{xen_srvr}" == "1"
%files kdump
%defattr(-, root, root)
%attr(644,root,root) /lib/modules/%{kverrel}kdump/extra/hp-nx_nic/driver/nx_nic.ko
%attr(644,root,root) /lib/modules/%{kverrel}kdump/extra/hp-nx_nic/nx_xport.ko

%files xen
%defattr(-, root, root)
%attr(644,root,root) /lib/modules/%{kverrel}xen/extra/hp-nx_nic/driver/nx_nic.ko
%attr(644,root,root) /lib/modules/%{kverrel}xen/extra/hp-nx_nic/nx_xport.ko
%endif

%if "%{rhel4}" == "1"
%files
%defattr(-, root, root)
%attr(644,root,root) /lib/modules/%{kernel}/extra/nx_nic.ko
%attr(644,root,root) /lib/modules/%{kernel}/extra/nx_xport.ko
%endif


%files tools
%defattr(-, root, root)
%{_mandir}/man4/nx_nic.4.gz
%attr(755,root,root) /lib/firmware/nx3fwct.bin
%attr(755,root,root) /lib/firmware/nx3fwmn.bin
%attr(755,root,root) /lib/firmware/nxromimg.bin

%define major_change (Severity:Critical)
%define medium_change (Severity:Medium)
%define minor_change (Severity:Minor)

%changelog
*Wed Nov 12 2008 Mahesh M.R <Mahesh.Mr@hp.com> %{version}-%{release}
-modified spec files to fit to kernel modules specification %{major_change}
