%define name    k3b
%define version 0.11.7
%define release 1

%define linux_distribution %(%{_sourcedir}/linuxdist.sh)

%define _rpmfilename %%{ARCH}/%%{NAME}-%%{VERSION}-%%{RELEASE}%(%{_sourcedir}/linuxdist.sh).%%{ARCH}.rpm

Summary:        Excellent CD-Burner for KDE3
Name:           k3b
Version:        %{version}
Release:        %{release}.xcyb
Epoch:		1
License:        GPL
Vendor:         Sebastian Trueg <trueg@k3b.org>
URL:            http://www.k3b.org
Packager:       Mihai Maties <mihai@xcyb.org>
Group:          Applications/Multimedia
Source:         k3b-%{version}.tar.bz2
Source1:	linuxdist.sh
BuildRoot:      %{_tmppath}/%{name}-%{version}
Prefix:         %(kde-config --prefix)
Provides:       cd-burner
Requires:       cdrecord >= 2.0, mkisofs >= 2.0, cdrdao >= 1.1, kdelibs >= 3.1, flac
BuildRequires:  kdelibs-devel >= 3.1, qt-devel, audiofile-devel, XFree86-devel
BuildRequires:	libart_lgpl-devel, libpng-devel, flac-devel

# Unfortunately the --without-libmad and --without-libvorbis options do not work
# because Sebastian always enables it if it's available.
# ( also the --without-arts parameter seems to be ignored, so actually only
#   --without-k3bsetup does work )
%{!?_without_libmad:Requires: libmad, id3lib}
%{!?_without_libmad:BuildRequires: libmad-devel, id3lib-devel}
%{!?_without_libogg:Requires: libvorbis, libogg}
%{!?_without_libogg:BuildRequires: libvorbis-devel, libogg-devel}
%{!?_without_arts:Requires: arts}
%{!?_without_arts:BuildRequires: arts-devel}


%description
K3b - The CD Creator - Writing cds under linux made easy. It has an extremely
easy to use interface and supports many features: data/audio/video/mixed
[on-the-fly] CD burning, CD copying, erasing and ripping, CD-text writing,
burning iso/bin-cue images and many more.


%prep
%setup

%build

CFLAGS="$RPM_OPT_FLAGS" CXXFLAGS="$RPM_OPT_FLAGS" \
./configure \
             --prefix=%{prefix} --libdir=%{_libdir}\
	         %{?_without_k3bsetup:--without-k3bsetup} \
	         %{?_without_libmad:--without-libmad} \
	         %{?_without_arts:--without-arts} \
                $LOCALFLAGS

make %{_smp_mflags}

%install

make install-strip DESTDIR=$RPM_BUILD_ROOT

desktop-file-install --vendor xcyborg --delete-original\
  --dir $RPM_BUILD_ROOT%{_datadir}/applications\
  --add-category X-Red-Hat-Base\
  --add-category AudioVideo\
  $RPM_BUILD_ROOT%{_datadir}/applnk/Multimedia/%{name}.desktop

desktop-file-install --vendor kde --delete-original \
  --dir $RPM_BUILD_ROOT%{_datadir}/applications \
  --add-category KDE\
  --add-category X-KDE-System \
  --add-category System \
  $RPM_BUILD_ROOT%{_datadir}/applnk/Settings/System/k3bsetup2.desktop

rm -rf $RPM_BUILD_ROOT%{_datadir}/applnk

cd $RPM_BUILD_ROOT

install -D -m 644 $RPM_BUILD_ROOT/usr/share/icons/crystalsvg/48x48/apps/k3b.png \
  $RPM_BUILD_ROOT/usr/share/pixmaps/k3b.png

find . -type d | sed '1,2d;s,^\.,\%attr(-\,root\,root) \%dir ,' > $RPM_BUILD_DIR/file.list.k3b
find . -type f | sed 's,^\.,\%attr(-\,root\,root) ,' >> $RPM_BUILD_DIR/file.list.k3b
find . -type l | sed 's,^\.,\%attr(-\,root\,root) ,' >> $RPM_BUILD_DIR/file.list.k3b

%clean
rm -rf $RPM_BUILD_ROOT
rm -rf $RPM_BUILD_DIR/%{name}-%{version}
rm -rf ../file.list.k3b

