%define name    k3b
%define version 0.10
%define release 1

Summary:        Excellent CD-Burner for KDE3.1
Name:           k3b
Version:        %{version}
Release:        %{release}.xcyb.3.rh9
Epoch:		0
License:        GPL
Vendor:         Sebastian Trueg <trueg@k3b.org>
URL:            http://www.k3b.org
Packager:       Mihai Maties <xcyborg@xteam.ro>
Group:          Applications/Multimedia
Source:         k3b-%{version}.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}
Prefix:         `kde-config --prefix`
Provides:       cd-burner
Requires:       cdrecord >= 2.0, mkisofs >= 2.0, cdrdao >= 1.1, kdelibs >= 3.1, arts
BuildRequires:  kdelibs-devel >= 3.1, arts-devel, qt-devel, audiofile-devel
Conflicts:      arson

%description
K3b - The CD Creator - Writing cds under linux made easy. It has an extremely 
easy to use interface and supports many features: data/audio/video/mixed 
[on-the-fly] CD burning, CD copying, erasing and ripping, CD-text writing, 
burning iso/bin-cue images and many more.


%prep
%setup
#%patch -p1
#cd kdeextragear-1
#ln -sf ../kde-common/admin
#make -f Makefile.cvs

%build

#cd kdeextragear-1 #For CVS version only
#WANT_AUTOCONF_2_5=1 make -f admin/Makefile.common
CFLAGS="$RPM_OPT_FLAGS" CXXFLAGS="$RPM_OPT_FLAGS" ./configure \
                 --prefix=%{prefix} \
                $LOCALFLAGS
# Setup for parallel builds
numprocs=`egrep -c ^cpu[0-9]+ /proc/stat || :`
if [ "$numprocs" = "0" ]; then
  numprocs=1
fi

make -j$numprocs

%install
#cd kdeextragear-1 #For CVS version only
make install-strip DESTDIR=$RPM_BUILD_ROOT

desktop-file-install --vendor xcyborg --delete-original\
  --dir $RPM_BUILD_ROOT%{_datadir}/applications\
  --add-category X-Red-Hat-Base\
  --add-category Application\
  --add-category System\
  --add-category AudioVideo\
  $RPM_BUILD_ROOT%{_datadir}/applnk/Multimedia/%{name}.desktop

#desktop-file-install --vendor xcyborg --delete-original\
#  --dir $RPM_BUILD_ROOT%{_datadir}/applications\
#  --add-category X-Red-Hat-Base\
#  --add-category Application\
#  --add-category System\
#  --add-category AudioVideo\
#  $RPM_BUILD_ROOT%{_datadir}/applnk/Multimedia/%{name}setup.desktop

rm -rf $RPM_BUILD_ROOT%{_datadir}/applnk

cd $RPM_BUILD_ROOT

install -D -m 644 $RPM_BUILD_ROOT/usr/share/icons/hicolor/48x48/apps/k3b.png \
  $RPM_BUILD_ROOT/usr/share/pixmaps/k3b.png

find . -type d | sed '1,2d;s,^\.,\%attr(-\,root\,root) \%dir ,' > $RPM_BUILD_DIR/file.list.k3b
find . -type f | sed 's,^\.,\%attr(-\,root\,root) ,' >> $RPM_BUILD_DIR/file.list.k3b
find . -type l | sed 's,^\.,\%attr(-\,root\,root) ,' >> $RPM_BUILD_DIR/file.list.k3b

%clean
rm -rf $RPM_BUILD_ROOT
rm -rf $RPM_BUILD_DIR/%{name}-%{version}
rm -rf ../file.list.k3b

%files -f ../file.list.k3b
#echo $PWD
#cd kdeextragear-1/k3b
%doc kdeextragear-1/k3b/AUTHORS
%doc kdeextragear-1/k3b/COPYING
%doc kdeextragear-1/k3b/ChangeLog
%doc kdeextragear-1/k3b/FAQ
%doc kdeextragear-1/k3b/INSTALL
%doc kdeextragear-1/k3b/README
%doc kdeextragear-1/k3b/TODO


%changelog
* Thu Oct 09 2003 - Mihai Maties <xcyborg@xteam.ro> - k3b-0.10-1.xcyb.3.rh9
- corrected some issues in the "clean" section

* Tue Aug 19 2003 - Mihai Maties <xcyborg@xteam.ro> - k3b-0.10-CVS.20030819.xcyb.3.rh9
- added patch file that fixes a compile error on many systems
- rebuilt

* Wed Aug 06 2003 - Mihai Maties <xcyborg@xteam.ro> - k3b-0.10-CVS.20030801.xcyb.3.rh9
- added audiofile-devel to BuildRequires

* Sat Aug 02 2003 - Mihai Maties <xcyborg@xteam.ro> - k3b-0.10-CVS.20030801.xcyb.2.rh9
- adapted spec file for CVS release

* Fri Jul 19 2003 - Mihai Maties <xcyborg@xteam.ro> - k3b-0.9-2.rh9
- changed rpm name to reflect the release, too
- added "requires" and "provides" tags
- added "conflict" tag with arson (because of the mimetypes files)
- added separate doc directive to correctly mark appropriate files as documentation
- added patch file which:
  - removes wrong Categories from the original k3b.desktop file
  - adds romanian translation to the desktop-files

* Tue Jul 15 2003 - Mihai Maties <xcyborg@xteam.ro> - k3b-0.9.rh9
- initial rpm release

