# This spec file was generated using Kpp
# If you find any problems with this spec file please report
# the error to ian geiser <geiseri@msoe.edu>
Summary:   
Name:      k3b
Version:   0.7.3
Release:   0.1
Copyright: GPL
Vendor:    Sebastian Trueg <trueg@informatik.uni-freiburg.de>
Url:       http://k3b.sourceforge.net
Icon:     k3b.xpm
Packager:  Sebastian Trueg <trueg@informatik.uni-freiburg.de>
Group:     Archiving/Cd burning
Source:    k3b-0.7.2.tar.gz
BuildRoot: 

%description
K3b - The CD Creator - Writing cds under linux made easy.

%prep
%setup
CFLAGS="$RPM_OPT_FLAGS" CXXFLAGS="$RPM_OPT_FLAGS" ./configure \
                 \
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
