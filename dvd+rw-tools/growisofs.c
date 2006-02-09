/*
 * growisofs 5.22 by Andy Polyakov <appro@fy.chalmers.se>.
 *
 * Use-it-on-your-own-risk, GPL bless...
 *
 * This front-end to mkisofs(8) was originally developed to facilitate
 * appending of data to ISO9660 volumes residing on random write access
 * DVD media such as DVD+RW, DVD-RAM, as well as plain files/iso images.
 * At later stages even support for multi-session recording to DVD
 * write-once media such as DVD+R and DVD-R was added.
 *
 * As for growing random access volumes. The idea is very simple. The
 * program appends new data as it was added to a multisession media and
 * then copies the new volume descriptor(s) to the beginning of media
 * thus effectively updating the root catalog reference...
 *
 * For further details see http://fy.chalmers.se/~appro/linux/DVD+RW/.
 *
 * Revision history:
 *
 * 1.1:
 * - flush cache before copying volume descriptors;
 * 2.0:
 * - support for /dev/raw*;
 * - support for set-root-uid operation (needed for /dev/raw*);
 * - support for first "session" burning (needed for "poor-man");
 * - "poor-man" support for those who don't want to recompile the
 *   kernel;
 * 2.1:
 * - mkisofs_pid typo;
 * 2.2:
 * - uninitialized in_device variable;
 * - -help option;
 * 3.0:
 * - support for DVD+R;
 * 3.1:
 * - -Z fails if a file system is present and stdin is not a tty;
 * 3.2:
 * - support for image burning (needed for DVD+R as you can't use dd);
 * 3.3:
 * - 'growisofs -Z /dev/scdN image.iso' is too confusing, implement
 *   'growisofs -Z /dev/scdN=image.iso' instead;
 * 4.0:
 * - transport C++-fication for better portability;
 * - support for -dvd-compat option (improved DVD+R/RW compatibility);
 * - -dvd-video implies -dvd-compat;
 * - support for SONY DRU-500A;
 * - progress indicator for -Z /dev/scdN=image.iso;
 * - agressive -poor-man-ing;
 * 4.1:
 * - uninitialized errno at exit from -Z /dev/scdN=image.iso;
 * 4.2:
 * - don't print initial bogus progress indicator values;
 * - apparently some firmwares exhibit ambiguity in DVD+R disc
 *   finalizing code;
 * 5.0:
 * - enforced 32K write strategy (needed for DVD-R[W]);
 * - support for DVD-RW Restricted Overwrite Mode;
 * - support for DVD-R[W] Sequential Mode;
 * 5.1:
 * - support for writing speed control;
 * 5.2:
 * - re-make it work under Linux 2.2 kernel;
 * - progress indicator to display recording velocity;
 * - code to protect against overburns;
 * - undocumented -use-the-force-luke flag to overwrite the media
 *   none interactively;
 * - brown-bag bug in "LONG WRITE IN PROGRESS" handling code fixed;
 * 5.3:
 * - Pioneer workarounds/fix-ups, most notably DVR-x05 doesn't seem
 *   to digest immediate "SYNC CACHE";
 * - support for DVD-RW Quick Format, upon release verified to work
 *   with Pioneer DVR-x05;
 * - bug in DVD+RW overburn "protection" code fixed;
 * - media reload is moved here from dvd+rw-format;
 * - refuse to burn if session starts close to or beyond 4GB limit
 *   (limitation of Linux isofs implementation);
 * - dry_run check is postponed all the way till the first write;
 * 5.4:
 * - split first write to two to avoid "empty DMA table?" in kernel log;
 * - setup_fds is introduced to assist ports to another platforms;
 * - set-root-uid assistant code directly at entry point (see main());
 * - OpenBSD/NetBSD support added, it's worth noting that unlike 3.3
 *   port by Maarten Hofman, it's /dev/rcd?c which is expected to be
 *   passed as argument, not /dev/cd?c.
 * 5.5:
 * - fix for ENOENT at unmount, I should have called myself with execlp,
 *   not execl;
 * - security: chdir("/") in set-root-uid assistant;
 * - use /proc/mounts instead of MOUNTED (a.k.a. /etc/mtab) in Linux
 *   umount code;
 * 5.6:
 * - unconditional exit in set-root-uid assistant, mostly for aesthetic
 *   reasons;
 * - support for DVD-RW DAO recordings (whenever Pioneer-ish Quick
 *   Format is not an option, DAO should fill in for it, as it's the
 *   only recording strategy applicable after *minimal* blanking
 *   procedure);
 * - support for SG_IO pass-through interface, or in other words
 *   support for Linux 2>=5;
 * - 'growisofs -M /dev/cdrom=/dev/zero', this is basically a counter-
 *   intuitive kludge assigned to fill up multi-session write-once
 *   media for better compatibility with DVD-ROM/-Video units, to keep
 *   it mountable [in the burner unit] volume descriptors from previous
 *   session are copied to the new session;
 * - disable -dvd-compat with -M option and DVD+R, advice to fill up
 *   the media as above instead;
 * - postpone Write Page setup all the way till after dry_run check;
 * - if recording to write-once media is terminated by external event,
 *   leave the session opened, so that the recording can be resumed
 *   (though no promises about final results are made, it's just that
 *   leaving it open makes more sense than to close the session);
 * - ask unit to perform OPC if READ DISC INFORMATION doesn't return
 *   any OPC descriptors;
 * - get rid of redundant Quick Grow in Restricted Overwrite;
 * - Solaris 2.x support is merged, it's volume manager aware, i.e.
 *   you can run it with or without volume manager;
 * 5.7:
 * - Solaris USB workaround;
 * - 15 min timeout for FLUSH CACHE in DVD-RW DAO;
 * - revalidate recording speed;
 * - load media upon start-up (Linux used to auto-close tray upon open,
 *   but not the others, which is why this functionality is added so
 *   late);
 * 5.8:
 * - elder Ricoh firmwares seem to report events differently, which
 *   triggered growisofs and dvd+rw-format to end-less loop at startup
 *   [event handling was introduced in 5.6 for debugging purposes];
 * - int ioctl_fd is transformed to void *ioctl_handle to facilitate
 *   port to FreeBSD;
 * - FreeBSD support contributed by Matthew Dillon;
 * - volume descriptors from second session were discarded in
 *   Restricted Overwrite since 5.6;
 * 5.9:
 * - some [SONY] firmwares make it impossible to tell apart minimally
 *   and fully blanked media, so we need a way to engage DAO manually
 *   [in DVD-RW]... let's treat multiple -dvd-compat options as "cry"
 *   for DAO;
 * - refuse to finalize even DVD-R media with -M flag (advise to fill
 *   it up with -M /dev/cdrom=/dev/zero too), apparently DVD-units
 *   [or is it just SONY?] also "misplace" legacy lead-out in the same
 *   manner as DVD+units;
 * - oops! DAO hung at >4MB buffer because of sign overflow;
 * - couple of human-readable error messages in poor_mans_pwrite64;
 * - work around Plextor firmware deficiency which [also] manifests as
 *   end-less loop upon startup;
 * 5.10:
 * - increase timeout for OPC, NEC multi-format derivatives might
 *   require more time to fulfill the OPC procedure;
 * - extended syntax for -use-the-force-luke option, it's now possible
 *   to engage DVD-R[W] dummy mode by -use-the-force-luke=[tty,]dummy
 *   for example, where "tty" substitutes for the original non-extended
 *   option meaning, see the source for more easter eggs;
 * - FreeBSD: compile-time option to pass -M /dev/fd/0 to mkisofs to
 *   make life easier for those who mount devfs, but not fdescfs;
 * - eliminate potential race conditions;
 * - avoid end-less loop if no media was in upon tray load;
 * - interpret value of MKISOFS environment variable as absolute path
 *   to mkisofs binary;
 * - to facilitate for GUI front-ends return different exit codes, most
 *   notably exit value of 128|errno denotes a fatal error upon program
 *   startup [messages worth popping up in a separate modal dialog
 *   perhaps?], errno - fatal error during recording and 1 - warnings
 *   at exit;
 * - to facilitate for GUI front-ends auto-format blank DVD+RW media;
 * - Linux: fix for failure to copy volume descriptors when DVD-RW
 *   Restricted Overwrite procedure is applied to patched kernel;
 * - FreeBSD: growisofs didn't close tray upon startup nor did the rest
 *   of the tools work with open tray;
 * - bark at -o option and terminate execution, the "problem" was that
 *   users seem to misspell -overburn once in a while, in which case it
 *   was passed down to mkisofs and an iso-image was dumped to current
 *   working directory instead of media;
 * - generalize -M /dev/cdrom=file.iso option, but if file.iso is not
 *   /dev/zero, insist on sane -C argument to be passed prior -M and
 *   double-verify the track starting address;
 * 5.11:
 * - authorship statement in -version output;
 * - make speed_factor floating point and print "Current Write Speed"
 *   factor for informational purposes;
 * - Pioneer DVR-x06 exhibited degraded performance when recording DVD+;
 * - Pioneer DVR-x06 failed to complete DVD+ recording gracefully;
 * - alter set-root-uid behaviour under Linux from "PAM-junky" to more
 *   conservative one;
 * 5.12:
 * - single Pioneer DVR-x06 user reported that very small fraction of
 *   his recordings get terminted with "LONG WRITE IN PROGRESS," even
 *   though growisofs explicitly reserves for this condition... It
 *   turned out that at those rare occasions unit reported a lot of free
 *   buffer space, which growisofs treated as error condition. It's not
 *   clear if it's firmware deficiency, but growisofs reserves even for
 *   this apparently rare condition now.
 * - [major] issue with MODE SENSE/SELECT on SANYO derivatives, such as
 *   Optorite, is addressed;
 * - Linux can't open(2) a socket by /dev/fd/N, replace it with dup(2);
 * - more relaxed command line option parsing and simultaneously a
 *   zealous check to make sure that no mkisofs options are passed
 *   along with -[ZM] /dev/cdrom=image;
 * - report I/O error if input stream was less than 64K;
 * - -M /dev/cdrom=/dev/zero didn't relocate the lead-out in DVD-RW
 *   Restricted Overwrite;
 * 5.13:
 * - workarounds for Panasonic/MATSUSHITA DVD-RAM LF-D310;
 * - Solaris: media load upon start-up;
 * 5.14:
 * - LG GSA-4040B failed to auto-format DVD+RW blanks;
 * - '| growisofs -Z /dev/cdrom=/dev/fd/0' failed with "already carries
 *   isofs" even when running interactively, so I check on /dev/tty
 *   instead of isatty(0);
 * - error output was confusing when overburn condition was raised in
 *   dry-run mode;
 * - more sane default drain buffer size to minimize system load when
 *   unit fails to return usable buffer utilization statistics under
 *   "LONG WRITE IN PROGRESS" condition;
 * - progress indicator process was orphaned if -Z /dev/cdrom=file.iso
 *   terminated prematurely;
 * - -overburn -Z /dev/cdrom=file.iso printed two "ignored" messages;
 * - Solaris: use large-file API in setup_fds;
 * - HP-UX: HP-UX support is contributed by HP;
 * - block signals in the beginning of recording, formally it shouldn't
 *   be necessary, but is apparently needed for some units (is it?);
 * - prepare code for -speed even in DVD+ context, need a test-case...
 * - TEAC DV-W50D and Lite-On LDW-811S failed to set recording velocity,
 *   deploy GET PERFORMANCE/SET STREAMING commands;
 * - Lite-On LDW-811S returns 0s in Write Speed descriptors in page 2A,
 *   this would cause a problem if DVD+ speed control was implemented;
 * 5.15:
 * - confusing output when DAO mode is manually engaged and DVD-RW media
 *   is minimally blanked;
 * - complement -use-the-force-luke=dao[:size] to arrange for piping
 *   non-iso images in DAO mode (size is to be expressed in 2KB chunks);
 * - Pioneer DVR-x06 apparently needs larger timeout to avoid "the LUN
 *   appears to be stuck" message in the beginning of DAO recording;
 * - HP-UX: fix-up umount code;
 * - HP-UX: make sure user doesn't pass /dev/rscsi/cXtYlZ, they should
 *   stick to /dev/rdsk/cXtYdZ;
 * - implement -use-the-force-luke=seek:N -Z /dev/dvd=image to arrange
 *   for 'builtin_dd if=image of=/dev/dvd obs=32k seek=N/16' (note that
 *   N is expected to be expressed in 2KB chunks);
 * - skip overwrite check for blank media to avoid read errors at start,
 *   which reportedly may cause bus reset in some configurations;
 * - make get_mmc_profile load media, explicit media load used to be on
 *   per platform basis, while it doesn't have to;
 * - postpone handle_events till after dry-run checkpoint;
 * - error reporting revised;
 * - Optorite seems to insist on resuming suspended DVD+RW format, at
 *   least it's apparently the only way to get *reliable* results
 *   (formally this contradicts specification, which explicitly states
 *   that format is to be resumed automatically and transparently);
 * - FreeBSD: FreeBSD 5-CURRENT since 2003-08-24, including 5.2 fails
 *   to pull sense data automatically, at least for ATAPI transport,
 *   so I reach for it myself (it's apparently a kernel bug, which
 *   eventually will be fixed, but I keep the workaround code just in
 *   case);
 * - -speed option in DVD+ context is enabled, upon release tested with
 *   Plextor PX-708A;
 * - make builtin_dd print amount of transferred data, together with
 *   -use-the-force-luke=seek:N it's easier to maintain "tar-formatted"
 *   rewritable media;
 * 5.16:
 * - brown-bag bug in "LONG WRITE IN PROGRESS" code path;
 * 5.17:
 * - Linux: fix for /proc/sys/dev/cdrom/check_media set to 1;
 * - HP-UX: INQUIRY buffer is required to be 128 bytes. Well, "required"
 *   is wrong word in this context, as it's apparently a kernel bug
 *   addressed in PHKL_30038 (HPUX 11.11) and PHKL_30039 (HPUX 11.23).
 *   This "change" affects all dvd+rw-tools, but I don't bump their
 *   version numbers for this, as it's an "ugly" workaround for an
 *   *external* problem;
 * - switch to GET PERFORMANCE even for current write speed (most
 *   notably required for NEC and derivatives);
 * - the above change required adaptations for Pioneer and LG units,
 *   which don't/fail to provide current write speed through GET
 *   PERFORMANCE despite the fact that the command is mandatory;
 * - HP-UX: retain root privileges in setup_fds, SIOC_IO requires them;
 * - fix for COMMAND SEQUENCE ERROR in the beginning of DVD-recording;
 * - drop privileges prior mkisofs -version;
 * 5.18:
 * - refuse to run if ${SUDO_COMMAND} is set;
 * - minimize amount of compiler warnings on 64-bit platforms;
 * - skip count-down if no_tty_check is set;
 * - -use-the-force-luke=tracksize:size option by suggestion from K3b;
 * - Linux: fix for "Bad file descriptor" with DVD+RW kernel patch;
 * 5.19:
 * - IRIX: IRIX 6.x port is added;
 * - Solaris: get rid of media reload, which made it possible to improve
 *   volume manager experience as well;
 * - address speed verification issues with NEC ND-2500 and Plextor
 *   PX-708A;
 * - make DVD-RAM work in "poor-man" mode;
 * - average write speed report at the end of recording;
 * - LG GSA-4081B fails to "SET STREAMING" with "LBA OUT OF RANGE" for
 *   DVD+RW media, but not e.g. DVD-R;
 * 5.20:
 * - DVD-RAM reload if recorded with -poor-man;
 * - -use-the-force-luke=wrvfy for WRITE AND VERIFY(10);
 * - "flushing cache" takes forever, from 5.19-1;
 * - HP-UX: inconsistency between /dev/rdsk and /dev/rscsi names;
 * - handle non-fatal OPC errors;
 * - DVD+R Double Layer support;
 * - -use-the-force-luke=4gms to allow ISO9660 directory structures
 *   to cross 4GB boundary, the option is effective only with DVD+R DL
 *   and for data to be accessible under Linux isofs a kernel patch is
 *   required;
 * - more sane sanity check for -use-the-force-luke=tracksize:N;
 * - -use-the-force-luke=break:size to set Layer Break position for
 *   Double Layer recordings;
 * - speed verification issue with 8x AccessTek derivatives addressed;
 * - -use-the-force-luke=noload to leave tray ejected at the end;
 * - allow to resume incomplete sessions recorded with -M option;
 * - Layer Break position sanity check with respect to dataset size;
 * - #if directive to get rid of sudo check at compile time with
 *   'make WARN=-DI_KNOW_ALL_ABOUT_SUDO';
 * 5.21:
 * - Linux: fix for kernel version 2.6>=8, 2.6.8 itself is deficient,
 *   but the problem can be worked around by installing this version
 *   set-root-uid;
 * 5.22:
 * - fix for DVD+R recordings in Samsung TS-H542A;
 * - TODO: increase priority;
 * - TODO: 4x is [intentinally] treated as 1x;
 * - TODO: http://lists.debian.org/cdwrite/2004/10/msg00045.html
 * - DVD-R Dual Layer DAO and Incremental support;
 * 5.22-k3b:
 * - Applied patch to retry close session.
 * - Added buffer status (very simplistic, should be reworked before release)
 * - Added lazy option (for now default) which makes growisofs ignore some error
 *   like failed speed change or an opc error.
 * - raise priority of the process
 * - growisofs is installed suid root
 */
