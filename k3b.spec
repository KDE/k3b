%define name    k3b
%define version 0.9
%define release 1asp
%define distversion %( perl -e '$_=\<\>;/(\\d+)\\.(\\d)\\.?(\\d)?/; print "$1$2".($3||0)' /etc/*-release)

Summary:   CD-Burner for KDE3
Name:      k3b
Version:   %{version}
Release:   %{_vendor}_%{distversion}
Copyright: GPL
Vendor:    Sebastian Trueg <trueg@k3b.org>
Url:       http://www.k3b.org
Icon:      k3b.png
Packager:  Sebastian Trueg <trueg@k3b.org>
Group:     Archiving/Cd burning
Source:    k3b-%{version}.tar.gz
BuildRoot: /tmp/%{name}-%{version}
Prefix:    `kde-config --prefix`

%description
K3b - The CD Creator - Writing cds under linux made easy.

%prep
%setup
WANT_AUTOCONF_2_5=1 make -f admin/Makefile.common
CFLAGS="$RPM_OPT_FLAGS" CXXFLAGS="$RPM_OPT_FLAGS" ./configure \
                 --prefix=%{prefix} \
                $LOCALFLAGS
%build
# Setup for parallel builds
numprocs=`egrep -c ^cpu[0-9]+ /proc/stat || :`
if [ "$numprocs" = "0" ]; then
  numprocs=1
fi

make -j$numprocs

%install
make install-strip DESTDIR=$RPM_BUILD_ROOT

cd $RPM_BUILD_ROOT
find . -type d | sed '1,2d;s,^\.,\%attr(-\,root\,root) \%dir ,' > $RPM_BUILD_DIR/file.list.k3b
find . -type f | sed 's,^\.,\%attr(-\,root\,root) ,' >> $RPM_BUILD_DIR/file.list.k3b
find . -type l | sed 's,^\.,\%attr(-\,root\,root) ,' >> $RPM_BUILD_DIR/file.list.k3b

%clean
rm -rf $RPM_BUILD_ROOT/*
rm -rf $RPM_BUILD_DIR/k3b
rm -rf ../file.list.k3b


%files -f ../file.list.k3b