%files -f ../file.list.k3b
%doc AUTHORS
%doc COPYING
%doc ChangeLog
%doc FAQ
%doc INSTALL
%doc README
%doc TODO


%changelog
* Fri Mar 11 2004 - Christopher C. Weis <christopher-c-weis@uiowa.edu>
- added "--libdir" to "./configure" to account for the x86_64  /usr/lib64 directory

* Tue Feb 10 2004 - Mihai Maties <mihai@xcyb.org> - k3b-0.11.2-1.xcyb*
- removed AudioVideo category from k3b.desktop since on KDE3.2 k3b will
appear both in in the System menu and Multimedia menu

* Fri Jan 23 2004 - Mihai Maties <mihai@xcyb.org> - k3b-0.11-1.xcyb*
- spec automatically generated with K3b CVS "snapshooter" v0.3
- added flac and flac-devel to [Build]Requires

* Wed Jan 21 2004 - Mihai Maties <mihai@xcyb.org> - k3b-*
- added a define (%CVS) to be able to build stable and CVS versions with the same 
spec
- removed the mimetypes workaround since the Sebastian removed them from the
source tree as well
- removed the "personal" release number tag from %release

* Thu Jan 10 2004 - Mihai Maties <mihai@xcyb.org> - k3b-0.10.3-2.xcyb.9.*
- install x-iso and x-cue mimelinks only if needed to avoid conflicts with other
packages (e.g. kde3.2)
- install 'k3b setup 2' as a standalone application in K/System Tools/ 
- added a few translations to the desktop files

* Thu Nov 13 2003 - Mihai Maties <mihai@xcyb.org> - k3b-0.10.2-3.xcyb.8.*
- added small script (linuxdist.sh) that determines the linux distribution
the package was built on and adds it to the name of the rpm (fc1, rh9, mdk82 etc)
- fixed yet another bug with BuildRequires ( I need to get some sleep ... )
- removed my k3bsetup2.desktop file from the package / now using the original
one from the kit

* Thu Nov 13 2003 - Mihai Maties <mihai@xcyb.org> - k3b-0.10.2-2.xcyb.7.rh9
- fixed some bugs with BuildRequires
- added libart_lgpl and XFree86-devel to BuildRequires

* Fri Oct 31 2003 - Mihai Maties <mihai@xcyb.org> - k3b-0.10.1-2.xcyb.6.rh9
- added --without-libmad , --without-k3bsetup and --without-arts options
- added requirements for libmad* , libvorbis* and libogg*

* Mon Oct 27 2003 - Mihai Maties <mihai@xcyb.org> - k3b-0.10.1-1.xcyb.5.rh9
- increased Epoch value to 1 to increase priority of this package over other ones
(this rpm is suppose to be more up to date with K3b changes )

* Thu Oct 16 2003 - Mihai Maties <mihai@xcyb.org> - k3b-0.10-2.xcyb.4.rh9
- "manually" added k3bsetup2 to KDE's Control Center

* Thu Oct 09 2003 - Mihai Maties <mihai@xcyb.org> - k3b-0.10-1.xcyb.3.rh9
- corrected some issues in the "clean" section

* Tue Aug 19 2003 - Mihai Maties <mihai@xcyb.org> - k3b-0.10-CVS.20030819.xcyb.3.rh9
- added patch file that fixes a compile error on many systems
- rebuilt

* Wed Aug 06 2003 - Mihai Maties <mihai@xcyb.org> - k3b-0.10-CVS.20030801.xcyb.3.rh9
- added audiofile-devel to BuildRequires

* Sat Aug 02 2003 - Mihai Maties <mihai@xcyb.org> - k3b-0.10-CVS.20030801.xcyb.2.rh9
- adapted spec file for CVS release

* Fri Jul 19 2003 - Mihai Maties <mihai@xcyb.org> - k3b-0.9-2.rh9
- changed rpm name to reflect the release, too
- added "requires" and "provides" tags
- added "conflict" tag with arson (because of the mimetypes files)
- added separate doc directive to correctly mark appropriate files as documentation
- added patch file which:
  - removes wrong Categories from the original k3b.desktop file
  - adds romanian translation to the desktop-files

* Tue Jul 15 2003 - Mihai Maties <mihai@xcyb.org> - k3b-0.9.rh9
- initial rpm release

