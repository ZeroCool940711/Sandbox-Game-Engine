# ========== IMPORTANT ==========
# 
# DO NOT BUILD PACKAGES AS ROOT USER!!!!!!
#
# Doing so may damage the system (i.e. render the system unusable) if 
# the RPM spec file contains error, since the error may delete system 
# files.



# Macros. 
%define name					bigworld-bwmachined
%define bwmachined_name  		bwmachined2
%define version	1.9.1
%define patch	0


Name:		%{name}
Version:	%{version}.%{patch}
Release:	%{patch}
Group:		Middleware/MMOG
Vendor:		BigWorld Pty Ltd
URL:		http://www.bigworldtech.com/
Packager:	BigWorld Support <support@bigworldtech.com>
License:	BigWorld License
Summary:	The BWMachined component of the BigWorld server tools.
ExclusiveArch: i386
Exclusiveos: linux
BuildRoot: /tmp/bwmachined_DbQtvt
Provides: bigworld-bwmachined
Requires(pre):  /sbin/service, /sbin/chkconfig
Requires(post): /sbin/service, /sbin/chkconfig


%description
This package contains the BWMachined component of the BigWorld Server.  The
BWMachined component is run on every machine that runs a BigWorld process.  


# This is pre-install script.
%pre 

if [ -f %{_initrddir}/%{bwmachined_name} ]; then
	/sbin/service %{bwmachined_name} stop > /dev/null 2>&1
	/sbin/chkconfig --del %{bwmachined_name}
fi

# Delete old bwmachined, if any.
if [ -f /usr/local/sbin/bwmachined2 ]; then
	rm -rf /usr/local/sbin/bwmachined2
fi


# This is post-install script. 
%post

/sbin/chkconfig --add %{bwmachined_name}
/sbin/service %{bwmachined_name} start


# This is pre-uninstall script.
%preun 
# Only run this when the software is being uninstalled, rather than
# upgraded/updated. $1 is an argument passed to script automatically 
# which stores the count of version of the software installed after
# the installation or uninstall. 
if [ "$1" -eq 0 ]; then
	/sbin/service %{bwmachined_name} stop > /dev/null 2>&1
	/sbin/chkconfig --del %{bwmachined_name}
fi

exit 0


# Files to include in binary RPM.
%files 

%attr(550, root, root) %{_sbindir}/bwmachined2
%attr(755, root, root) %{_initrddir}/bwmachined2
%config %attr(744, root, root) %{_sysconfdir}/bwmachined.conf


%changelog
* Tue Aug 08 2008 BigWorld Support
- Version 1.0.