#define PRINT_VERSION(cmd)	do {			\
    char *s=strrchr((cmd),'/');				\
    s ? s++ : (s=(cmd));				\
    printf ("* %.*sgrowisofs by <appro@fy.chalmers.se>,"\
	    " version 5.22-k3b,\n",(int)(s-(cmd)),(cmd));	\
} while (0)

#define _LARGEFILE_SOURCE 
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#if defined(__linux)
/* ... and "engage" glibc large file support */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#elif defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__)
#define off64_t  off_t
#define stat64   stat
#define fstat64  fstat
#define open64   open
#define pread64  pread
#define pwrite64 pwrite
#define lseek64  lseek
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <signal.h>
#include <poll.h>
#include <assert.h>
#include <sys/time.h>

#ifdef __FreeBSD__
/* Defines for setpriority(2) */
#include <sys/resource.h>
#endif

#define	FATAL_START(err)	(0x80|(err))
#ifndef EMEDIUMTYPE
#define EMEDIUMTYPE     EINVAL
#endif
#ifndef ENOMEDIUM
#define ENOMEDIUM       ENODEV
#endif

typedef ssize_t (*pwrite64_t)(int,const void *,size_t,off64_t);
/*
 * Symbols from growisofs_mmc.cpp
 */
/*
 * These might terminate the program if error appears fatal.
 * The return value is therefore is always sane and suitable
 * for assignment.
 */
int        get_mmc_profile	(void *fd);
int        plusminus_r_C_parm	(void *fd,char *C_parm);
pwrite64_t poor_mans_setup	(void *fd,off64_t leadout);
/*
 * These never terminate the program.
 * Pay attention to the return value.
 */
int        media_reload		(char *file,struct stat *ref);
int        fumount		(int fd);
off64_t    get_capacity		(void *fd);
int        poor_man_rewritable	(void *fd,void *buf);

/* simplified */
struct iso_primary_descriptor {
    unsigned char type	[1];
    unsigned char id	[5];
    unsigned char void1	[80-5-1];
    unsigned char volume_space_size [8];
    unsigned char void2	[2048-80-8];
};

#define CD_BLOCK	((off64_t)2048)
#define VOLDESC_OFF	16
#define DVD_BLOCK	(32*1024)

static unsigned int from_733 (unsigned char *s)
{ unsigned int ret=0;
    ret |= s[0];
    ret |= s[1]<<8;
    ret |= s[2]<<16;
    ret |= s[3]<<24;
  return ret;
}

static void to_733 (unsigned char *s,unsigned int val)
{   s[0] = (val)     & 0xFF;
    s[1] = (val>>8)  & 0xFF;
    s[2] = (val>>16) & 0xFF;
    s[3] = (val>>24) & 0xFF;
    s[4] = (val>>24) & 0xFF;
    s[5] = (val>>16) & 0xFF;
    s[6] = (val>>8)  & 0xFF;
    s[7] = (val)     & 0xFF;
}

static pwrite64_t pwrite64_method = pwrite64;
/*
 * Page-aligned 2*DVD_BLOCK=64KB large buffer, second 32KB contain
 * "next" session volume descriptors that were written in the beginning
 * of current recording...
 */
static char *the_buffer;
/*
 * id_fd is passed to mkisofs, out_fd - to pwrite and ioctl_fd - to ioctl.
 */
static int    in_fd=-1,out_fd=-1;

#ifndef INVALID_HANDLE
#define INVALID_HANDLE ((void *)-1)
#endif
static void   *ioctl_handle=INVALID_HANDLE;
#define ioctl_fd ((long)ioctl_handle)

static int	poor_man=-1, zero_image=0, quiet=1,
		overburn=0, no_tty_check=0, dry_run=0, dao_size=0,
		no_4gb_check=0, layer_break=0;

int    dvd_compat=0, test_write=0, no_reload=0, mmc_profile=0, wrvfy=0, lazy=1;
double speed_factor=0.0;
char  *ioctl_device;
int    _argc;
char **_argv;

#if defined(__linux)

#include <linux/cdrom.h>
#include <sys/ioctl.h>
#include <linux/raw.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#define __STRICT_ANSI__ 
#ifndef _LINUX_WAIT_H
#define _LINUX_WAIT_H	/* linux headers are impaired */
#endif
#include <linux/capability.h>

char *find_raw_device(struct stat64 *sb)
{ int   i,rawctl;
  dev_t dev_major,dev_minor;
  char  *ret=NULL;
  struct raw_config_request req;
  static char rawdevname[24] = "";

    if (!S_ISBLK(sb->st_mode)) return NULL;

    dev_major = major (sb->st_rdev);
    dev_minor = minor (sb->st_rdev);

    if ((rawctl=open ("/dev/rawctl",O_RDONLY)) < 0) return NULL;

    for (i=1;i<256;i++)
    {	req.raw_minor = i;
	if (ioctl(rawctl,RAW_GETBIND,&req) < 0) break;
	if (req.block_major == dev_major && req.block_minor == dev_minor)
	{   sprintf (rawdevname,"/dev/raw/raw%d",i);
	    ret = rawdevname;
	    break;
	}
    }
    close (rawctl);

  return ret;
}

