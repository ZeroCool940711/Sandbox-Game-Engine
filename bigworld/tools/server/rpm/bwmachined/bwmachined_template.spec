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
## PLACEHOLDER: PACKAGE SPECIFIC MACROS 


Name:		%{name}
Version:	%{version}.%{patch}
Release:	%{patch}
Group:		Middleware/MMOG
Vendor:		BigWorld Pty Ltd
URL:		http://www.bigworldtech.com/
Packager:	BigWorld Support <support@bigworldtech.com>
License:	BigWorld License
Summary:	The BWMachined component of the BigWorld server tools.
## ExcludeArch: <arch1>, <arch2>, ..., <archN>
ExclusiveArch: i386
## Excludeos: <os1>, <os2>, ..., <osN>
Exclusiveos: linux
## The next line is used for generating the actual BuildRoot line from script
## please do not remove it.
## PLACEHOLDER: BUILDROOT
Provides: bigworld-bwmachined
Requires(pre):  /sbin/service, /sbin/chkconfig
Requires(post): /sbin/service, /sbin/chkconfig
## Requires: 
## Obsoletes:
## Conflicts:


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

## PLACEHOLDER: FILES FOR RPM


%changelog
* Tue Aug 08 2008 BigWorld Support
- Version 1.0.


