Summary:	Toolchain for mastering recordable DVD media
Name:		dvd+rw-tools
Version:	5.22.4.10.9
Release:	1
License:	GPL
Group:		Applications/Multimedia
Source:		http://fy.chalmers.se/~appro/linux/DVD+RW/tools/dvd+rw-tools-%{version}.tar.gz
URL:		http://fy.chalmers.se/~appro/linux/DVD+RW/
Requires:	mkisofs >= 1.10
BuildRoot:	%{_tmppath}/%{name}-root
BuildRequires:	kernel-headers wget

%description
Collection of tools to master DVD+RW/+R/-R/-RW media. For further
information see http://fy.chalmers.se/~appro/linux/DVD+RW/.

%prep
%setup -q

%build
make
[ -f index.html ] || wget -nd http://fy.chalmers.se/~appro/linux/DVD+RW/

%install
[ %{buildroot} == / ] || rm -rf %{buildroot}
cd %{_builddir}/%{name}-%{version}
make prefix=%{buildroot}%{_prefix} manprefix=%{buildroot}%{_mandir} install
mkdir -p %{buildroot}%{_docdir}/%{name}-%{version}-%{release}
cp -a index.html  %{buildroot}%{_docdir}/%{name}-%{version}-%{release}

%clean
[ %{buildroot} == / ] || rm -rf %{buildroot}