char *setup_fds (char *device)
{ char *odevice;
  uid_t uid=getuid();
  struct stat64 sb,sc;
  int fd;

    /*
     * I ignore return values from set{re}uid calls because if
     * they fail we have no privileges to care about and should
     * just proceed anyway...
     */
#if 0
#define GET_REAL
    /*
     * Get real, but preserve saved uid. I count that user is
     * logged on console and therefore owns the 'device' [this
     * is a PAM's resposibility to arrange].
     */
    setreuid ((uid_t)-1,uid);
#else
    /*
     * More "traditional" set-root-uid behaviour. I assume that if
     * administrator has installed growisofs set-root-uid, then
     * [s]he consciously wanted to grant access to /dev/scdN to
     * everybody, not just console user.
     */
#endif

    if ((in_fd = open64 (device,O_RDONLY)) < 0)
	if (!(errno == ENOMEDIUM || errno == EMEDIUMTYPE) ||
	    (in_fd = open64 (device,O_RDONLY|O_NONBLOCK)) < 0)
	    fprintf (stderr,":-( unable to open64(\"%s\",O_RDONLY): ",device),
	    perror (NULL), exit (FATAL_START(errno));

#ifdef GET_REAL
    setreuid ((uid_t)-1,uid);
#endif

    if (fstat64(in_fd,&sb) < 0)
	fprintf (stderr,":-( unable to stat64(\"%s\"): ",device),
	perror (NULL), exit (FATAL_START(errno));

    if (!S_ISBLK(sb.st_mode))
    {	setuid(uid);		/* drop all privileges	*/
	close (in_fd);		/* reopen as mortal	*/
	if ((in_fd = open64 (device,O_RDONLY)) < 0)
	    fprintf (stderr,":-( unable to re-open64(\"%s\",O_RDONLY): ",
			    device),
	    perror (NULL), exit (FATAL_START(errno));
      open_rw:
	if ((out_fd = open64 (device,O_RDWR)) < 0)
	    fprintf (stderr,":-( unable to open64(\"%s\",O_RDWR): ",device),
	    perror (NULL), exit (FATAL_START(errno));
	if (fstat64(out_fd,&sc) < 0)
	    fprintf (stderr,":-( unable to stat64(\"%s\"): ",device),
	    perror (NULL), exit (FATAL_START(errno));
	if (sb.st_dev!=sc.st_dev || sb.st_ino!=sc.st_ino)
	    fprintf (stderr,":-( %s: race condition detected!\n",device),
	    exit(FATAL_START(EPERM));
      opened_rw:
	poor_man = 0;
	if (ioctl_handle!=INVALID_HANDLE)
	    close (ioctl_fd), ioctl_handle = INVALID_HANDLE;
	setuid (uid);		/* drop all privileges */
	return device;
    }

    /*
     * O_RDWR is expected to provide for none set-root-uid
     * execution under Linux kernel 2.6[.8]. If it fails
     * [as under 2.6.8], CAP_SYS_RAWIO in combination
     * with set-root-uid should fill in for the kernel
     * deficiency...
     */
    if ((fd = open64 (device,O_RDWR|O_NONBLOCK)) >= 0)
	ioctl_handle=(void *)(long)fd;
    else
	ioctl_handle=(void *)(long)dup(in_fd);

    /*
     * get_mmc_profile terminates the program if ioctl_handle is
     * not an MMC unit...
     */
    mmc_profile = get_mmc_profile (ioctl_handle);

    /* Consume media_changed flag */
    if (ioctl (ioctl_fd,CDROM_MEDIA_CHANGED,CDSL_CURRENT) < 0)
    {	fprintf (stderr,":-( %s: CD_ROM_MEDIA_CHANGED ioctl failed: ",
			device),
	perror (NULL), exit (FATAL_START(errno));
    }

    switch (mmc_profile&0xFFFF)
    {	case 0x11:	/* DVD-R			*/
	case 0x13:	/* DVD-RW Restricted Overwrite	*/
	case 0x14:	/* DVD-RW Sequential		*/
	case 0x15:	/* DVD-R Dual Layer Sequential	*/
	case 0x16:	/* DVD-R Dual Layer Jump	*/
	case 0x1B:	/* DVD+R			*/
	case 0x2B:	/* DVD+R Double Layer		*/
	open_poor_man:
	    poor_man = 1;
	    out_fd = dup(ioctl_fd);
#if defined(PR_SET_KEEPCAPS)
	    if (prctl(PR_SET_KEEPCAPS,1) == 0) do {
#if defined(_LINUX_CAPABILITY_VERSION) && defined(CAP_SYS_RAWIO) && defined(SYS_capset)
		struct __user_cap_header_struct	h;
		struct __user_cap_data_struct	d;

		h.version	= _LINUX_CAPABILITY_VERSION;
		h.pid		= 0;
		d.effective	= 0;
		d.permitted	= 1<<CAP_SYS_RAWIO;
		d.inheritable	= 0;
		if (syscall(SYS_capset,&h,&d) < 0) break;

		/* drop all privileges & effective capabilities */
		/* setuid(uid) seems to preserve saved uid
		   after dropping CAP_SETUID above... */
		setresuid (uid,uid,uid);

		d.effective	= 1<<CAP_SYS_RAWIO;
		if (syscall(SYS_capset,&h,&d) < 0) break;
#endif
	    } while (0);
#endif
	    setuid (uid);	/* drop all privileges	*/
	    return ioctl_device=device;
	case 0x1A:	/* DVD+RW			*/
	case 0x12:	/* DVD-RAM			*/
	    if (poor_man>0) goto open_poor_man;
	    break;
	default:
	    fprintf (stderr,":-( %s: media is not recognized as "
			    "recordable DVD: %X\n",device,mmc_profile);
	    exit (FATAL_START(EMEDIUMTYPE));
    }

    /*
     * Attempt to locate /dev/raw/raw*
     */
#ifdef GET_REAL
#undef GET_REAL
    setreuid((uid_t)-1,0);	/* get root for a /dev/raw sake */
#endif
    if ((odevice=find_raw_device (&sb)))	/* /dev/raw */
    {	if ((out_fd=open64 (odevice,O_RDWR)) < 0)
	{   if (errno == EROFS)	/* must be unpatched kernel */
		goto open_poor_man;
	    else
		fprintf (stderr,":-( unable to open64(\"%s\",O_RDWR): ",
				odevice),
		perror (NULL), exit (FATAL_START(errno));
	}
	device=odevice;
	goto opened_rw;
    }
    if ((mmc_profile&0xFFFF) == 0x12)	goto open_rw;	/* DVD-RAM */
    else				goto open_poor_man;
}

#elif defined(__OpenBSD__) || defined(__NetBSD__)

char *setup_fds (char *device)
{ uid_t uid=getuid();
  struct stat sb,sc;

    /*
     * We might be entering as euid=root!
     */
    if ((in_fd = open (device,O_RDONLY)) < 0)
	fprintf (stderr,":-( unable to open(\"%s\",O_RDONLY): ",device),
	perror (NULL), exit (FATAL_START(errno));

    if (fstat (in_fd,&sb) < 0)
	fprintf (stderr,":-( unable to stat(\"%s\"): ",device),
	perror (NULL), exit (FATAL_START(errno));

    if (!S_ISCHR(sb.st_mode))
    {	if (S_ISBLK(sb.st_mode) && !strncmp (device,"/dev/cd",7))
	{   fprintf (stderr,":-) you most likely want to use /dev/r%s instead!\n",
			    device+5);
	    if (isatty(0) && !dry_run) poll(NULL,0,5000);
	}
	setuid(uid);	/* drop all privileges	*/
	close (in_fd);	/* reopen as mortal	*/
	if ((in_fd = open (device,O_RDONLY)) < 0)
	    fprintf (stderr,":-( unable to reopen(\"%s\",O_RDONLY): ",device),
	    perror (NULL), exit (FATAL_START(errno));
      open_rw:
	if ((out_fd = open (device,O_RDWR)) < 0)
	    fprintf (stderr,":-( unable to open(\"%s\",O_RDWR): ",device),
	    perror (NULL), exit (FATAL_START(errno));
      opened_rw:
	poor_man = 0;
	close (ioctl_fd);
	ioctl_handle = INVALID_HANDLE;
	setuid (uid);	/* drop all privileges	*/
	return device;
    }

    /*
     * Still as euid=root! But note that get_mmc_profile makes sure it's
     * an MMC device, as it terminates the program if the unit doesn't
     * reply to GET CONFIGURATON. In combination with following switch
     * this means that if installed set-root-uid, growisofs grants
     * access to DVD burner(s), but not to any other device.
     */
    ioctl_handle = (void *)(long)open (device,O_RDWR); /* O_RDWR is a must for SCIOCCOMMAND */
    if (ioctl_handle == INVALID_HANDLE)
    {	fprintf (stderr,":-( unable to open(\"%s\",O_RDWR): ",device),
	perror (NULL);
	if (errno == EBUSY)
	    fprintf (stderr,"    is its block counterpart mounted?\n");
	exit (FATAL_START(errno));
    }
    if (fstat(ioctl_fd,&sc) < 0)
	fprintf (stderr,":-( unable to stat(\"%s\"): ",device),
	perror (NULL), exit (FATAL_START(errno));
    if (sb.st_dev!=sc.st_dev || sb.st_ino!=sc.st_ino)
	fprintf (stderr,":-( %s: race condition detected!\n",device),
	exit(FATAL_START(EPERM));

    mmc_profile = get_mmc_profile (ioctl_handle);
    switch (mmc_profile&0xFFFF)
    {	case 0x11:	/* DVD-R			*/
	case 0x13:	/* DVD-RW Restricted Overwrite	*/
	case 0x14:	/* DVD-RW Sequential		*/
	case 0x15:	/* DVD-R Dual Layer Sequential	*/
	case 0x16:	/* DVD-R Dula Layer Jump	*/
	case 0x1A:	/* DVD+RW			*/
	case 0x1B:	/* DVD+R			*/
	case 0x2B:	/* DVD+R Double Layer		*/
	open_poor_man:
	    poor_man = 1;
	    out_fd = dup(ioctl_fd);
	    setuid (uid);	/* drop all privileges	*/
	    return ioctl_device=device;
	case 0x12:	/* DVD-RAM			*/
	    if (poor_man>0) goto open_poor_man;
	    out_fd = dup(ioctl_fd);
	    goto opened_rw;
	default:
	    fprintf (stderr,":-( %s: media is not recognized as "
			    "recordable DVD: %X\n",device,mmc_profile);
	    exit (FATAL_START(EMEDIUMTYPE));
    }
    /* not actually reached */
    setuid(uid);
    goto open_rw;
}

#elif defined(__FreeBSD__)

#include <sys/cdio.h>
#include <camlib.h>
#include <cam/scsi/scsi_pass.h>

#undef	ioctl_fd
#define	ioctl_fd (((struct cam_device *)ioctl_handle)->fd)

#define PASS_STDIN_TO_MKISOFS

