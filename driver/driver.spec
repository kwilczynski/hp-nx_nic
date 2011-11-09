summary: VMware ESX Driver
Name: @@RPMBASENAME@@
Version: @@DRIVERVERSION@@
Release: @@RPMBUILDNUMBER@@
Copyright: commercial
Vendor: VMware, Inc.
BuildRoot: @@CURRENTDIR@@/rpm
# For high-level RPM tools
Group: Applications/Emulators
# Obsoletes single driver rpm
Obsoletes: VMware-esx-drivers
Requires: vmware-hwdata >= 1.00
Requires: DriverAPI-8.0
%description
VMware ESX Driver the VW of virtualization.  Drivers wanted.

%prep

%install

%pre
%post
/usr/sbin/esxcfg-pciid -q --boot
%preun
%postun
/usr/sbin/esxcfg-pciid -q --boot
%files
%attr(-, root, root) /usr/lib/vmware/vmkmod
%attr(-, root, root) /usr/lib/vmware-debug/vmkmod
%attr(-, root, root) /etc/vmware/pciid