%files
%defattr(-,root,root)
%{_prefix}/bin/*
%doc %{_docdir}/%{name}-%{version}-%{release}
%doc %{_mandir}/man1/*

%changelog
* Sun Nov 28 2004 Andy Polyakov <appro@fy.chalmers.se>
- DVD-R Dual Layer DAO and Incremental support;
* Sun Sep 26 2004 Andy Polyakov <appro@fy.chalmers.se>
- fix for DVD+R recordings in Samsung TS-H542A;
* Sat Aug 28 2004 Andy Polyakov <appro@fy.chalmers.se>
- 5.21.4.10.8 release;
* Wed Aug 25 2004 Andy Polyakov <appro@fy.chalmers.se>
- Linux: fix for kernel version 2.6>=8, 2.6.8 itself is deficient,
  but the problem can be worked around by installing this version
  set-root-uid;
* Thu Jul 15 2004 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.20.4.10.8 release;
* Tue Jul 13 2004 Andy Polyakov <appro@fy.chalmers.se>
- Layer Break position sanity check with respect to dataset size;
- #if directive to get rid of sudo check at compile time with
  'make WARN=-DI_KNOW_ALL_ABOUT_SUDO';
* Mon Jul 12 2004 Andy Polyakov <appro@fy.chalmers.se>
- speed verification issue with 8x AccessTek derivatives addressed;
- -use-the-force-luke=noload to leave tray ejected at the end;
- allow to resume incomplete sessions recorded with -M option;
* Sat Jul  3 2004 Andy Polyakov <appro@fy.chalmers.se>
- -use-the-force-luke=break:size to set Layer Break position for
  Double Layer recordings;
* Fri Jun 25 2004 Andy Polyakov <appro@fy.chalmers.se>
- handle non-fatal OPC errors;
- DVD+R Double Layer support;
- -use-the-force-luke=4gms to allow ISO9660 directory structures
  to cross 4GB boundary, the option is effective only with DVD+R DL
  and for data to be accessible under Linux isofs kernel patch is
  required; 
* Wed May 26 2004 Andy Polyakov <appro@fy.chalmers.se>
- HP-UX: inconsistency between /dev/rdsk and /dev/rscsi names;
* Wed Apr 21 2004 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.19-1 hotfix to address "flushing cache takes forever;
* Sat Apr 18 2004 Andy Polyakov <appro@fy.chalmers.se>
- DVD-RAM reload if recorded with -poor-man;
- -use-the-force-luke=wrvfy for WRITE AND VERIFY(10);
- "flushing cache" takes forever;
- dvd+rw-format 4.10: add support for DVD-RAM;
* Sat Apr 10 2004 Andy Polyakov <appro@fy.chalmers.se>
- 5.19.4.9.7 release;
* Sun Apr  4 2004 Andy Polyakov <appro@fy.chalmers.se>
- LG GSA-4081B fails to "SET STREAMING" with "LBA OUT OF RANGE" for
  DVD+RW media, but not e.g. DVD-R;
- dvd+rw-booktype: BTC support;
* Sat Apr  3 2004 Andy Polyakov <appro@fy.chalmers.se>
- make DVD-RAM work in "poor-man" mode;
- average write speed report at the end of recording;
* Fri Apr  2 2004 Andy Polyakov <appro@fy.chalmers.se>
- dvd+rw-format 4.9: permit for DVD-RW blank even if format descriptors
  are not present;
* Thu Apr  1 2004 Andy Polyakov <appro@fy.chalmers.se>
- Solaris: get rid of media reload, which made it possible to improve
  volume manager experience as well;
- address speed verification issues with NEC ND-2500 and Plextor
  PX-708A;
* Mon Mar 22 2004 Andy Polyakov <appro@fy.chalmers.se>
- dvd+rw-tools-5.18.4.8.6: www.software.hp.com release;
* Sat Mar 20 2004 Andy Polyakov <appro@fy.chalmers.se>
- IRIX: IRIX 6.x port is added;
* Wed Feb 11 2004 Andy Polyakov <appro@fy.chalmers.se>
- minimize amount of compiler warnings on 64-bit platforms;
- skip count-down if no_tty_check is set;
- -use-the-force-luke=tracksize:size option by suggestion from K3b;
- Linux: fix for "Bad file descriptor" with DVD+RW kernel patch;
* Tue Jan 20 2004 Andy Polyakov <appro@fy.chalmers.se>
- refuse to run if ${SUDO_COMMAND} is set;
* Wed Jan 14 2004 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.17: fix for COMMAND SEQUENCE ERROR in the beginning of
  DVD-recording;
- drop privileges prior mkisofs -version;
* Tue Jan 13 2004 Andy Polyakov <appro@fy.chalmers.se>
- the last speed change required adaptations for Pioneer and LG units,
  which don't/fail to provide current write speed through GET
  PERFORMANCE despite the fact that the command is mandatory;
- HP-UX: retain root privileges in setup_fds, SIOC_IO requires them;
* Sun Jan  4 2004 Andy Polyakov <appro@fy.chalmers.se>
- switch to GET PERFORMANCE even for current write speed (most
  notably required for NEC and derivatives);
* Tue Dec 30 2003 Andy Polyakov <appro@fy.chalmers.se>
- Linux: fix for /proc/sys/dev/cdrom/check_media set to 1;
- HP-UX: INQUIRY buffer is required to be 128 bytes, well, "required"
  is wrong word in this context, as it's apparently a kernel bug
  addressed in PHKL_30038 (HPUX 11.11) and PHKL_30039 (HPUX 11.23);
* Fri Dec 26 2003 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.16: brown-bag bug in "LONG WRITE IN PROGRESS" code path;
* Mon Dec 22 2003 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.15: confusing output when DAO mode is manually engaged
  and DVD-RW media is minimally blanked;
- complement -use-the-force-luke=dao[:size] to arrange for piping
  non-iso images in DAO mode (size is to be expressed in 2KB chunks);
- Pioneer DVR-x06 apparently needs larger timeout to avoid "the LUN
  appears to be stuck" message in the beginning of DAO recording;
- HP-UX: fix-up umount code;
- HP-UX: make sure user doesn't pass /dev/rscsi/cXtYlZ, they should
  stick to /dev/rdsk/cXtYdZ;
- implement -use-the-force-luke=seek:N -Z /dev/dvd=image to arrange
  for 'builtin_dd if=image of=/dev/dvd obs=32k seek=N/16' (note that
  N is expected to be expressed in 2KB chunks);
- skip overwrite check for blank media to avoid read errors at start,
  which reportedly may cause bus reset in some configurations;
- make get_mmc_profile load media, explicit media load used to be on
  per platform basis, while it doesn't have to;
- postpone handle_events till after dry-run checkpoint;
- error reporting revised;
- Optorite seems to insist on resuming suspended DVD+RW format, at
  least it's apparently the only way to get *reliable* results
  (formally this contradicts specification, which explicitly states
  that format is to be resumed automatically and transparently);
- FreeBSD: FreeBSD 5-CURRENT since 2003-08-24, including 5.2 fails
  to pull sense data automatically, at least for ATAPI transport,
  so I reach for it myself (it's apparently a kernel bug, which
  eventually will be fixed, but I keep the workaround code just in
  case);
- -speed option in DVD+ context is enabled, upon release tested with
  Plextor PX-708A;
- make builtin_dd print amount of transferred data, together with
  -use-the-force-luke=seek:N it's easier to maintain "tar-formatted"
  rewritable media;
- dvd+rw-format 4.8: DVD-RW format fails if preceeded by dummy
  recording;
- make sure we talk to MMC unit, be less intrusive;
- unify error reporting;
- permit for -lead-out even for blank DVD+RW, needed(?) for SANYO
  derivatives;
- dvd+rw-booktype 5: support for Benq derivatives;
* Tue Dec  2 2003 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.14: TEAC DV-W50D and Lite-On LDW-811S failed to set
  recording velocity, deploy GET PERFORMANCE/SET STREAMING commands;
- Lite-On LDW-811S returns 0s in Write Speed descriptors in page 2A,
  this would cause a problem if DVD+ speed control was implemented;
* Thu Nov 20 2003 Andy Polyakov <appro@fy.chalmers.se>
- Solaris: use large-file API in setup_fds;
- HP-UX: HP-UX support is contributed by HP;
- block signals in the beginning of recording, formally it shouldn't
  be necessary, but is apparently needed for some units (is it?);
- prepare code for -speed even in DVD+ context, need a test-case...
* Sun Nov  2 2003 Andy Polyakov <appro@fy.chalmers.se>
- progress indicator process was orphaned if -Z /dev/cdrom=file.iso
  terminated prematurely;
- -overburn -Z /dev/cdrom=file.iso printed two "ignored" messages;
* Thu Oct 23 2003 Andy Polyakov <appro@fy.chalmers.se>
- '| growisofs -Z /dev/cdrom=/dev/fd/0' failed with
  "already carries isofs" even when running interactively, so I check
  on /dev/tty instead of isatty(0);
- error output was confusing when overburn condition was raised in
  dry-run mode;
- more sane default drain buffer size to minimize system load when
  unit fails to return usable buffer utilization statistics under
  "LONG WRITE IN PROGRESS" condition;
* Wed Oct  1 2003 Andy Polyakov <appro@fy.chalmers.se>
- LG GSA-4040B fails to auto-format DVD+RW blanks;
* Mon Sep 15 2003 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.13: support for Panasonic/MATSUSHITA DVD-RAM LF-D310;
* Sat Sep  6 2003 Andy Polyakov <appro@fy.chalmers.se>
- RPM build fix-ups, no version change;
* Fri Aug 31 2003 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.12: [major] issue with MODE SENSE/SELECT on SANYO
  derivatives, such as Optorite, is addressed;
- Linux can't open(2) a socket by /dev/fd/N, replace it with dup(2);
- more relaxed command line option parsing and simultaneously a
  zealous check to make sure that no mkisofs options are passed
  along with -[ZM] /dev/cdrom=image;
- report I/O error if input stream was less than 64K;
- -M /dev/cdrom=/dev/zero didn't relocate the lead-out in DVD-RW
  Restricted Overwrite;
* Fri Aug 15 2003 Andy Polyakov <appro@fy.chalmers.se>
- single Pioneer DVR-x06 user reported that very small fraction of
  his recordings get terminted with "LONG WRITE IN PROGRESS," even
  though growisofs explicitly reserves for this condition... It
  turned out that at those rare occasions unit reported a lot of free
  buffer space, which growisofs treated as error condition. It's not
  clear if it's firmware deficiency, but growisofs reserves even for
  this apparently rare condition now.
- dvd+rw-format 4.7: when formatting DVD+RW, Pioneer DVR-x06 remained
  unaccessible for over 2 minutes after dvd+rw-format exited and user
  was frustrated to poll the unit himself, now dvd+rw-format does it for
  user;
* Sun Aug  3 2003 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.11: authorship statement in -version output;
- make speed_factor floating point and print "Current Write Speed"
  factor for informational purposes;
- Pioneer DVR-x06 exhibited degraded performance when recording DVD+;
- Pioneer DVR-x06 failed to complete DVD+ recording gracefully;
- alter set-root-uid behaviour under Linux from "PAM-junky" to more
  conservative one;
- dvd+rw-format 4.6: -force to ignore error from READ DISC INFORMATION;
- -force was failing under FreeBSD with 'unable to unmount';
- undocumented -gui flag to ease progress indicator parsing for
  GUI front-ends;
* Fri Jul 14 2003 Andy Polyakov <appro@fy.chalmers.se>
- dvd+rw-format 4.5: increase timeout for OPC, NEC multi-format
  derivatives might require more time to fulfill the OPC procedure;
- growisofs 5.10: increase timeout for OPC, NEC multi-format
  derivatives might require more time to fulfill the OPC procedure;
- extended syntax for -use-the-force-luke option, it's now possible
  to engage DVD-R[W] dummy mode by -use-the-force-luke=[tty,]dummy
  for example, where "tty" substitutes for the original non-extended
  option meaning, see the source for more easter eggs;
- FreeBSD: compile-time option to pass -M /dev/fd/0 to mkisofs to
  make life easier for those who mount devfs, but not fdescfs;
- eliminate potential race conditions;
- avoid end-less loop if no media was in upon tray load;
- interpret value of MKISOFS environment variable as absolute path
  to mkisofs binary;
- to facilitate for GUI front-ends return different exit codes, most
  notably exit value of 128|errno denotes a fatal error upon program
  startup [messages worth popping up in a separate modal dialog
  perhaps?], errno - fatal error during recording and 1 - warnings
  at exit;
- to facilitate for GUI front-ends auto-format blank DVD+RW media;
- Linux: fix for failure to copy volume descriptors when DVD-RW
  Restricted Overwrite procedure is applied to patched kernel;
- FreeBSD: growisofs didn't close tray upon startup nor did the rest
  of the tools work with open tray;
- bark at -o option and terminate execution, the "problem" was that
  users seem to misspell -overburn once in a while, in which case it
  was passed down to mkisofs and an iso-image was dumped to current
  working directory instead of media;
- generalize -M /dev/cdrom=file.iso option, but if file.iso is not
  /dev/zero, insist on sane -C argument to be passed prior -M and
  double-verify the track starting address;
* Tue Jun 20 2003 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.9: some [SONY] firmwares make it impossible to tell
  apart minimally and fully blanked media, so we need a way to engage
  DAO manually [in DVD-RW]... let's treat multiple -dvd-compat options
  as "cry" for DAO;
- refuse to finalize even DVD-R media with -M flag (advise to fill
  it up with -M /dev/cdrom=/dev/zero too), apparently DVD-units
  [or is it just SONY?] also "misplace" legacy lead-out in the same
  manner as DVD+units;
- oops! DAO hung at >4MB buffer because of sign overflow;
- couple of human-readable error messages in poor_mans_pwrite64;
- work around Plextor firmware deficiency which [also] manifests as
  end-less loop upon startup;
* Wed Jun 14 2003 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.8: elder Ricoh firmwares seem to report events
  differently, which triggered growisofs and dvd+rw-format to
  end-less loop at startup [event handling was introduced in 5.6
  for debugging purposes];
- int ioctl_fd is transformed to void *ioctl_handle to facilitate
  port to FreeBSD;
- FreeBSD support contributed by Matthew Dillon;
- volume descriptors from second session were discarded in
  Restricted Overwrite since 5.6;
* Sun Jun  8 2003 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.7: Solaris 2.x USB workaround;
- 15 min timeout for FLUSH CACHE in DVD-RW DAO;
- revalidate recording speed;
- load media upon start-up (Linux used to auto-close tray upon open,
  but not the others, which is why this functionality is added so
  late);
- dvd+rw-mediainfo: DVD-R[W] MediaID should be printed now;
* Sat May 31 2003 Andy Polyakov <appro@fy.chalmers.se>
- Solaris support is merged;
* Mon May 26 2003 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.6: unconditional exit in set-root-uid assistant, mostly
  for aesthetic reasons;
- support for DVD-RW DAO recordings (whenever Pioneer-ish Quick
  Format is not an option, DAO should fill in for it, as it's the
  only recording strategy applicable after *minimal* blanking
  procedure);
- support for SG_IO pass-through interface, or in other words
  support for Linux 2>=5;
- 'growisofs -M /dev/cdrom=/dev/zero', this is basically a counter-
  intuitive kludge assigned to fill up multi-session write-once
  media for better compatibility with DVD-ROM/-Video units, to keep
  it mountable [in the burner unit] volume descriptors from previous
  session are copied to the new session;
- disable -dvd-compat with -M option and DVD+R, advice to fill up
  the media as above instead;
- postpone Write Page setup all the way till after dry_run check;
- if recording to write-once media is terminated by external event,
  leave the session opened, so that the recording can be resumed
  (though no promises about final results are made, it's just that
  leaving it open makes more sense than to close the session);
- ask unit to perform OPC if READ DISC INFORMATION doesn't return
  any OPC descriptors;
- get rid of redundant Quick Grow in Restricted Overwrite;
- dvd+rw-formwat 4.4: support for -force=full in DVD-RW context;
- ask unit to perform OPC if READ DISC INFORMATION doesn't return
  any OPC descriptors;
- new dvd+rw-mediainfo utility for debugging purposes;
* Thu May 1 2003 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.5: fix for ENOENT at unmount, I should have called myself
  with execlp, not execl;
- security: chdir to / in set-root-uid assistant;
- use /proc/mounts instead of MOUNTED (a.k.a. /etc/mtab) in Linux
  umount code;
- changed to 'all' target in Makefile to keep NetBSD people happy;
* Sun Apr 20 2003 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.4: setup_fds is introduced to assist ports to another
  platforms;
- set-root-uid assistant code directly at entry point (see main());
- OpenBSD/NetBSD port added;
* Thu Mar 27 2003 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.4: split first write to avoid "empty DMA table?" in
  kernel log;
- dvd+rw-format 4.3: natural command-line restrictions;
* Thu Mar 20 2003 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.3: refuse to burn if session starts close to or beyond
  the 4GB limit (due to limitation of Linux isofs implementation).
- media reload is moved to growisofs from dvd+rw-format.
- dry_run check is postponed all the way till the first write.
* Sat Mar 15 2003 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.3/dvd+rw-format 4.2: support for DVD-RW Quick Format,
  upon release tested with Pioneer DVR-x05.
- bug in DVD+RW overburn protection code fixed.
* Thu Feb 27 2003 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.2: brown-bag bug in "LONG WRITE IN PROGRESS" handling
  code fixed.
* Mon Feb 1 2003 Andy Polyakov <appro@fy.chalmers.se>
- code to protect against overburns.
- progress indicator to display recording velocity.
- re-make it work under Linux 2.2 kernel.
* Tue Jan 14 2003 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.1: support for DVD-R[W] writing speed control.
- dvd+rw-booktype 4: see the source.
* Tue Nov 26 2002 Andy Polyakov <appro@fy.chalmers.se>
- growisofs 5.0: support for DVD-R[W].
- dvd+rw-format 4.0: support for DVD-RW.
- growisofs 4.2: workaround for broken DVD+R firmwares (didn't make
  public by itself).
* Thu Nov 4 2002 Andy Polyakov <appro@fy.chalmers.se>
- Minor growisofs update. Uninitialized errno at exit when
  -Z /dev/scd0=image.iso is used.
* Thu Nov 3 2002 Andy Polyakov <appro@fy.chalmers.se>
- Initial packaging. Package version is derived from growisofs,
  dvd+rw-format and dvd+rw-booktype version. 4.0.3.0.3 means
  growisofs 4.0, dvd+rw-format 3.0 dvd+rw-booktype 3.