char *setup_fds (char *device)
{ uid_t		uid=getuid();
  struct stat	sb,sc;
  union ccb	ccb;
  char		pass[32]; /* periph_name is 16 chars long */
  struct cam_device *cam;
  int		once=1;

    /*
     * We might be entering as euid=root!
     */
    if ((in_fd = open (device,O_RDONLY)) < 0)
	fprintf (stderr,":-( unable to open(\"%s\",O_RDONLY): ",device),
	perror (NULL), exit (FATAL_START(errno));

    if (fstat (in_fd,&sb) < 0)
	fprintf (stderr,":-( unable to stat(\"%s\"): ",device),
	perror (NULL), exit (FATAL_START(errno));

    if (!S_ISCHR(sb.st_mode))
    {	setuid(uid);	/* drop all privileges	*/
	close (in_fd);	/* reopen as mortal	*/
	if ((in_fd = open (device,O_RDONLY)) < 0)
	    fprintf (stderr,":-( unable to reopen(\"%s\",O_RDONLY): ",device),
	    perror (NULL), exit (FATAL_START(errno));
      open_rw:
	if ((out_fd = open (device,O_RDWR)) < 0)
	    fprintf (stderr,":-( unable to open(\"%s\",O_RDWR): ",device),
	    perror (NULL), exit (FATAL_START(errno));
	if (fstat(out_fd,&sc) < 0)
	    fprintf (stderr,":-( unable to stat(\"%s\"): ",device),
	    perror (NULL), exit (FATAL_START(errno));
	if (sb.st_dev!=sc.st_dev || sb.st_ino!=sc.st_ino)
	    fprintf (stderr,":-( %s: race condition detected!\n",device),
	    exit(FATAL_START(EPERM));
      opened_rw:
	poor_man = 0;
	if (ioctl_handle && ioctl_handle != INVALID_HANDLE)
	    cam_close_device (ioctl_handle);
	ioctl_handle = INVALID_HANDLE;
	setuid (uid);	/* drop all privileges	*/
	return device;
    }

    /*
     * Still as euid=root! But note that get_mmc_profile makes sure it's
     * an MMC device, as it terminates the program if the unit doesn't
     * reply to GET CONFIGURATON. In combination with following switch
     * this means that if installed set-root-uid, growisofs grants
     * access to DVD burner(s), but not to any other device.
     */

    for (once=1;1;once--)
    {	memset (&ccb,0,sizeof(ccb));
	ccb.ccb_h.func_code = XPT_GDEVLIST;
	if (ioctl (in_fd,CAMGETPASSTHRU,&ccb) < 0)
	{   if (errno==ENXIO && once)
	    {	if (ioctl (in_fd,CDIOCCLOSE)==0) continue;	}
	    fprintf (stderr,":-( unable to CAMGETPASSTHRU for %s: ",device),
	    perror (NULL), exit (FATAL_START(errno));
	}
	break;
    }

    sprintf (pass,"/dev/%.15s%u",ccb.cgdl.periph_name,ccb.cgdl.unit_number);
    cam = cam_open_pass (pass,O_RDWR,NULL);
    if (cam == NULL)
    {	fprintf (stderr,":-( unable to cam_open_pass(\"%s\",O_RDWR): ",pass),
	perror (NULL);
	exit (FATAL_START(errno));
    }

    ioctl_handle = (void *)cam;

    mmc_profile = get_mmc_profile (ioctl_handle);
    switch (mmc_profile&0xFFFF)
    {	case 0x11:	/* DVD-R			*/
	case 0x13:	/* DVD-RW Restricted Overwrite	*/
	case 0x14:	/* DVD-RW Sequential		*/
	case 0x15:	/* DVD-R Dual Layer Sequential	*/
	case 0x16:	/* DVD-R Dual Layer Jump	*/
	case 0x1A:	/* DVD+RW			*/
	case 0x1B:	/* DVD+R			*/
	case 0x2B:	/* DVD+R Double Layer		*/
	open_poor_man:
	    poor_man = 1;
	    out_fd = dup(in_fd);	/* it's ignored in poor_man ... */
	    setuid (uid);	/* drop all privileges	*/
	    return ioctl_device=cam->device_path;
	case 0x12:	/* DVD-RAM			*/
	    if (poor_man>0) goto open_poor_man;
	    break;
	default:
	    fprintf (stderr,":-( %s: media is not recognized as "
			    "recordable DVD: %X\n",device,mmc_profile);
	    exit (FATAL_START(EMEDIUMTYPE));
    }
    setuid(uid);
    goto open_rw;
}

#elif defined(__sun) || defined(sun)

#include <volmgt.h>
#include <sys/cdio.h>

char *setup_fds (char *device)
{ uid_t uid=getuid();
  struct stat64 sb,sc;
  int v;

    if ((v=volmgt_running()))
    { char *file=NULL,*volname;

	/*
	 * I leak some memory here, but I don't care...
	 */
	if ((volname=volmgt_symname (device)))
	    file=media_findname (volname);
	else
	    file=media_findname (device);

	if (file) device=file;
	else      v=0;
    }

    /*
     * We might be entering as euid=root!
     */
    if ((in_fd = open64 (device,O_RDONLY)) < 0)
	if (errno != ENXIO ||
	    (in_fd = open64 (device,O_RDONLY|O_NONBLOCK)) < 0)
	    fprintf (stderr,":-( unable to open(\"%s\",O_RDONLY): ",device),
	    perror (NULL), exit (FATAL_START(errno));

    if (fstat64 (in_fd,&sb) < 0)
	fprintf (stderr,":-( unable to stat(\"%s\"): ",device),
	perror (NULL), exit (FATAL_START(errno));

    if (!S_ISCHR(sb.st_mode))
    { char *s;
	if (S_ISBLK(sb.st_mode) && (s=strstr (device,"/dsk/")))
	{   fprintf (stderr,":-) you most likely want to use %.*s/r%s instead!\n",
			    (int)(s-device),device,s+1);
	    if (isatty(0) && !dry_run) poll(NULL,0,5000);
	}
	setuid(uid);	/* drop all privileges	*/
	close (in_fd);	/* reopen as mortal	*/
	if ((in_fd = open64 (device,O_RDONLY)) < 0)
	    fprintf (stderr,":-( unable to reopen(\"%s\",O_RDONLY): ",device),
	    perror (NULL), exit (FATAL_START(errno));
      open_rw:
	if ((out_fd = open64 (device,O_RDWR)) < 0)
	    fprintf (stderr,":-( unable to open(\"%s\",O_RDWR): ",device),
	    perror (NULL), exit (FATAL_START(errno));
	if (fstat64(out_fd,&sc) < 0)
	    fprintf (stderr,":-( unable to stat(\"%s\"): ",device),
	    perror (NULL), exit (FATAL_START(errno));
	if (sb.st_dev!=sc.st_dev || sb.st_ino!=sc.st_ino)
	    fprintf (stderr,":-( %s: race condition detected!\n",device),
	    exit(FATAL_START(EPERM));
	poor_man = 0;
	close (ioctl_fd);
	ioctl_handle = INVALID_HANDLE;
	setuid (uid);	/* drop all privileges	*/
	return device;
    }

    /*
     * Still as euid=root! But note that get_mmc_profile makes sure it's
     * an MMC device, as it terminates the program if the unit doesn't
     * reply to GET CONFIGURATON. In combination with following switch
     * this means that if installed set-root-uid, growisofs grants
     * access to DVD burner(s), but not to any other device.
     */
    ioctl_handle = (void *)(long)dup (in_fd);

#if 0
    if (ioctl(ioctl_handle,CDROMSTART)<0 && errno==ENXIO)
	media_load(ioctl_handle);
#endif

    mmc_profile = get_mmc_profile (ioctl_handle);
    switch (mmc_profile&0xFFFF)
    {	case 0x11:	/* DVD-R			*/
	case 0x12:	/* DVD-RAM			*/
	case 0x13:	/* DVD-RW Restricted Overwrite	*/
	case 0x14:	/* DVD-RW Sequential		*/
	case 0x15:	/* DVD-R Dual Layer Sequential	*/
	case 0x16:	/* DVD-R Dual Layer Jump	*/
	case 0x1A:	/* DVD+RW			*/
	case 0x1B:	/* DVD+R			*/
	case 0x2B:	/* DVD+R Double Layer		*/
	    poor_man = 1;
	    out_fd = dup(ioctl_fd);
#if 0	    /* 'man uscsi' maintains that root privileges are required upon
	     * issue of USCSI ioctl, we therefore can't drop them...
	     */
	    setuid (uid);	/* drop all privileges	*/
#endif
	    return ioctl_device=device;
	default:
	    fprintf (stderr,":-( %s: media is not recognized as "
			    "recordable DVD: %X\n",device,mmc_profile);
	    exit (FATAL_START(EMEDIUMTYPE));
    }
    setuid(uid);
    goto open_rw;
}

#elif defined(__hpux)

#ifdef SCTL_MAJOR
#  define SCTL_SANITY_CHECK SCTL_MAJOR-1
#  if SCTL_SANITY_CHECK<=0
#    undef SCTL_MAJOR
#  endif
#  undef SCTL_SANITY_CHECK
#endif

#ifndef SCTL_MAJOR
#error "SCTL_MAJOR is undefined or not sane."
#endif

#include <sys/mknod.h>
#include <sys/diskio.h>

#define seteuid(x)	setreuid((uid_t)-1,x)

#if 1
#define CANNOT_PASS_DEV_FD_N_TO_MKISOFS
#elif 0
--- ./multi.c.orig	Wed Dec 25 15:15:24 2002
+++ ./multi.c	Tue Nov 11 17:12:27 2003
@@ -1067,3 +1067,13 @@
 open_merge_image(path)
 	char	*path;
 {
+	int fd;
+
+	if (sscanf (path,"/dev/fd/%u",&fd) == 1) {
+		int fdd = dup(fd);	/* validate file descriptor */
+		if (fdd < 0) return -1;
+		close (fdd);
+		in_image = fdopen (fd,"rb");
+		return in_image ? 0 : -1;
+	}
+
#endif

#ifdef CANNOT_PASS_DEV_FD_N_TO_MKISOFS
char *get_M_parm (int fd, char *device)
{ struct stat sb;
  dev_t m;
  char *ret=device;
  static char ctl[16];

    if (fstat (fd,&sb)==0 && S_ISCHR(sb.st_mode))
    {	m = minor (sb.st_rdev);
	sprintf (ctl,"%d,%d,%d",(m>>16)&0xFF,(m>>12)&0xF,(m>>8)&0xF);
	ret = ctl;
    }

  return ret;
}
#endif

char *setup_fds (char *device)
{ uid_t uid=getuid();
  struct stat64 sb,sc;
  dev_t  m;
  static char rscsi [32];
  disk_describe_type ddt;

    /*
     * We might be entering as euid=root!
     */
    if ((in_fd = open64 (device,O_RDONLY)) < 0)
	fprintf (stderr,":-( unable to open(\"%s\",O_RDONLY): ",device),
	perror (NULL), exit (FATAL_START(errno));

    if (fstat64 (in_fd,&sb) < 0)
	fprintf (stderr,":-( unable to stat(\"%s\"): ",device),
	perror (NULL), exit (FATAL_START(errno));

    if (!S_ISCHR(sb.st_mode))
    { char *s;
	if (S_ISBLK(sb.st_mode) && (s=strstr (device,"/dsk/")))
	{   fprintf (stderr,":-) you most likely want to use %.*s/r%s instead!\n",
			    (int)(s-device),device,s+1);
	    if (isatty(0) && !dry_run) poll(NULL,0,5000);
	}
	setuid(uid);	/* drop all privileges	*/
	close (in_fd);	/* reopen as mortal	*/
	if ((in_fd = open64 (device,O_RDONLY)) < 0)
	    fprintf (stderr,":-( unable to reopen(\"%s\",O_RDONLY): ",device),
	    perror (NULL), exit (FATAL_START(errno));
      open_rw:
	if ((out_fd = open64 (device,O_RDWR)) < 0)
	    fprintf (stderr,":-( unable to open(\"%s\",O_RDWR): ",device),
	    perror (NULL), exit (FATAL_START(errno));
	if (fstat64(out_fd,&sc) < 0)
	    fprintf (stderr,":-( unable to stat(\"%s\"): ",device),
	    perror (NULL), exit (FATAL_START(errno));
	if (sb.st_dev!=sc.st_dev || sb.st_ino!=sc.st_ino)
	    fprintf (stderr,":-( %s: race condition detected!\n",device),
	    exit(FATAL_START(EPERM));
	poor_man = 0;
	close (ioctl_fd);
	ioctl_handle = INVALID_HANDLE;
	setuid (uid);	/* drop all privileges	*/
	return device;
    }

    /*
     * Still as euid=root! But note that get_mmc_profile makes sure it's
     * an MMC device, as it terminates the program if the unit doesn't
     * reply to GET CONFIGURATON. In combination with following switch
     * this means that if installed set-root-uid, growisofs grants
     * access to DVD burner(s), but not to any other device.
     */

    m=minor(sb.st_rdev);

    /* Make sure user isn't using /dev/rscsi/cXtYlZ... */
#if 1
    if (ioctl (in_fd,DIOC_DESCRIBE,&ddt) != 0)
#else
    if (major(sb.st_rdev) == SCTL_MAJOR)
#endif
	fprintf (stderr,":-( stick to /dev/rdsk/c%ut%u%c%x!\n",
			(m>>16)&0xFF,(m>>12)&0xF,'d',(m>>8)&0xF),
	exit(FATAL_START(EINVAL));

    /*
     * Even though /dev/rdsk/cXtYdZ accepts SIOC_IO as well, we have to
     * use /dev/rscsi/cXtYlZ for pass-through access in order to avoid
     * command replay by upper "class" driver...
     */
    sprintf (rscsi,"/dev/rscsi/c%ut%u%c%x",
		    (m>>16)&0xFF,(m>>12)&0xF,'l',(m>>8)&0xF);
    ioctl_handle = (void *)(long)open64 (rscsi,O_RDONLY);
    if (ioctl_handle == INVALID_HANDLE)
    {	fprintf (stderr,":-( unable to open(\"%s\",O_RDONLY): ",rscsi),
	perror (NULL);
	if (errno == ENOENT)
	    fprintf (stderr,":-! consider "
			    "'mknod %s c %d 0x%06x; chmod 0600 %s'\n",
			    rscsi,SCTL_MAJOR,(m&0xFFFF00)|2,rscsi);
	exit (FATAL_START(errno));
    }
    if (fstat64 (ioctl_fd,&sc) < 0)
	fprintf (stderr,":-( unable to stat(\"%s\"): ",rscsi),
	perror (NULL), exit (FATAL_START(errno));
    /* Make sure we land on same SCSI ID... */
    if ((m&0xFFFF00) != (minor(sc.st_rdev)&0xFFFF00))
	fprintf (stderr,":-( SCSI ID mismatch: %06x!=%06x\n",
			m,minor(sc.st_rdev)),
	exit(FATAL_START(EPERM));

    mmc_profile = get_mmc_profile (ioctl_handle);
    switch (mmc_profile&0xFFFF)
    {	case 0x11:	/* DVD-R			*/
	case 0x12:	/* DVD-RAM			*/
	case 0x13:	/* DVD-RW Restricted Overwrite	*/
	case 0x14:	/* DVD-RW Sequential		*/
	case 0x15:	/* DVD-R Dual Layer Sequential	*/
	case 0x16:	/* DVD-R Dual Layer Jump	*/
	case 0x1A:	/* DVD+RW			*/
	case 0x1B:	/* DVD+R			*/
	case 0x2B:	/* DVD+R Double Layer		*/
	    poor_man = 1;
	    out_fd = dup(ioctl_fd);
#if 0	    /* HP-UX requires root privileges upon SIOC_IO ioctl */
	    setuid (uid);	/* drop all privileges	*/
#endif
	    return ioctl_device=rscsi;
	default:
	    fprintf (stderr,":-( %s: media is not recognized as "
			    "recordable DVD: %X\n",device,mmc_profile);
	    exit (FATAL_START(EMEDIUMTYPE));
    }
    setuid(uid);
    goto open_rw;
}

/*
 * PA-RISC HP-UX doesn't have /dev/zero:-(
 */
static fd_set _dev_zero;

static int open64_zero(const char *pathname, int flags)
{ int fd;
    if (strcmp(pathname,"/dev/zero"))
	return open64 (pathname,flags);
    else if ((fd=open ("/dev/null",flags)) >= 0)
	FD_SET (fd,&_dev_zero);
  return fd;    
}

static ssize_t read_zero (int fd, void *buf, size_t count)
{   if (!FD_ISSET(fd,&_dev_zero))	return read (fd,buf,count);
    memset (buf,0,count);		return count;
}

static int close_zero (int fd)
{ int ret=close (fd);
    if (ret>=0 && FD_ISSET(fd,&_dev_zero))	FD_CLR (fd,&_dev_zero);
  return ret;
}

static int dup2_zero (int oldfd, int newfd)
{ int ret;
    ret = dup2 (oldfd,newfd);
    if (ret >= 0)
    {	if (FD_ISSET(oldfd,&_dev_zero))	FD_SET (ret,&_dev_zero);
	else				FD_CLR (ret,&_dev_zero);
    }
  return ret;
}
#define open64	open64_zero
#define read	read_zero
#define close	close_zero
#define dup2	dup2_zero

#elif defined(__sgi)

#include <sys/attributes.h>
#include <sys/param.h>
#include <mediad.h>

char *setup_fds (char *device)
{ uid_t uid=getuid();
  struct stat64 sb,sc;
  char	hw_path[MAXPATHLEN],*s;
  int	hw_len=sizeof(hw_path)-1;
  int	bus=0,tgt=4,lun=0;	/* default config for O2 */
  static char rscsi[64];

    /*
     * We might be entering as euid=root!
     */
    if ((in_fd = open64 (device,O_RDONLY)) < 0)
    {	if (errno==EIO)	goto tray_might_be_open;
	fprintf (stderr,":-( unable to open(\"%s\",O_RDONLY): ",device),
	perror (NULL), exit (FATAL_START(errno));
    }

    if (fstat64 (in_fd,&sb) < 0)
	fprintf (stderr,":-( unable to stat(\"%s\"): ",device),
	perror (NULL), exit (FATAL_START(errno));

    if (!S_ISCHR(sb.st_mode))
    {	if (S_ISBLK(sb.st_mode) && !strncmp (device,"/dev/dsk",7))
	{   fprintf (stderr,":-) you most likely want to use "
			    "/dev/r%s instead!\n",device+5);
	    if (isatty(0) && !dry_run) poll(NULL,0,5000);
	}
      open_rw:
	setuid(uid);	/* drop all privileges	*/
	close (in_fd);	/* reopen as mortal	*/
	if ((in_fd = open64 (device,O_RDONLY)) < 0)
	    fprintf (stderr,":-( unable to reopen(\"%s\",O_RDONLY): ",device),
	    perror (NULL), exit (FATAL_START(errno));
	if ((out_fd = open64 (device,O_RDWR)) < 0)
	    fprintf (stderr,":-( unable to open(\"%s\",O_RDWR): ",device),
	    perror (NULL), exit (FATAL_START(errno));
      opened_rw:
	poor_man = 0;
	close (ioctl_fd);
	ioctl_handle = INVALID_HANDLE;
	setuid (uid);	/* drop all privileges	*/
	return device;
    }

    /*
     * Still as euid=root! But note that get_mmc_profile makes sure it's
     * an MMC device, as it terminates the program if the unit doesn't
     * reply to GET CONFIGURATON. In combination with following switch
     * this means that if installed set-root-uid, growisofs grants
     * access to DVD burner(s), but not to any other device.
     */

  tray_might_be_open:

    if ((in_fd<0) ? attr_get (device,"_devname",hw_path,&hw_len,0) :
		    attr_getf (in_fd,"_devname",hw_path,&hw_len,0) )
	fprintf (stderr,":-( unable to obtain hw_path for \"%s\": ",device),
	perror (NULL), exit (FATAL_START(errno));
    if (hw_len>=sizeof(hw_path)) hw_len=sizeof(hw_path)-1; /* paranoia */
    hw_path[hw_len]='\0';

    if ((s=strstr(hw_path,"/scsi_ctlr/")))
	sscanf (s,"/scsi_ctlr/%d/target/%d/lun/%d/",&bus,&tgt,&lun);

    /* Make sure user is using /dev/rdsk/dksXdYlZvol... */
    if ((s=strstr(hw_path,"/disk/volume/char"))==NULL)
    {   if (lun)
	    fprintf (stderr,":-( stick to /dev/rdsk/dks%dd%dl%dvol!\n",
			    bus,tgt,lun);
	else
	    fprintf (stderr,":-( stick to /dev/rdsk/dks%dd%dvol!\n",
			    bus,tgt);
	exit(FATAL_START(EINVAL));
    }

    memcpy (s,"/scsi",6);
    ioctl_handle = (void *)(long)open64 (hw_path,O_RDWR);
    if (ioctl_handle == INVALID_HANDLE)
	fprintf (stderr,":-( unable to open(\"%s\",O_RDWR): ",hw_path),
	perror (NULL), exit (FATAL_START(errno));
    if (fstat64 (ioctl_fd,&sc) < 0)
	fprintf (stderr,":-( unable to stat(\"%s\"): ",hw_path),
	perror (NULL), exit (FATAL_START(errno));
    memcpy (s,"/disk/volume/char",6);

    mmc_profile = get_mmc_profile (ioctl_handle);
    switch (mmc_profile&0xFFFF)
    {	case 0x11:	/* DVD-R			*/
	case 0x13:	/* DVD-RW Restricted Overwrite	*/
	case 0x14:	/* DVD-RW Sequential		*/
	case 0x15:	/* DVD-R Dual Layer Sequential	*/
	case 0x16:	/* DVD-R Dual Layer Jump	*/
	case 0x1A:	/* DVD+RW			*/
	case 0x1B:	/* DVD+R			*/
	case 0x2B:	/* DVD+R Double Layer		*/
	open_poor_man:
	    poor_man = 1;
	    out_fd = dup (ioctl_fd);
	    if (in_fd<0 && (in_fd=open64 (hw_path,O_RDONLY))<0)
		/* hope for the best? */ ;
	    setuid (uid);	/* drop all privileges	*/
	    mediad_get_exclusiveuse (hw_path,"growisofs");
	    if (mediad_last_error ()==RMED_NOERROR)
		putenv ("MEDIAD_GOT_EXCLUSIVEUSE=");	/* kludge... */
	    sprintf (rscsi,"/dev/scsi/sc%dd%dl%d",bus,tgt,lun);
	    return ioctl_device=rscsi;	/* might be bogus... */
	case 0x12:	/* DVD-RAM			*/
	    /* Some of latest tentative IRIX releases seem to
	     * implement DVD-RAM writing at dksc level, but I'm
	     * not sure which one. So I just fall down to poor-man,
	     * at least for now... */
	    goto open_poor_man;
	    break;
	default:
	    fprintf (stderr,":-( %s: media is not recognized as "
			    "recordable DVD: %X\n",device,mmc_profile);
	    exit (FATAL_START(EMEDIUMTYPE));
    }
    /* not actually reached */
    setuid(uid);
    goto open_rw;
}

#else
#error "Unsupported OS"
#endif

int setup_C_parm (char *C_parm,struct iso_primary_descriptor *descr)
{ int next_session=-1,profile=mmc_profile&0xFFFF;

    if (!poor_man || profile==0x1A || profile==0x13 || profile==0x12)
    {	next_session = from_733(descr->volume_space_size);
	/* pad to closest 32K boundary */
	next_session += 15;
	next_session /= 16;
	next_session *= 16;
	sprintf (C_parm,"16,%u",next_session);
    }
    else if (profile==0x2B || profile==0x1B || profile==0x11 || profile==0x14)
	next_session=plusminus_r_C_parm (ioctl_handle,C_parm);

  return next_session;
}

int builtin_dd (int infd,int outfd,off64_t outoff)
{ char		*s;
  int		 n,fd;
  unsigned int	 off;
  struct stat64  sb;
  off64_t	 capacity=0,tracksize=0,startoff=outoff;
  pid_t		 pid=(pid_t)-1,ppid=getpid();
  volatile struct { time_t zero; off64_t current,final; } *progress;

    if ((fd=mkstemp (s=strdup("/tmp/dvd+rw.XXXXXX"))) < 0)
	fprintf (stderr,":-( unable to mkstemp(\"%s\")",s),
	exit(FATAL_START(errno));

    ftruncate(fd,(off_t)sizeof(*progress));
    unlink(s), free(s);

    progress = (void *)mmap (NULL,sizeof(*progress),PROT_READ|PROT_WRITE,
			MAP_SHARED,fd,(off_t)0);
    close (fd);
    if (progress == MAP_FAILED)
	fprintf (stderr,":-( unable to anonymously mmap %lu?\n",
			sizeof(*progress)),
	perror (NULL), exit(FATAL_START(errno));

    if (fstat64 (infd,&sb))
	perror (":-( unable to fstat64"), exit(FATAL_START(errno));

    if (ioctl_handle!=INVALID_HANDLE)
	capacity = get_capacity (ioctl_handle);

    progress->zero=0;
    progress->current=outoff;
    if (dao_size || S_ISREG(sb.st_mode))
    {	tracksize = dao_size ? (dao_size*CD_BLOCK) : sb.st_size;
	progress->final=outoff+tracksize;
	if (capacity && progress->final > capacity)
	{   fprintf (stderr,":-( %s: %lld blocks are free, "
			    "%lld to be written!\n",
			    ioctl_device,
			    (capacity-outoff)/2048,tracksize/2048);
	    if (overburn)
		fprintf (stderr,":-! ignoring...\n");
	    else
		close(infd), close(outfd),
		exit (FATAL_START(ENOSPC));
	}
    }
    else progress->final=0;

    if (!dry_run && quiet<=0 && (pid=fork()) == 0)
    { double		ratio,velocity,slept;
      int		delta,nfirst=0;
      off64_t		lastcurrent=outoff,current;
      struct timeval	tv;

	close (infd);
	close (outfd);
	while (kill (ppid,0)==0)
	{   gettimeofday (&tv,NULL); slept =  tv.tv_usec/1e6+tv.tv_sec;
	    lastcurrent = progress->current;
	    poll (NULL,0,3333);
	    gettimeofday (&tv,NULL); slept -= tv.tv_usec/1e6+tv.tv_sec;
	    if (progress->zero==0 || !nfirst++) continue;
	    if ((current = progress->current) > outoff)
	    {	delta = time (NULL) - progress->zero;
		ratio = (double)(progress->final-outoff) /
			(double)(current-outoff);
		delta *= ratio - 1.0;
		velocity=(current-lastcurrent)/(-slept*1024*1385);
		fprintf (stdout,"%10lld/%lld (%4.1f%%) @%.1fx, "
				"remaining %d:%02d\n",
				current,progress->final,100.0/ratio,
				velocity,delta/60,delta%60);
		lastcurrent=current;
	    }
	    else
		fprintf (stdout,"%10lld/%lld (%4.1f%%) @0x, remaining ??:??\n",
				current,progress->final,0.0);
	    fflush (stdout);
	}
	exit(0); /* not [normally] reached */
    }

    /* suck in first 64K and examine ISO9660 Primary Descriptor if present */
    off = 0;
    while ((n=read (infd,the_buffer+off,2*DVD_BLOCK-off)) > 0)
    {	off += n;
	if (off == 2*DVD_BLOCK)
	{   if (!memcmp(the_buffer+DVD_BLOCK,"\1CD001",6))
	    { struct iso_primary_descriptor *descr;

		descr=(struct iso_primary_descriptor *)(the_buffer+DVD_BLOCK);

		if (!zero_image)
	        {   if (tracksize==0)
		    {	tracksize=from_733(descr->volume_space_size)*CD_BLOCK;
			if (capacity && (outoff+tracksize) > capacity)
		    	{   fprintf (stderr,":-( %s: %lld blocks are free, "
					    "%lld to be written\n",
					    ioctl_device,
					    (capacity-outoff)/2048,
					    tracksize/2048);
			    if (overburn)
				fprintf (stderr,":-! ignoring...\n");
			    else
			    {	n = -1; errno = FATAL_START(ENOSPC);
				goto out;
			    }
			}
		    }
		    /* else already checked for overburn condition */

		    /* layer_break is meaningful only for -Z recording */
		    if (layer_break>0 && !outoff)
		    {	if (tracksize > layer_break*CD_BLOCK*2)
			{   fprintf (stderr,":-( insane Layer Break position "
					    "with respect to dataset size\n");
			    n = -1; errno = FATAL_START(EINVAL);
			    goto out;
			}
			if (!progress->final) progress->final = tracksize;
			tracksize = layer_break*CD_BLOCK*2;
		    }
		}
		else if (capacity > outoff)
		{ int i=0;
		  unsigned int ts = (tracksize=capacity-outoff)/2048;

		    while (i<16 && descr->type[0] != (unsigned char)255)
			to_733 (descr->volume_space_size,ts),
			descr++, i++;
		}
		else
		{   fprintf (stderr,":-( capacity is undefined or insane?\n");
		    n = -1; errno = FATAL_START(EINVAL);/* ... or whatever */
		    goto out;
		}
	    }
	    else if (outoff && zero_image)
	    {	fprintf (stderr,":-( no volume descriptors found "
				"in previous session?\n");
		n = -1; errno = FATAL_START(ENOTDIR);	/* ... or whatever */
		goto out;
	    }

	    if (dry_run) close(infd), close(outfd), exit(0);

	    if (poor_man)
		/*
		 * See commentary section in growisofs_mmc.cpp for
		 * further details on poor_mans_setup
		 */
		pwrite64_method = poor_mans_setup (ioctl_handle,
						   outoff+tracksize);

	    if (!progress->final)
	    {	if (tracksize)	progress->final = outoff+tracksize;
		else		progress->final = capacity;
	    }
	    if (capacity && progress->final>capacity)
		progress->final = capacity;

	    progress->zero=time(NULL);

	    /* Write two 32KB chunks, not one 64KB... */
	    if ((n=(*pwrite64_method) (outfd,the_buffer,DVD_BLOCK,outoff))
			!= DVD_BLOCK)
	    {	if (n>0)	errno = FATAL_START(EIO);
		else if (n==0)	errno = FATAL_START(ENOSPC);
		n = -1;
		goto out;
	    }
	    outoff += DVD_BLOCK;
	    progress->current=outoff;

	    if ((n=(*pwrite64_method) (outfd,the_buffer+DVD_BLOCK,DVD_BLOCK,outoff))
			!= DVD_BLOCK)
	    {	if (n>0)	errno = EIO;
		else if (n==0)	errno = ENOSPC;
		n = -1;
		goto out;
	    }
	    outoff += DVD_BLOCK;
	    progress->current=outoff;

	    break;
	}
    }
    if (n<=0) goto out;

    /* yeah, really kludgy, shuffling file descriptor like that... */
    if (zero_image)
	close(infd), infd=open64 ("/dev/zero",O_RDONLY);

    off = 0;
    /*
     * From now on only the first 32K of the_buffer are used!
     */
    while ((n=read (infd,the_buffer+off,DVD_BLOCK-off)) > 0)
    {	off += n;
	if (off == DVD_BLOCK)
	{   off = 0;
	    if ((n=(*pwrite64_method) (outfd,the_buffer,DVD_BLOCK,outoff))
			!= DVD_BLOCK)
	    {	if (n>0)	errno = EIO;
		else if (n==0)	errno = ENOSPC;
		n = -1;
		break;
	    }
	    outoff += DVD_BLOCK;
	    progress->current=outoff;
	}
    }
    if (off)	/* pad to the closest 32K is needed */
    {	memset (the_buffer+off,0,DVD_BLOCK-off);
	if ((n=(*pwrite64_method) (outfd,the_buffer,DVD_BLOCK,outoff))
			!= DVD_BLOCK)
	{    if (n>0)		errno = EIO;
	     else if (n==0)	errno = ENOSPC;
	     n = -1;
	}
	outoff += DVD_BLOCK;
    }

    printf ("builtin_dd: %lld*2KB out @ average %.1fx1385KBps\n",
	    (outoff-startoff)/2048,
	    ((outoff-startoff)/(1024.0*1385.0))/(time(NULL)-progress->zero));
out:
    { int saved_errno=errno;

	if (dry_run)	    close(infd), close(outfd), exit(errno);
	if (pid!=(pid_t)-1) kill (pid,SIGTERM), waitpid (pid,NULL,0);

	errno = outoff ? saved_errno : (n--?saved_errno:FATAL_START(EIO));
    }

  return n;
}

void pipe_mkisofs_up (char *mkisofs_argv[],int infd,int outfd,off64_t outoff)
{ pid_t mkisofs_pid;
  int   fildes[2],ret,n;

    if (pipe (fildes) < 0)
        perror (":-( unable to create pipe"), exit(FATAL_START(errno));

    if ((mkisofs_pid=fork ()) == (pid_t)-1)
	perror (":-( unable to fork mkisofs"), exit(FATAL_START(errno));
    else if (mkisofs_pid == 0)
    {	dup2  (fildes[1],1);
	close (fildes[0]);
	close (fildes[1]);
	close (outfd);	/* redundant:-) */

#ifdef PASS_STDIN_TO_MKISOFS
	dup2(infd,0);
	close(infd);
	infd=0;
#endif
	if ((n=fcntl (infd,F_GETFD))<0) n=0;
	fcntl (infd,F_SETFD,n&~FD_CLOEXEC);
	/*
	 * If platform-specific setup_fds did not drop privileges,
	 * do it now. I ignore return value because if it fails,
	 * then privileges were dropped already.
	 */
	setuid(getuid());
	execvp (mkisofs_argv[0],mkisofs_argv);
	fprintf (stderr,":-( unable to execute %s: ",mkisofs_argv[0]),
	perror (NULL), exit (FATAL_START(errno));
    }

    close (fildes[1]);

    n=builtin_dd(fildes[0],outfd,outoff);

    if (n==0) /* mkisofs must have finished, consume the exit code */
    {	if ((waitpid (mkisofs_pid,&ret,0)) == -1)
	    perror (":-( waitpid failed"), exit (errno);

	if (!WIFEXITED(ret) || WEXITSTATUS(ret)!=0)
	    fprintf (stderr,":-( mkisofs has failed: %d\n",WEXITSTATUS(ret)),
	    exit (1);
    }
    else if (n<0)
    { int err = errno;
	errno = err&0x7F;	/* they might be passing FATAL_START */
	perror (":-( write failed"), exit (err);
    }
}

/*
 * This may not be larger than 32KB!
 */
#define MAX_IVDs	16
#define IVDs_SIZE	(sizeof(struct iso_primary_descriptor)*MAX_IVDs)

int main (int argc, char *argv[])
{ int  imgfd=-1;
  char *in_device=NULL,*out_device=NULL,*in_image=NULL,*env;
  char dev_found;
  int  i,n,warn_for_isofs=0;
  char **mkisofs_argv,C_parm[24],M_parm_[16],*M_parm=M_parm_;
  int  mkisofs_argc,growisofs_argc;
  int  next_session=-1,alleged_next_session=-1;
  unsigned int new_size;
  struct iso_primary_descriptor *descr;


  /*
   * ADDED
   * Try to change the priority
   */
  if( setpriority( PRIO_PROCESS, 0/*getpid()*/, -20 ) )
    fprintf( stderr, ":-( Could not set priority.\n" );

  /*
   * Drop root privileges
   */
  if (setuid(getuid()) < 0)
    fprintf( stderr, ":-( Could not set back effective uid.\n");

#if !defined(I_KNOW_ALL_ABOUT_SUDO)
    if (getenv ("SUDO_COMMAND"))
    {	fprintf (stderr,":-( %s is being executed under sudo, "
			"aborting!\n",argv[0]);
	fprintf (stderr,"    See NOTES paragraph in growisofs "
			"manual page for further details.\n");
	exit(FATAL_START(EACCES));
    }
#endif
    /*
     * This is a set-root-uid "entry point" for listed operations. User
     * can't trick this code to unmount arbitrary file system, as [s]he
     * has to pass opened file descriptor to the mounted device. As for
     * file descriptor passed by this program itself, I rely upon the
     * fact that it was appropriately audited at open time in platform-
     * specific setup_fds above...
     */
    if (*argv[0] == '-')
    { int fd;
      struct stat fdst;

	chdir ("/");
	if (argc != 3)	exit (EINVAL);
	fd=atoi (argv[1]);
	if (!strcmp(argv[0],"-umount"))
	{   if (fumount (fd)) exit (errno);
	    exit (0);
	}
	else if (!strcmp(argv[0],"-reload"))
	{   if (fstat (fd,&fdst) < 0)
		perror (":-( unable to fstat"),	exit (1);

	    close (fd);

	    if (media_reload (argv[2],&fdst))
		perror (":-( unable to reload tray"), exit (1);
	    exit (0);
	}
	exit(1);
    }

    /* mmap buffer so that we can use it with /dev/raw/rawN */
#if defined(MAP_ANONYMOUS) && !(defined(__sun) || defined(sun))
    the_buffer = mmap (NULL,2*DVD_BLOCK,PROT_READ|PROT_WRITE,
			MAP_PRIVATE|MAP_ANONYMOUS,-1,(off_t)0);
#else
    { int fd;

	if ((fd=open ("/dev/zero",O_RDWR)) < 0)
	    perror (":-( unable to open \"/dev/zero\"?"),
	    exit(FATAL_START(errno));
	the_buffer = mmap (NULL,2*DVD_BLOCK,PROT_READ|PROT_WRITE,
			MAP_PRIVATE,fd,0);
	close (fd);
    }
#endif
    if (the_buffer == MAP_FAILED)
	fprintf (stderr,":-( unable to anonymously mmap %d: ",2*DVD_BLOCK),
	perror (NULL), exit (FATAL_START(errno));

    mkisofs_argv = malloc ((argc+3)*sizeof(char *));
    if (mkisofs_argv == NULL)
        fprintf (stderr,":-( unable to allocate %lu bytes: ",
			(argc+3)*sizeof(char *)),
	perror (NULL), exit (FATAL_START(errno));

    env = getenv ("MKISOFS");
    mkisofs_argv[0] = (env?env:"mkisofs"), mkisofs_argc = 1;
    growisofs_argc=0;

    _argc=argc,	_argv=argv;

    for (i=1;i<argc;i++)
    { int len=strlen(argv[i]);
      char *opt;

	dev_found = '\0';
	if (argv[i][0] == '-')
	{   opt = argv[i] + ((argv[i][1]=='-')?(len--,1):0);
	    if (argv[i][1] == 'M')
	    {	if (len > 2) in_device = argv[i]+2;
		else	     in_device = argv[++i];
		dev_found = 'M';
	    }
	    else if (!strncmp(opt,"-prev-session",13))
	    {	if (len > 13) in_device = opt+13;
		else          in_device = argv[++i];
		dev_found = 'M';
	    }
	    else if (argv[i][1] == 'Z')
	    {	if (len > 2) in_device = argv[i]+2;
		else	     in_device = argv[++i];
		dev_found = 'Z';
	    }
	    else if (!strncmp(opt,"-zero-session",13))
	    {	if (len > 13) in_device = opt+13;
		else          in_device = argv[++i];
		dev_found = 'Z';
	    }
	    else if (!strcmp(opt,"-poor-man"))
	    {	if (poor_man<0) poor_man = 1;
		continue;
	    }
	    else if (!strncmp(opt,"-speed",6))
	    { char *s;
		if (len>6)	(s=strchr(opt,'='))?s++:s;
		else		s=argv[++i];
		if (s)		speed_factor=atof(s);
		if (speed_factor<=0)
		    fprintf (stderr,"-speed=%.1f: insane speed factor.\n",
				    speed_factor),
		    exit(FATAL_START(EINVAL));
		continue;
	    }
	    else if (!strcmp(opt,"-dvd-compat"))
	    {	if (poor_man<0) poor_man = 1;
		dvd_compat++;
		continue;
	    }
	    else if (!strcmp(opt,"-overburn"))
	    {	overburn = 1;
		continue;
	    }
	    else if (argv[i][1] == 'o')
	    {	if (!strchr(argv[i]+2,'-'))	/* somewhat opportunistic... */
		    fprintf (stderr,"%s: -o[output] option "
				    "is not permitted.\n",argv[0]),
		    exit(FATAL_START(EINVAL));
	    }
	    else if (!strncmp(opt,"-use-the-force-luke",19))
	    { char *s=strchr (opt,'='),*o;

		if (s == NULL)	/* backward compatibility */
		    no_tty_check = 1;
		else
		{   s++;
		    if (strstr(s,"tty"))	no_tty_check = 1;
		    if (strstr(s,"dummy"))	test_write   = 1;
		    if (strstr(s,"notray"))	no_reload    = 1;
		    if (strstr(s,"noload"))	no_reload    = -1;
		    if (strstr(s,"wrvfy"))	wrvfy        = 1;
		    if (strstr(s,"4gms"))	no_4gb_check = 1;
		    if (strstr(s,"moi"))
		    {	quiet=-1; mkisofs_argv[mkisofs_argc++] = "-quiet";   }
		    if ((o=strstr(s,"dao")))
		    {	dvd_compat  += 256;
			/*  vvvvvvvvvvv tracksize option takes precedence! */
			if (dao_size==0 && (o[3]==':' || o[3]=='='))
			{   dao_size=strtol(o+4,0,0);
			    if (dao_size<=0)
				fprintf (stderr,":-( insane dao%c%d option\n",
						o[3],dao_size),
				exit(FATAL_START(EINVAL));
			}
		    }
		    if ((o=strstr(s,"tracksize")))
		    {	if (o[9]==':' || o[9]=='=')
			{   dao_size=strtol(o+10,0,0);
			    if (dao_size<=0)
				fprintf (stderr,":-( insane tracksize%c%d option\n",
						o[9],dao_size),
				exit(FATAL_START(EINVAL));
			}
		    }
		    if ((o=strstr(s,"break")))
		    {	if (o[5]==':' || o[5]=='=')
			{   layer_break=strtol(o+6,0,0);
			    if (layer_break<=0 || layer_break%16)
				fprintf (stderr,":-( insane break%c%d option\n",
						o[5],layer_break),
				exit(FATAL_START(EINVAL));
			}
		    }
		    if ((o=strstr(s,"seek")) && next_session<0)
		    {	if (o[4]==':' || o[4]=='=')
			{   next_session=strtol(o+5,0,0);
			    if (next_session<0 || next_session%16)
				fprintf (stderr,":-( insane seek%c%d option\n",
						o[4],next_session),
				exit(FATAL_START(EINVAL));
			}
		    }
		}
		continue;
	    }
	    else if (!strcmp(opt,"-dvd-video"))
	    {	if (poor_man<0) poor_man = 1;
		dvd_compat++,	growisofs_argc++;
	    }
	    else if (!strcmp(opt,"-quiet"))
		quiet++,			growisofs_argc++;
	    else if (argv[i][1] == 'C' || !strncmp(opt,"-cdrecord-params",16))
	    { char *s=argv[i+1];
	      int   i1,i2;
		if (argv[i][1]=='C' && len>2)	s=argv[i]+2;
		else				i++;
		if (sscanf (s,"%d,%d",&i1,&i2) == 2)
		    alleged_next_session=i2;
		continue;
	    }
	    else if (argv[i][1] == '#' || !strcmp(opt,"-dry-run"))
	    {	dry_run = 1;
		continue;
	    }
	    else if (argv[i][1] == '?' || !strcmp(opt,"-help"))
	    {	PRINT_VERSION (argv[0]);
		printf ("- usage: %s [-dvd-compat] [-overburn] [-speed=1] \\\n"
			"         -[ZM] /dev/dvd <mkisofs options>\n",argv[0]);
		printf ("  for <mkisofs options> see 'mkisofs %s'\n",opt);
		exit (FATAL_START(EINVAL));
	    }
	    else if (strstr (opt,"-version"))
	    {	PRINT_VERSION (argv[0]);
		printf ("  front-ending to %s: ",mkisofs_argv[0]);
		fflush (stdout);
		setuid(getuid());
		execlp (mkisofs_argv[0],mkisofs_argv[0],"-version",NULL);
		fprintf (stderr,"\n- %s: unable to execute %s: ",
				argv[0],mkisofs_argv[0]),
		perror (NULL), exit (FATAL_START(errno));
	    }
	}

	if (dev_found && in_device)
	{   if (*in_device == '=') in_device++;

	    if (1 || dev_found == 'Z')
	    {	if ((in_image = strchr(in_device,'=')))
		{ uid_t euid=geteuid();

		    seteuid (getuid());	/* get real for parsing -[ZM] a=b */

		    while (in_image)
		    {	*in_image='\0';
			errno=0;
			if (access (in_device,F_OK)==0 || errno!=ENOENT)
			    break;
			*in_image='=',
			in_image=strchr(in_image+1,'=');
		    }

		    if (errno)
			fprintf (stderr,":-( \"%s\": unexpected errno:",
					in_device),
			perror (NULL), exit (FATAL_START(errno));

		    if (in_image)
		    {	in_image++;

			if (sscanf(in_image,"/dev/fd/%u",&imgfd) == 1)
			    imgfd = dup (imgfd); /* validate descriptor */
			else
			    imgfd = open64(in_image,O_RDONLY);

			if (imgfd < 0)
			    fprintf (stderr,":-( unable to open64(\"%s\","
					    "O_RDONLY): ",in_image),
			    perror (NULL), exit(FATAL_START(errno));

			if (!strcmp(in_image,"/dev/zero"))
			    zero_image=1;
		    }

		    seteuid (euid);	/* revert to saved [set-]uid */
		}
	    }

	    /*
	     * Sets up in_fd, out_fd, ioctl_handle and poor_man variable.
	     * This procedure is platform-specific. If the program
	     * has to be installed set-root-uid, then this procedure
	     * is the one to drop privileges [if appropriate].
	     */
	    out_device=setup_fds (in_device);

	    *(long *)the_buffer=0;	/* redundant:-)	*/
	    if (mmc_profile&0x10000)	/* blank media	*/
		n=0, errno=EIO;
	    else
	    {	n=pread64 (in_fd,the_buffer,2048,VOLDESC_OFF*CD_BLOCK);
		if (n==0) errno=EIO;	/* end-of-file reached? */
	    }
	    if (n!=2048 && dev_found=='M')
		perror (":-( unable to pread64(2) primary volume descriptor"),
		fprintf (stderr,"    you most likely want to use -Z option.\n"), 
		exit (FATAL_START(errno));

	    if (dev_found == 'M')
	    {	if (memcmp (the_buffer,"\1CD001",6))
	            fprintf (stderr,":-( %s doesn't look like isofs...\n",
		    		in_device), exit(FATAL_START(EMEDIUMTYPE));

		next_session=setup_C_parm(C_parm,
				(struct iso_primary_descriptor *)the_buffer);

		if (imgfd>=0)
		{   if (zero_image)
		    { off64_t off=(atoi(C_parm)-16)*CD_BLOCK;

			dup2(in_fd,imgfd);	/* kludge! */
		    	if (lseek64 (imgfd,off,SEEK_SET) == (off64_t)-1)
			    fprintf (stderr,":-( %s: unable to lseek(%lld): ",
					    in_device,off),
			    perror (NULL), exit(FATAL_START(errno));
		    } else if (alleged_next_session!=next_session)
			fprintf (stderr,"%s: -C argument is %s.\n",
					argv[0],alleged_next_session>=0?
					"insane":"undefined"),
			exit(FATAL_START(EINVAL));
		}
		else if (next_session > (0x200000-0x5000)) /* 4GB/2K-40MB/2K */
		    if ((mmc_profile&0xFFFF)!=0x2B || !no_4gb_check)
			fprintf (stderr,":-( next session would cross 4GB "
					"boundary, aborting...\n"),
			exit (FATAL_START(ENOSPC));

		mkisofs_argv[mkisofs_argc++] = "-C";
		mkisofs_argv[mkisofs_argc++] = C_parm;
#ifdef CANNOT_PASS_DEV_FD_N_TO_MKISOFS
# ifdef PASS_STDIN_TO_MKISOFS
		M_parm = "-";
# else
		M_parm = get_M_parm (in_fd,in_device);
# endif
#else
# ifdef PASS_STDIN_TO_MKISOFS
		M_parm = "/dev/fd/0";
# else
		sprintf (M_parm,"/dev/fd/%d",in_fd);
# endif
#endif
		mkisofs_argv[mkisofs_argc++] = "-M";
		mkisofs_argv[mkisofs_argc++] = M_parm;
		len = 3 + strlen(C_parm) + 3 + strlen(M_parm);
		growisofs_argc += 4;
	    }
	    else
	    {	if (!memcmp (the_buffer,"\1CD001",6))
		    warn_for_isofs = 1;
		if (next_session<0) next_session = 0;
		continue;
	    }
	}
	else
	{   mkisofs_argv[mkisofs_argc++] = argv[i];   }
    }

    if (in_device == NULL)
        fprintf (stderr,"%s: previous \"session\" device is not specified, "
			"do use -M or -Z option\n",argv[0]),
	exit (FATAL_START(EINVAL));

    if (imgfd<0)
    {	if (mkisofs_argc==1)
	    fprintf (stderr,"%s: no mkisofs options specified, "
			    "aborting...\n",argv[0]),
	    exit (FATAL_START(EINVAL));
    }
    else if ((mkisofs_argc-growisofs_argc)>1)
	fprintf (stderr,"%s: no mkisofs options are permitted with =, "
			"aborting...\n",argv[0]),
	exit (FATAL_START(EINVAL));

    mkisofs_argv[mkisofs_argc] = NULL;

    assert (next_session!=-1);
    assert (in_fd!=-1);
    assert (out_fd!=-1);

    /* never finalize disc at multi-sessioning DVD±R recordings...	*/
    { int profile = mmc_profile&0xFFFF;
	if (next_session>0 &&
	    (profile==0x2B || profile==0x1B || profile==0x11))
						dvd_compat=0;
	/* ... except when filling the media up:-)			*/
	if (next_session>0 && zero_image)	dvd_compat=1;
    }

    if (warn_for_isofs)
    { int fd=open("/dev/tty",O_RDONLY);

	if (fd>=0)
	{   if (isatty (fd)) warn_for_isofs |= 2;
	    close (fd);
	}
	else if (isatty (0)) warn_for_isofs |= 2;

	if (no_tty_check || (warn_for_isofs&2))
	    fprintf (stderr,"WARNING: %s already carries isofs!\n",in_device),
	    printf ("About to execute '");
	else
	    fprintf (stderr,"FATAL: %s already carries isofs!\n",in_device),
	    exit(FATAL_START(EBUSY));
    }
    else
	printf ("Executing '");

    if (imgfd>=0)
        printf ("builtin_dd if=%s of=%s obs=32k seek=%u",
		in_image,out_device,next_session/16);
    else
    {   for (i=0;i<mkisofs_argc;i++) printf ("%s ",mkisofs_argv[i]);
        printf ("| builtin_dd of=%s obs=32k seek=%u",
		out_device,next_session/16);
    }
    printf ("'\n");
    fflush (stdout);

    if ((warn_for_isofs&2) && !dry_run && !no_tty_check)
    {	fprintf (stderr,"Sleeping for 5 sec...\a"),	poll (NULL,0,1000);
	fprintf (stderr,"\b\b\b\b\b\b\b\b4 sec...\a"),	poll (NULL,0,1000);
	fprintf (stderr,"\b\b\b\b\b\b\b\b3 sec...\a"),	poll (NULL,0,1000);
	fprintf (stderr,"\b\b\b\b\b\b\b\b2 sec...\a"),	poll (NULL,0,1000);
	fprintf (stderr,"\b\b\b\b\b\b\b\b1 sec...\a"),	poll (NULL,0,1000);
	fprintf (stderr,"\b\b\b\b\b\b\b\b0 sec...\r");
    }

#define CLOSEONEXEC(fd)	do { int f;		\
	if ((f=fcntl ((fd),F_GETFD)) < 0) f=0;	\
	fcntl ((fd),F_SETFD,f|FD_CLOEXEC);	} while (0)
    CLOSEONEXEC(in_fd);
    CLOSEONEXEC(out_fd);
    CLOSEONEXEC(ioctl_fd);
#undef CLOSEONEXEC

    if (!dry_run && (poor_man || next_session==0))	/* unmount media */
    {	pid_t rpid,pid;
	int   rval;

	if ((pid=fork()) == (pid_t)-1)
	    perror (":-( unable to fork -umount"), exit (FATAL_START(errno));

	/* pass through set-root-uid if any */
	if (pid == 0)
	{   char str[12];

	    if ((rval=fcntl (in_fd,F_GETFD))<0) rval=0;
	    fcntl (in_fd,F_SETFD,rval&~FD_CLOEXEC);

	    sprintf (str,"%d",in_fd);
	    execlp (argv[0],"-umount",str,in_device,NULL);
	    exit (FATAL_START(errno));
	}
	while (1)
	{   rpid = waitpid (pid,&rval,0);
	    if (rpid == (pid_t)-1 || (rpid != pid && (errno=ECHILD,1)))
	    {	if (errno==EINTR)	continue;
		else			perror (":-( waipid failed"),
					exit(FATAL_START(errno));
	    }
	    if (WIFSTOPPED(rval))	continue;
	    errno=0;
	    if (WIFEXITED(rval))	errno=WEXITSTATUS(rval);
	    else			errno=ECHILD;
	    break;
	}
	if (errno)
	{   if (errno==EBUSY)
		fprintf (stderr,":-( %s: unable to proceed with recording: ",
			    in_device),
#ifdef __hpux
		fprintf (stderr,"device is mounted\n"),
#else
		fprintf (stderr,"unable to unmount\n"),
#endif
		exit (FATAL_START(errno));
	    else
		fprintf (stderr,":-( unable to umount %s: ",in_device);
	    perror (""), exit (FATAL_START(errno));
	}
    }

    if (poor_man)
    {	out_device=in_device;
	if (!ioctl_device) ioctl_device=out_device;

	switch (mmc_profile&0xFFFF)
	{   case 0x11:	/* DVD-R Sequential		*/
	    case 0x12:	/* DVD-RAM			*/
	    case 0x13:	/* DVD-RW Restricted Overwrite	*/
	    case 0x14:	/* DVD-RW Sequential		*/
	    case 0x15:	/* DVD-R Dual Layer Sequential	*/
#if 0	/* reserved for now... */
	    case 0x16:	/* DVD-R Dual Layer Jump	*/
#endif
	    case 0x1A:	/* DVD+RW			*/
	    case 0x1B:	/* DVD+R			*/
	    case 0x2B:	/* DVD+R Double Layer		*/
		break;
	    default:
		fprintf (stderr,":-( %s: mounted media doesn't appear to be "
	    			"[supported] DVD±RW or DVD±R\n",out_device),
		exit(FATAL_START(EMEDIUMTYPE));
		break;
	}
    }

    if (imgfd>=0)
    {	quiet--;
	if (builtin_dd (imgfd,out_fd,next_session*CD_BLOCK) < 0)
	{ int err = errno;
	    errno = err&0x7F;     /* they might be passing FATAL_START */
	    perror (":-( write failed"), exit (err);
	}
	errno = 0;
	exit (0);
	/* NOT REACHED */
    }

    pipe_mkisofs_up (mkisofs_argv,in_fd,out_fd,next_session*CD_BLOCK);

    /*
     * Recall that second 32KB written in this session are still
     * in the upper half of the_buffer! And now note that
     * poor_man_rewritable() fills the first 32KB with volume
     * descriptor set from beginning of the volume, if appropriate
     * that is! The latter is a workaround hook for DVD-RW Restricted
     * Overwrite interfering with DVD+RW kernel patch... Is this mode
     * ugly or is it ugly? G-r-r-r...
     */
    if (next_session!=0 &&
	(!poor_man || poor_man_rewritable (ioctl_handle,the_buffer)))
    {
	descr = (struct iso_primary_descriptor *)the_buffer;

	if (memcmp (descr+16,"\1CD001",6))
	    fprintf (stderr,":-( %s:%d doesn't look like isofs!\n",
				out_device,next_session),
	    exit ((errno=EMEDIUMTYPE));

	fprintf (stderr,"%s: copying volume descriptor(s)\n",
			poor_man?ioctl_device:out_device);

	new_size = from_733(descr[16].volume_space_size) + next_session;

	if (!poor_man && (errno=0, pread64 (out_fd,descr,DVD_BLOCK,
				  	    VOLDESC_OFF*CD_BLOCK)) != DVD_BLOCK)
	    errno=errno?errno:EIO,
	    perror (":-( unable to pread64(2) volume descriptors set"),
	    exit (errno);

	memcpy (descr+0,descr+16,sizeof(struct iso_primary_descriptor));
	to_733(descr[0].volume_space_size,new_size);

	/*
	 * copy secondary volume descriptor(s) which are expected to
	 * appear in the very same order.
	 */
	for (i=1;i<MAX_IVDs;i++)
	{   if (descr[16+i].type[0] == (unsigned char)255)	break;
	    if (memcmp (descr[16+i].id,"CD001",5))		break;

	    if (descr[16+i].type[0] != descr[i].type[0])
	    {	fprintf (stderr,":-? volume descriptor mismatch, did you "
				"use same mkisofs options?\n");
		break;
	    }
	    memcpy (descr+i,descr+16+i,sizeof(struct iso_primary_descriptor));
	    to_733(descr[i].volume_space_size,new_size);
	}

	if ((*pwrite64_method) (out_fd,descr,DVD_BLOCK,
		VOLDESC_OFF*CD_BLOCK) != DVD_BLOCK)
            perror (":-( unable to pwrite64(2) volume descriptors set"),
	    exit (errno);
    }

    errno = 0;
    exit (0);
}
