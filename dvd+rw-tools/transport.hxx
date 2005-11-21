//
// This is part of dvd+rw-tools by Andy Polyakov <appro@fy.chalmers.se>
//
// Use-it-on-your-own-risk, GPL bless...
//
// For further details see http://fy.chalmers.se/~appro/linux/DVD+RW/
//

#if defined(__unix) || defined(__unix__)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/time.h>

inline long getmsecs()
{ struct timeval tv;
    gettimeofday (&tv,NULL);
  return tv.tv_sec*1000+tv.tv_usec/1000;
}

#include <errno.h>

#ifndef EMEDIUMTYPE
#define EMEDIUMTYPE	EINVAL
#endif
#ifndef	ENOMEDIUM
#define	ENOMEDIUM	ENODEV
#endif

#elif defined(_WIN32)
#include <windows.h>
#include <stdio.h>

#define EINVAL		ERROR_BAD_ARGUMENTS
#define ENOMEM		ERROR_OUTOFMEMORY
#define EMEDIUMTYPE	ERROR_MEDIA_INCOMPATIBLE
#define ENOMEDIUM	ERROR_MEDIA_OFFLINE
#define ENODEV		ERROR_BAD_COMMAND
#define EAGAIN		ERROR_NOT_READY
#define ENOSPC		ERROR_DISK_FULL
#define EIO		ERROR_NOT_SUPPORTED
#define ENXIO		ERROR_GEN_FAILURE

static class _win32_errno {
    public:
	operator int()		{ return GetLastError(); }
	int operator=(int e)	{ SetLastError(e); return e; }
} _sys_errno;
#ifdef errno
#undef errno
#endif
#define errno _sys_errno

inline void perror (const char *str)
{ LPVOID lpMsgBuf;

    FormatMessage( 
	FORMAT_MESSAGE_ALLOCATE_BUFFER |
	FORMAT_MESSAGE_FROM_SYSTEM | 
	FORMAT_MESSAGE_IGNORE_INSERTS,
	NULL,
	GetLastError(),
	0, // Default language
	(LPTSTR) &lpMsgBuf,
	0,
	NULL 
	);
    if (str)
	fprintf (stderr,"%s: %s",str,lpMsgBuf);
    else
	fprintf (stderr,"%s",lpMsgBuf);

    LocalFree(lpMsgBuf);
}

#define poll(a,b,t)	Sleep(t)
#define getmsecs()	GetTickCount()
#define exit(e)		ExitProcess(e)

#endif

#define CREAM_ON_ERRNO_NAKED(s)				\
    switch ((s)[12])					\
    {	case 0x04:	errno=EAGAIN;	break;		\
	case 0x20:	errno=ENODEV;	break;		\
	case 0x21:	if ((s)[13]==0)	errno=ENOSPC;	\
			else		errno=EINVAL;	\
			break;				\
	case 0x30:	errno=EMEDIUMTYPE;	break;	\
	case 0x3A:	errno=ENOMEDIUM;	break;	\
    }
#define CREAM_ON_ERRNO(s)	do { CREAM_ON_ERRNO_NAKED(s) } while(0)

#define	FATAL_START(er)	(0x80|(er))
#define ERRCODE(s)	((((s)[2]&0x0F)<<16)|((s)[12]<<8)|((s)[13]))
#define	SK(errcode)	(((errcode)>>16)&0xF)
#define	ASC(errcode)	(((errcode)>>8)&0xFF)
#define ASCQ(errcode)	((errcode)&0xFF)

static void sperror (const char *cmd,int err)
{ int saved_errno=errno;

    if (err==-1)
	fprintf (stderr,":-( unable to %s: ",cmd);
    else
	fprintf (stderr,":-[ %s failed with SK=%Xh/ASC=%02Xh/ACQ=%02Xh]: ",
			cmd,SK(err),ASC(err),ASCQ(err));
    errno=saved_errno, perror (NULL);
}

class autofree {
    private:
	unsigned char *ptr;
    public:
	autofree()			{ ptr=NULL; }
	~autofree()			{ if (ptr) free(ptr); }
	unsigned char *operator=(unsigned char *str)
					{ return ptr=str; }
	operator unsigned char *()	{ return ptr; }
};

#if defined(__linux)

#include <sys/ioctl.h>
#include <linux/cdrom.h>
#include <mntent.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <scsi/sg.h>
#if !defined(SG_FLAG_LUN_INHIBIT)
# if defined(SG_FLAG_UNUSED_LUN_INHIBIT)
#  define SG_FLAG_LUN_INHIBIT SG_FLAG_UNUSED_LUN_INHIBIT
# else
#  define SG_FLAG_LUN_INHIBIT 0
# endif
#endif
#ifndef CHECK_CONDITION
#define CHECK_CONDITION 0x01
#endif

typedef enum {	NONE=CGC_DATA_NONE,	// 3
		READ=CGC_DATA_READ,	// 2
		WRITE=CGC_DATA_WRITE	// 1
	     } Direction;
#ifdef SG_IO
static const int Dir_xlate [4] = {	// should have been defined
					// private in USE_SG_IO scope,
					// but it appears to be too
		0,			// implementation-dependent...
		SG_DXFER_TO_DEV,	// 1,CGC_DATA_WRITE
		SG_DXFER_FROM_DEV,	// 2,CGC_DATA_READ
		SG_DXFER_NONE	};	// 3,CGC_DATA_NONE
static const class USE_SG_IO {
private:
    int	yes_or_no;
public:
    USE_SG_IO()	{ struct utsname buf;
		    uname (&buf);
		    // was CDROM_SEND_PACKET declared dead in 2.5?
		    yes_or_no=(strcmp(buf.release,"2.5.43")>=0);
		}
    ~USE_SG_IO(){}
    operator int()			const	{ return yes_or_no; }
    int operator[] (Direction dir)	const	{ return Dir_xlate[dir]; }
} use_sg_io;
#endif

class Scsi_Command {
private:
    int fd,autoclose;
    char *filename;
    struct cdrom_generic_command cgc;
    union {
	struct request_sense	s;
	unsigned char		u[18];
    } _sense;
#ifdef SG_IO
    struct sg_io_hdr		sg_io;
#else
    struct { int cmd_len,timeout; }	sg_io;
#endif
public:
    Scsi_Command()	{ fd=-1, autoclose=1; filename=NULL; }
    Scsi_Command(int f)	{ fd=f,  autoclose=0; filename=NULL; }
    Scsi_Command(void*f){ fd=(long)f, autoclose=0; filename=NULL; }
    ~Scsi_Command()	{ if (fd>=0 && autoclose) close(fd),fd=-1;
			  if (filename) free(filename),filename=NULL;
			}
    int associate (const char *file,const struct stat *ref=NULL)
    { struct stat sb;

	/*
	 * O_RDWR is expected to provide for none set-root-uid
	 * execution under Linux kernel 2.6[.8]. Under 2.4 it
	 * falls down to O_RDONLY...
	 */
	if ((fd=open (file,O_RDWR|O_NONBLOCK)) < 0 &&
	    (fd=open (file,O_RDONLY|O_NONBLOCK)) < 0)	return 0;
	if (fstat(fd,&sb) < 0)				return 0;
	if (!S_ISBLK(sb.st_mode))	{ errno=ENOTBLK;return 0; }

	if (ref && (!S_ISBLK(ref->st_mode) || ref->st_rdev!=sb.st_rdev))
	{   errno=ENXIO; return 0;   }

	filename=strdup(file);

	return 1;
    }
    unsigned char &operator[] (size_t i)
    {	if (i==0)
	{   memset(&cgc,0,sizeof(cgc)), memset(&_sense,0,sizeof(_sense));
	    cgc.quiet = 1;
	    cgc.sense = &_sense.s;
#ifdef SG_IO
	    if (use_sg_io)
	    {	memset(&sg_io,0,sizeof(sg_io));
		sg_io.interface_id= 'S';
		sg_io.mx_sb_len	= sizeof(_sense);
		sg_io.cmdp	= cgc.cmd;
		sg_io.sbp	= _sense.u;
		sg_io.flags	= SG_FLAG_LUN_INHIBIT|SG_FLAG_DIRECT_IO;
	    }
#endif
	}
	sg_io.cmd_len = i+1;
	return cgc.cmd[i];
    }
    unsigned char &operator()(size_t i)	{ return _sense.u[i]; }
    unsigned char *sense()		{ return _sense.u;    }
    void timeout(int i)			{ cgc.timeout=sg_io.timeout=i*1000; }
#ifdef SG_IO
    size_t residue()			{ return use_sg_io?sg_io.resid:0; }
#else
    size_t residue()			{ return 0; }
#endif
    int transport(Direction dir=NONE,void *buf=NULL,size_t sz=0)
    { int ret = 0;

#ifdef SG_IO
#define KERNEL_BROKEN 0
	if (use_sg_io)
	{   sg_io.dxferp		= buf;
	    sg_io.dxfer_len		= sz;
	    sg_io.dxfer_direction	= use_sg_io[dir];
	    if (ioctl (fd,SG_IO,&sg_io)) return -1;

#if !KERNEL_BROKEN
	    if ((sg_io.info&SG_INFO_OK_MASK) != SG_INFO_OK)
#else
	    if (sg_io.status)
#endif
	    {	errno=EIO; ret=-1;
#if !KERNEL_BROKEN
		if (sg_io.masked_status&CHECK_CONDITION)
#endif
		{   ret = ERRCODE(sg_io.sbp);
		    if (ret==0) ret=-1;
		    else	CREAM_ON_ERRNO(sg_io.sbp);
		}
	    }
	    return ret;
	}
	else
#undef KERNEL_BROKEN
#endif
	{   cgc.buffer		= (unsigned char *)buf;
	    cgc.buflen		= sz;
	    cgc.data_direction	= dir;
	    if (ioctl (fd,CDROM_SEND_PACKET,&cgc))
	    {	ret = ERRCODE(_sense.u);
		if (ret==0) ret=-1;
	    }
	}
	return ret;
    }
    int umount(int f=-1)
    { struct stat    fsb,msb;
      struct mntent *mb;
      FILE          *fp;
      pid_t          pid,rpid;
      int            ret=0,rval;

	if (f==-1) f=fd;
	if (fstat (f,&fsb) < 0)				return -1;
	if ((fp=setmntent ("/proc/mounts","r"))==NULL)	return -1;

	while ((mb=getmntent (fp))!=NULL)
	{   if (stat (mb->mnt_fsname,&msb) < 0) continue; // corrupted line?
	    if (msb.st_rdev == fsb.st_rdev)
	    {	ret = -1;
		if ((pid = fork()) == (pid_t)-1)	break;
		if (pid == 0) execl ("/bin/umount","umount",mb->mnt_dir,NULL);
		while (1)
		{   rpid = waitpid (pid,&rval,0);
		    if (rpid == (pid_t)-1)
		    {	if (errno==EINTR)	continue;
			else			break;
		    }
		    else if (rpid != pid)
		    {	errno = ECHILD;
			break;
		    }
		    if (WIFEXITED(rval))
		    {	if (WEXITSTATUS(rval) == 0) ret=0;
			else			    errno=EBUSY; // most likely
			break;
		    }
		    else
		    {	errno = ENOLINK;	// some phony errno
			break;
		    }
		}
		break;
	    }
	}
	endmntent (fp);

	return ret;
    }
    int is_reload_needed ()
    {	return ioctl (fd,CDROM_MEDIA_CHANGED,CDSL_CURRENT) == 0;   }
};

#elif defined(__OpenBSD__) || defined(__NetBSD__)

#include <sys/ioctl.h>
#include <sys/scsiio.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/mount.h>

typedef off_t off64_t;
#define stat64   stat
#define fstat64  fstat
#define open64   open
#define pread64	 pread
#define pwrite64 pwrite
#define lseek64  lseek

typedef enum {	NONE=0,
		READ=SCCMD_READ,
		WRITE=SCCMD_WRITE
	     } Direction;

class Scsi_Command {
private:
    int fd,autoclose;
    char *filename;
    scsireq_t req;
public:
    Scsi_Command()	{ fd=-1, autoclose=1; filename=NULL; }
    Scsi_Command(int f)	{ fd=f,  autoclose=0; filename=NULL; }
    Scsi_Command(void*f){ fd=(long)f, autoclose=0; filename=NULL; }
    ~Scsi_Command()	{ if (fd>=0 && autoclose) close(fd),fd=-1;
			  if (filename) free(filename),filename=NULL;
			}
    int associate (const char *file,const struct stat *ref=NULL)
    { struct stat sb;

	fd=open(file,O_RDWR|O_NONBLOCK);
	// this is --^^^^^^-- why we have to run set-root-uid...

	if (fd < 0)					return 0;
	if (fstat(fd,&sb) < 0)				return 0;
	if (!S_ISCHR(sb.st_mode))	{ errno=EINVAL; return 0; }

	if (ref && (!S_ISCHR(ref->st_mode) || ref->st_rdev!=sb.st_rdev))
	{   errno=ENXIO; return 0;   }

	filename=strdup(file);

	return 1;
    }
    unsigned char &operator[] (size_t i)
    {	if (i==0)
	{   memset(&req,0,sizeof(req));
	    req.flags = SCCMD_ESCAPE;
	    req.timeout = 30000;
	    req.senselen = 18; //sizeof(req.sense);
	}
	req.cmdlen = i+1;
	return req.cmd[i];
    }
    unsigned char &operator()(size_t i)	{ return req.sense[i]; }
    unsigned char *sense()		{ return req.sense;    }
    void timeout(int i)			{ req.timeout=i*1000; }
    size_t residue()			{ return req.datalen-req.datalen_used; }
    int transport(Direction dir=NONE,void *buf=NULL,size_t sz=0)
    { int ret=0;

	req.databuf = (caddr_t)buf;
	req.datalen = sz;
	req.flags |= dir;
	if (ioctl (fd,SCIOCCOMMAND,&req) < 0)	return -1;
	if (req.retsts==SCCMD_OK)		return 0;

	errno=EIO; ret=-1;
	if (req.retsts==SCCMD_SENSE)
	{   ret = ERRCODE(req.sense);
	    if (ret==0) ret=-1;
	    else	CREAM_ON_ERRNO(req.sense);
	}
	return ret;
    }
    // this code is basically redundant... indeed, we normally want to
    // open device O_RDWR, but we can't do that as long as it's mounted.
    // in other words, whenever this routine is invoked, device is not
    // mounted, so that it could as well just return 0;
    int umount(int f=-1)
    { struct stat    fsb,msb;
      struct statfs *mntbuf;
      int            ret=0,mntsize,i;

	if (f==-1) f=fd;

	if (fstat (f,&fsb) < 0)				return -1;
	if ((mntsize=getmntinfo(&mntbuf,MNT_NOWAIT))==0)return -1;

	for (i=0;i<mntsize;i++)
	{ char rdev[MNAMELEN+1],*slash,*rslash;

	    mntbuf[i].f_mntfromname[MNAMELEN-1]='\0';	// paranoia
	    if ((slash=strrchr (mntbuf[i].f_mntfromname,'/'))==NULL) continue;
	    strcpy (rdev,mntbuf[i].f_mntfromname); // rdev is 1 byte larger!
	    rslash = strrchr  (rdev,'/');
	    *(rslash+1) = 'r', strcpy (rslash+2,slash+1);
	    if (stat (rdev,&msb) < 0) continue;
	    if (msb.st_rdev == fsb.st_rdev)
	    {	ret=unmount (mntbuf[i].f_mntonname,0);
		break;
            }
	}

	return ret;
    }
    int is_reload_needed ()
    {	return 1;   }
};

#elif defined(__FreeBSD__)

#include <sys/ioctl.h>
#include <camlib.h>
#include <cam/scsi/scsi_message.h>
#include <cam/scsi/scsi_pass.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <dirent.h>

typedef off_t off64_t;
#define stat64   stat
#define fstat64  fstat
#define open64   open
#define pread64  pread
#define pwrite64 pwrite
#define lseek64  lseek

#define ioctl_fd (((struct cam_device *)ioctl_handle)->fd)

typedef enum {	NONE=CAM_DIR_NONE,
		READ=CAM_DIR_IN,
		WRITE=CAM_DIR_OUT
	     } Direction;

class Scsi_Command {
private:
    int fd,autoclose;
    char *filename;
    struct cam_device  *cam;
    union ccb		ccb;
public:
    Scsi_Command()
    {	cam=NULL, fd=-1, autoclose=1; filename=NULL;   }
    Scsi_Command(int f)
    {	char pass[32];	// periph_name is 16 chars long

	cam=NULL, fd=-1, autoclose=1, filename=NULL;

	memset (&ccb,0,sizeof(ccb));
	ccb.ccb_h.func_code = XPT_GDEVLIST;
	if (ioctl (f,CAMGETPASSTHRU,&ccb) < 0) return;

	sprintf (pass,"/dev/%.15s%u",ccb.cgdl.periph_name,ccb.cgdl.unit_number);
	cam=cam_open_pass (pass,O_RDWR,NULL);
    }
    Scsi_Command(void *f)
    {	cam=(struct cam_device *)f, autoclose=0; fd=-1; filename=NULL;  }
    ~Scsi_Command()
    {	if (cam && autoclose)	cam_close_device(cam), cam=NULL;
	if (fd>=0)		close(fd);
	if (filename)		free(filename), filename=NULL;
    }

    int associate (const char *file,const struct stat *ref=NULL)
    {	struct stat sb;
	char pass[32];		// periph_name is 16 chars long

	fd=open(file,O_RDONLY|O_NONBLOCK);

	// all if (ref) code is actually redundant, it never runs
	// as long as RELOAD_NEVER_NEEDED...
	if (ref && fd<0 && errno==EPERM)
	{   // expectedly we would get here if file is /dev/passN
	    if (stat(file,&sb) < 0)		return 0;
	    if (!S_ISCHR(ref->st_mode) || ref->st_rdev!=sb.st_rdev)
		return (errno=ENXIO,0);
	    fd=open(file,O_RDWR);
	}

	if (fd < 0)				return 0;
	if (fstat(fd,&sb) < 0)			return 0;
	if (!S_ISCHR(sb.st_mode))		return (errno=EINVAL,0);

	if (ref && (!S_ISCHR(ref->st_mode) || ref->st_rdev!=sb.st_rdev))
	    return (errno=ENXIO,0);

	memset (&ccb,0,sizeof(ccb));
	ccb.ccb_h.func_code = XPT_GDEVLIST;
	if (ioctl(fd,CAMGETPASSTHRU,&ccb)<0)	return (close(fd),fd=-1,0);

	sprintf (pass,"/dev/%.15s%u",ccb.cgdl.periph_name,ccb.cgdl.unit_number);
	cam=cam_open_pass (pass,O_RDWR,NULL);
	if (cam==NULL)				return (close(fd),fd=-1,0);

	filename=strdup(file);

	return 1;
    }
    unsigned char &operator[] (size_t i)
    {	if (i==0)
	{   memset(&ccb,0,sizeof(ccb));
	    ccb.ccb_h.path_id    = cam->path_id;
	    ccb.ccb_h.target_id  = cam->target_id;
	    ccb.ccb_h.target_lun = cam->target_lun;
	    cam_fill_csio (&(ccb.csio),
			1,				// retries
			NULL,				// cbfncp
			CAM_DEV_QFRZDIS,		// flags
			MSG_SIMPLE_Q_TAG,		// tag_action
			NULL,				// data_ptr
			0,				// dxfer_len
			sizeof(ccb.csio.sense_data),	// sense_len
			0,				// cdb_len
			30*1000);			// timeout
	}
	ccb.csio.cdb_len = i+1;
	return ccb.csio.cdb_io.cdb_bytes[i];
    }
    unsigned char &operator()(size_t i)
			{ return ((unsigned char *)&ccb.csio.sense_data)[i]; }
    unsigned char *sense()
			{ return (unsigned char*)&ccb.csio.sense_data;    }
    void timeout(int i)	{ ccb.ccb_h.timeout=i*1000; }
    size_t residue()	{ return ccb.csio.resid; }
    int transport(Direction dir=NONE,void *buf=NULL,size_t sz=0)
    {	int ret=0;

	ccb.csio.ccb_h.flags |= dir;
	ccb.csio.data_ptr  = (u_int8_t *)buf;
	ccb.csio.dxfer_len = sz;

	if ((ret = cam_send_ccb(cam, &ccb)) < 0)
	    return -1;

	if ((ccb.ccb_h.status & CAM_STATUS_MASK) == CAM_REQ_CMP)
	    return 0;

	unsigned char  *sense=(unsigned char *)&ccb.csio.sense_data;

	errno = EIO;
	// FreeBSD 5-CURRENT since 2003-08-24, including 5.2 fails to
	// pull sense data automatically, at least for ATAPI transport,
	// so I reach for it myself...
	if ((ccb.csio.scsi_status==SCSI_STATUS_CHECK_COND) &&
	    !(ccb.ccb_h.status&CAM_AUTOSNS_VALID))
	{   u_int8_t  _sense[18];
	    u_int32_t resid=ccb.csio.resid;

	    memset(_sense,0,sizeof(_sense));

	    operator[](0)      = 0x03;	// REQUEST SENSE
	    ccb.csio.cdb_io.cdb_bytes[4] = sizeof(_sense);
	    ccb.csio.cdb_len   = 6;
	    ccb.csio.ccb_h.flags |= CAM_DIR_IN|CAM_DIS_AUTOSENSE;
	    ccb.csio.data_ptr  = _sense;
	    ccb.csio.dxfer_len = sizeof(_sense);
	    ccb.csio.sense_len = 0;
	    ret = cam_send_ccb(cam, &ccb);

	    ccb.csio.resid = resid;
	    if (ret<0)	return -1;
	    if ((ccb.ccb_h.status&CAM_STATUS_MASK) != CAM_REQ_CMP)
		return errno=EIO,-1;

	    memcpy(sense,_sense,sizeof(_sense));
	}

	ret = ERRCODE(sense);
	if (ret == 0)	ret = -1;
	else		CREAM_ON_ERRNO(sense);

	return ret;
    }
    int umount(int f=-1)
    { struct stat    fsb,msb;
      struct statfs *mntbuf;
      int            ret=0,mntsize,i;

	if (f==-1) f=fd;

	if (fstat (f,&fsb) < 0)				return -1;
	if ((mntsize=getmntinfo(&mntbuf,MNT_NOWAIT))==0)return -1;

	for (i=0;i<mntsize;i++)
	{   if (stat (mntbuf[i].f_mntfromname,&msb) < 0) continue;
	    if (msb.st_rdev == fsb.st_rdev)
	    {	ret=unmount (mntbuf[i].f_mntonname,0);
		break;
	    }
	}

	return ret;
    }
#define RELOAD_NEVER_NEEDED	// according to Matthew Dillon
    int is_reload_needed ()
    {  return 0;   }
};

#elif defined(__sun) || defined(sun)
//
// Licensing terms for commercial distribution for Solaris are to be
// settled with Inserve Technology, Åvägen 6, 412 50 GÖTEBORG, Sweden,
// tel. +46-(0)31-86 87 88, see http://www.inserve.se/ for further
// details.
//
#include <volmgt.h>
extern "C" int _dev_unmount(char *); // VolMgt ON Consolidation Private API
#include <sys/param.h>
#include <sys/scsi/impl/uscsi.h>
#include <sys/mount.h>
#include <sys/mnttab.h>
#include <sys/wait.h>
#include <sys/cdio.h>
#include <sys/utsname.h>
#include <sys/dklabel.h>
#include <sys/dkio.h>

typedef enum {	NONE=0,
		READ=USCSI_READ,
		WRITE=USCSI_WRITE
	     } Direction;

class Scsi_Command {
private:
    int fd,autoclose;
    char *filename;
    struct uscsi_cmd ucmd;
    unsigned char cdb[16], _sense[18];
public:
    Scsi_Command()	{ fd=-1, autoclose=1; filename=NULL; }
    Scsi_Command(int f)	{ fd=f,  autoclose=0; filename=NULL; }
    Scsi_Command(void*f){ fd=(long)f, autoclose=0; filename=NULL; }
    ~Scsi_Command()	{ if (fd>=0 && autoclose) close(fd),fd=-1;
			  if (filename) free(filename),filename=NULL;
			}
    int associate (const char *file,const struct stat *ref=NULL)
    { class autofree {
      private:
	char *ptr;
      public:
	autofree()			{ ptr=NULL; }
	~autofree()			{ if (ptr) free(ptr); }
	char *operator=(char *str)	{ return ptr=str; }
	operator char *()		{ return ptr; }
      } volname,device;
      struct stat sb;
      int v;

	if ((v=volmgt_running()))
	{   if ((volname=volmgt_symname ((char *)file)))
	    {	if ((device=media_findname (volname)) == NULL)
		    return 0;
	    }
	    else if ((device=media_findname ((char *)file))==NULL)
		return 0;
	}
	else device=strdup(file);

	fd=open (device,O_RDONLY|O_NONBLOCK);
	if (fd<0)					return 0;
	if (fstat(fd,&sb) < 0)				return 0;
	if (!S_ISCHR(sb.st_mode))	{ errno=ENOTTY;	return 0; }

	if (ref && (!S_ISCHR(ref->st_mode) || ref->st_rdev!=sb.st_rdev))
	{   errno=ENXIO; return 0;   }

	filename=strdup(device);

	return 1;
    }
    unsigned char &operator[] (size_t i)
    {	if (i==0)
	{   memset (&ucmd,0,sizeof(ucmd));
	    memset (cdb,0,sizeof(cdb));
	    memset (_sense,0,sizeof(_sense));
	    ucmd.uscsi_cdb    = (caddr_t)cdb;
	    ucmd.uscsi_rqbuf  = (caddr_t)_sense;
	    ucmd.uscsi_rqlen  = sizeof(_sense);
	    ucmd.uscsi_flags  = USCSI_SILENT  | USCSI_DIAGNOSE |
				USCSI_ISOLATE | USCSI_RQENABLE;
	    ucmd.uscsi_timeout= 60;
	}
	ucmd.uscsi_cdblen = i+1;
	return cdb[i];
    }
    unsigned char &operator()(size_t i)	{ return _sense[i]; }
    unsigned char *sense()		{ return _sense;    }
    void timeout(int i)			{ ucmd.uscsi_timeout=i; }
    size_t residue()			{ return ucmd.uscsi_resid; }
    int transport(Direction dir=NONE,void *buf=NULL,size_t sz=0)
    { int ret=0;

	ucmd.uscsi_bufaddr = (caddr_t)buf;
	ucmd.uscsi_buflen  = sz;
	ucmd.uscsi_flags  |= dir;
	if (ioctl(fd,USCSICMD,&ucmd))
	{   if (errno==EIO && _sense[0]==0)	// USB seems to be broken...
	    {	size_t residue=ucmd.uscsi_resid;
		memset(cdb,0,sizeof(cdb));
		cdb[0]=0x03;			// REQUEST SENSE
		cdb[4]=sizeof(_sense);
		ucmd.uscsi_cdblen  = 6;
		ucmd.uscsi_bufaddr = (caddr_t)_sense;
		ucmd.uscsi_buflen  = sizeof(_sense);
		ucmd.uscsi_flags   = USCSI_SILENT  | USCSI_DIAGNOSE |
				     USCSI_ISOLATE | USCSI_READ;
		ucmd.uscsi_timeout = 1;
		ret = ioctl(fd,USCSICMD,&ucmd);
		ucmd.uscsi_resid = residue;
		if (ret) return -1;
	    }
	    ret = ERRCODE(_sense);
	    if (ret==0)	ret=-1;
	    //else	CREAM_ON_ERRNO(_sense);
	}
	return ret;
    }
    // mimics umount(2), therefore inconsistent return values
    int umount(int f=-1)
    { struct stat   fsb,msb;
      struct mnttab mb;
      FILE         *fp;
      pid_t         pid,rpid;
      int           ret=0,i,rval;

	if (f==-1) f=fd;

	if (fstat (f,&fsb) < 0)			return -1;
	if ((fp=fopen (MNTTAB,"r")) == NULL)	return -1;

	while ((i=getmntent (fp,&mb)) != -1)
	{   if (i != 0)				continue; // ignore corrupted lines
	    if (stat (mb.mnt_special,&msb) < 0)	continue; // also corrupted?
	    if (msb.st_rdev == fsb.st_rdev)
	    {	if (_dev_unmount (mb.mnt_special))	break;
		{  struct utsname uts;
		    if (uname (&uts)>=0 && strcmp(uts.release,"5.8")>=0)
		    {	// M-m-m-m-m! Solaris 8 or later...
			ret = ::umount (mb.mnt_special);
			break;
		    }
		}
		ret = -1;
		if ((pid = fork()) == (pid_t)-1)	break;
		if (pid == 0) execl ("/usr/sbin/umount","umount",mb.mnt_mountp,NULL);
		while (1)
		{   rpid = waitpid (pid,&rval,0);
		    if (rpid == (pid_t)-1)
		    {	if (errno==EINTR)	continue;
			else			break;
		    }
		    else if (rpid != pid)
		    {	errno = ECHILD;
			break;
		    }
		    if (WIFEXITED(rval))
		    {	if (WEXITSTATUS(rval) == 0)	ret=0;
			else				errno=EBUSY; // most likely
			break;
		    }
		    else if (WIFSTOPPED(rval) || WIFCONTINUED(rval))
			continue;
		    else
		    {	errno = ENOLINK;	// some phony errno
			break;
		    }
		}
		break;
	    }
	}
	fclose (fp);

	return ret;
    }
    int is_reload_needed ()
    { struct dk_minfo  mi;
      struct dk_allmap pt;

	if (ioctl (fd,DKIOCGMEDIAINFO,&mi) < 0)	return 1;

	memset (&pt,0,sizeof(pt));
	pt.dka_map[2].dkl_nblk = mi.dki_capacity*(mi.dki_lbsize/DEV_BSIZE);
	pt.dka_map[0] = pt.dka_map[2];
	if (ioctl (fd,DKIOCSAPART,&pt) < 0)	return 1;

	return 0;
    }
};

#elif defined(__hpux)
//
// Copyright (C) 2003  Hewlett-Packard Development Company, L.P.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
// =====================================================================
//
// The code targets 11i/11.11 and later, but it might just work on
// 11.0 as well. For further information contact following HP office
//
// Hewlett-Packard Company
// 3404 E Harmony Road
// Fort Collins, CO 80528 USA
//
#include <strings.h>
#include <sys/scsi.h>
#include <mntent.h>
#include <sys/mknod.h>

typedef enum {  NONE=0,
                READ=SCTL_READ,
                WRITE=0
             } Direction;

class Scsi_Command {
private:
    int fd;
    int autoclose;
    char *filename;
    struct sctl_io cmd;
public:
    Scsi_Command()	{ fd=-1, autoclose=1; filename=NULL; }
    Scsi_Command(int f)	{ fd=f,  autoclose=0; filename=NULL; }
    Scsi_Command(void*f){ fd=(long)f, autoclose=0; filename=NULL; }
    ~Scsi_Command()	{ if (fd>=0 && autoclose) close(fd),fd=-1;
                          if (filename) free(filename),filename=NULL;
			}
    int associate (const char *file,const struct stat *ref=NULL)
    { struct stat sb;

	fd=open (file,O_RDONLY|O_NONBLOCK);

	if (fd < 0)					return 0;
	if (fstat(fd,&sb) < 0)				return 0;
	if (!S_ISCHR(sb.st_mode))	{ errno=EINVAL; return 0; }

	// shall we check for DIOC_DESCRIBE here?

	if (ref && (!S_ISCHR(ref->st_mode) || ref->st_rdev!=sb.st_rdev))
	{   errno=ENXIO; return 0;   }

	filename=strdup(file);

	return 1;
    }
    unsigned char &operator[] (size_t i)
    {	if (i==0)
	{   bzero (&cmd,sizeof(struct sctl_io));
	    cmd.max_msecs=30*1000;
	}
	cmd.cdb_length = i+1;
	return cmd.cdb[i];
    }
    unsigned char &sense(size_t i)  { return cmd.sense[i];  }
    unsigned char *sense()	    { return cmd.sense;	    }
    void timeout(int i)		    { cmd.max_msecs=i*1000; }
    size_t residue()		    { return cmd.data_length-cmd.data_xfer; }
    int transport(Direction dir=NONE,void *buf=NULL,size_t sz=0)
    { const char *err = NULL;

	cmd.data = buf;
	cmd.data_length = sz;
	cmd.flags = dir;

	if (ioctl (fd,SIOC_IO,&cmd) != 0)	return -1;

	if (cmd.cdb_status == S_GOOD)		return 0;

	errno = EIO;
	switch (cmd.cdb_status)
	{   case SCTL_INVALID_REQUEST:	err = "SCTL_INVALID_REQUEST";	break;
	    case SCTL_SELECT_TIMEOUT:	err = "SCTL_SELECT_TIMEOUT";	break;
	    case SCTL_INCOMPLETE:	err = "SCTL_INCOMPLETE";	break;
	    case SCTL_POWERFAIL:	err = "SCTL_POWERFAIL";		break;
	    default:			err = NULL;			break;
	}
	if (err != NULL)
	{   fprintf (stderr,":-( FAIL: command failed with %s.\n",err);
	    return -1;
	}

	switch (cmd.cdb_status & 0xff)
	{   case S_CHECK_CONDITION:
		if (cmd.sense_status==S_GOOD && cmd.sense_xfer!=0)
		{   CREAM_ON_ERRNO_NAKED(cmd.sense)	// CREAM_ON_ERRNO
							// provokes PA-RISC
							// compiler bug...
		    return ERRCODE(cmd.sense);
		}
		else
		    fprintf (stderr,":-( FAIL: S_CHECK_CONDITION status, "
				    "but no sense data?\n");
		break;
	    case S_BUSY:
		fprintf (stderr,":-( FAIL: S_BUSY condition?\n");
		errno = EAGAIN;
		break;
	    default:
		fprintf (stderr,":-( FAIL: unknown cdb_status=0x%x\n",
				cmd.cdb_status);
		break;
	}

	return -1;
    }
    // for now we only detect if media is mounted...
    int umount(int f=-1)
    { struct stat    fsb,msb;
      struct mntent *mb;
      FILE          *fp;
      int            ret=0;
      char           bdev[32];
      dev_t          m;

	if (f==-1) f=fd;

	if (fstat (f,&fsb) < 0)				return -1;
	if ((fp=setmntent (MNT_MNTTAB,"r")) == NULL)	return -1;

	m=minor(fsb.st_rdev);
	sprintf(bdev,"/dev/dsk/c%ut%ud%x",(m>>16)&0xFF,(m>>12)&0xF,(m>>8)&0xF);
	if (stat (bdev,&fsb) < 0)			return -1;

	while ((mb=getmntent (fp))!=NULL)
	{   if (stat (mb->mnt_fsname,&msb) < 0) continue; // corrupted line?
	    if (msb.st_rdev == fsb.st_rdev)
	    {	errno=EBUSY; ret=-1; break;	}
	}
	endmntent (fp);

	return ret;
    }
    int is_reload_needed ()
    {	return 1;   }
};

#elif defined(__sgi)
//
// Not necessarily relevant IRIX notes.
//
// Information about UDF support seems to be contradictory. Some manuals
// maintain that UDF writing is supported for DVD-RAM and hard disk, but
// the only mention of UDF in IRIX release notes is "6.5.18 introduces
// read-only support for the UDF filesystems format." If UDF writing is
// supported, then it was implemented presumably in 6.5.21. DVD-RAM
// writing at block device level was most likely introduced by then too.
//
// IRIX doesn't provide access to files larger than 2GB on ISO9660.
// That's because ISO9660 is implemented as NFSv2 user-land server,
// and 2GB limit is implied by NFSv2 protocol.
//

#ifdef PRIVATE
#undef PRIVATE	// <sys/dsreq.h> conflicts with <sys/mman.h>?
#endif
#include <sys/dsreq.h>
#ifdef PRIVATE
#undef PRIVATE	// <sys/dsreq.h> conflicts with <sys/mman.h>?
#endif
#include <mntent.h>
#include <sys/wait.h>
#include <poll.h>
#include <sys/attributes.h>
#include <sys/param.h>
#include <mediad.h>

typedef enum {	NONE=0,
		READ=DSRQ_READ,
		WRITE=DSRQ_WRITE
	     } Direction;

class Scsi_Command {
private:
    int fd,autoclose;
    char *filename;
    dsreq_t req;
    unsigned char cdb[16], _sense[18];
public:
    Scsi_Command()	{ fd=-1, autoclose=1; filename=NULL; }
    Scsi_Command(int f)	{ fd=f,  autoclose=0; filename=NULL; }
    Scsi_Command(void*f){ fd=(long)f, autoclose=0; filename=NULL; }
    ~Scsi_Command()	{ if (fd>=0 && autoclose) close(fd),fd=-1;
			  if (filename) free(filename),filename=NULL;
			}
    int associate (const char *file,const struct stat *ref=NULL)
    { struct stat sb;
      char hw_path[MAXPATHLEN];
      int  hw_len=sizeof(hw_path)-1;

	if (attr_get(file,"_devname",hw_path,&hw_len,0))	return 0;
	if (hw_len>=sizeof(hw_path)) hw_len=sizeof(hw_path)-1;	// paranoia
	hw_path[hw_len]='\0';

	if (ref)
	{   // hw_path is maintained by kernel through hwgfs and
	    // I assume it's not subject to race conditions...
	    if (stat(hw_path,&sb))	return 0;
            if (!S_ISCHR(ref->st_mode) || ref->st_rdev!=sb.st_rdev)
            {	errno=ENXIO; return 0;   }
	}

	if (strcmp(hw_path+strlen(hw_path)-5,"/scsi"))
	{   char *s=strstr(hw_path,"/disk/");
	    if (s==NULL)	{   errno=EINVAL; return 0;   }
	    strcpy (s,"/scsi");
	}
	fd=open (hw_path,O_RDONLY|O_NONBLOCK);
	if (fd<0)					return 0;
	if (fstat(fd,&sb) < 0)				return 0;
	if (!S_ISCHR(sb.st_mode))	{ errno=ENOTTY;	return 0; }

	filename=strdup(file);

	return 1;
    }
    unsigned char &operator[] (size_t i)
    {	if (i==0)
	{   memset (&req,0,sizeof(req));
	    memset (cdb,0,sizeof(cdb));
	    memset (_sense,0,sizeof(_sense));
	    req.ds_cmdbuf   = (caddr_t)cdb;
	    req.ds_sensebuf = (caddr_t)_sense;
	    req.ds_senselen = sizeof(_sense);
	    req.ds_flags    = DSRQ_SENSE;
	    req.ds_time     = 60*1000;
	}
	req.ds_cmdlen = i+1;
	return cdb[i];
    }
    unsigned char &operator()(size_t i)	{ return _sense[i]; }
    unsigned char *sense()		{ return _sense;    }
    void timeout(int i)			{ req.ds_time=i*1000; }
    size_t residue()			{ return req.ds_datalen-req.ds_datasent; }
    int transport(Direction dir=NONE,void *buf=NULL,size_t sz=0)
    { int ret=0,retries=3;

	req.ds_databuf = (caddr_t)buf;
	req.ds_datalen = sz;
	req.ds_flags  |= dir;

	// I personally don't understand why do we need to loop, but 
	// /usr/share/src/irix/examples/scsi/lib/src/dslib.c is looping...
	while (retries--)
	{   if (ioctl(fd,DS_ENTER,&req) < 0)	return -1;
	    if (req.ds_status==STA_GOOD)	return 0;

	    if (req.ds_ret==DSRT_NOSEL)		continue;
	    if (req.ds_status==STA_BUSY || req.ds_status==STA_RESERV)
	    {	poll(NULL,0,500); continue;   }

	    break;
	}

	errno=EIO; ret=-1;
	if (req.ds_status==STA_CHECK && req.ds_sensesent>=14) 
	{   ret = ERRCODE(_sense);
	    if (ret==0)	ret=-1;
	    else	CREAM_ON_ERRNO(_sense);
	}
	return ret;
    }
    // mimics umount(2), therefore inconsistent return values
    int umount(int f=-1)
    { struct stat    fsb,msb;
      struct mntent *mb;
      FILE          *fp;
      pid_t          pid,rpid;
      int            ret=0,rval;
      char           hw_path[MAXPATHLEN];
      int            hw_len=sizeof(hw_path)-1;

	if (f==-1) f=fd;

	if (fstat (f,&fsb) < 0)	return -1;

	if (!getenv("MEDIAD_GOT_EXCLUSIVEUSE"))
	{   if (attr_getf (f,"_devname",hw_path,&hw_len,0))	return -1;
	    if (hw_len>=sizeof(hw_path)) hw_len=sizeof(hw_path)-1;// paranoia
	    hw_path[hw_len]='\0';

	    // mediad even unmounts removable volumes. However! The
	    // locks are "granted" even for unmanaged devices, so
	    // it's not possible to tell if device is ignored through
	    // /etc/config/mediad.config or actually managed. Therefore
	    // I have to pass through own unmount code in either case...

	    mediad_get_exclusiveuse(hw_path,"dvd+rw-tools");
	    switch (mediad_last_error())
	    {	case RMED_NOERROR:	break;
		case RMED_EACCESS:
		case RMED_ECANTUMOUNT:	errno=EBUSY; return -1;
		case RMED_ENOMEDIAD:	break;
		case -1:	if(errno==ECONNREFUSED)	break;	// no mediad...
				else			return -1;
		default:	errno=ENOTTY; return -1;
	    }
	}

	if ((fp=setmntent (MOUNTED,"r"))==NULL)	return -1;

	while ((mb=getmntent (fp))!=NULL)
	{   if (stat (mb->mnt_fsname,&msb) < 0)	continue; // corrupted line?
	    // Following effectively catches only /dev/rdsk/dksXdYvol,
	    // which is sufficient for iso9660 volumes, but not for e.g.
	    // EFS formatted media. I mean code might have to be more
	    // versatile... Wait for feedback...
	    if (msb.st_rdev == fsb.st_rdev)
	    {	ret = -1;
		if ((pid = fork()) == (pid_t)-1)	break;
		if (pid == 0) execl ("/sbin/umount","umount",mb->mnt_dir,NULL);
		while (1)
		{   rpid = waitpid (pid,&rval,0);
		    if (rpid == (pid_t)-1)
		    {	if (errno==EINTR)	continue;
			else			break;
		    }
		    else if (rpid != pid)
		    {	errno = ECHILD;
			break;
		    }
		    if (WIFEXITED(rval))
		    {	if (WEXITSTATUS(rval) == 0)	ret=0;
			else				errno=EBUSY; // most likely
			break;
		    }
		    else if (WIFSTOPPED(rval) || WIFCONTINUED(rval))
			continue;
		    else
		    {	errno = ENOLINK;	// some phony errno
			break;
		    }
		}
		break;
	    }
	}
	endmntent (fp);

	return ret;
    }
#if 0	// for now just an idea to test...
#define RELOAD_NEVER_NEEDED
    int is_reload_needed ()
    {  return 0;   }
#else
    int is_reload_needed ()
    {	return 1;   }
#endif
};

#elif defined(_WIN32)

#if defined(__MINGW32__)
#include <ddk/ntddscsi.h>
#else
#include <devioctl.h>
#include <ntddscsi.h>
#endif

typedef enum {	NONE=SCSI_IOCTL_DATA_UNSPECIFIED,
		READ=SCSI_IOCTL_DATA_IN,
		WRITE=SCSI_IOCTL_DATA_OUT
	     } Direction;

typedef struct {
    SCSI_PASS_THROUGH_DIRECT	spt;
    unsigned char		sense[18];
} SPKG;

class Scsi_Command {
private:
    HANDLE fd;
    SPKG   p;
public:
    Scsi_Command()	{ fd=INVALID_HANDLE_VALUE; }
    ~Scsi_Command()	{ if (fd!=INVALID_HANDLE_VALUE) CloseHandle (fd); }
    int associate (const char *file)
    { char dev[32];
	sprintf(dev,"\\\\.\\%.*s",sizeof(dev)-5,file);
	fd=CreateFile (dev,GENERIC_WRITE|GENERIC_READ,
			   FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
	return fd!=INVALID_HANDLE_VALUE;
    }
    unsigned char &operator[] (size_t i)
    {	if (i==0)
	{   memset(&p,0,sizeof(p));
	    p.spt.Length = sizeof(p.spt);
	    p.spt.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
	    p.spt.TimeOutValue = 30;
	    p.spt.SenseInfoLength = sizeof(p.sense);
	    p.spt.SenseInfoOffset = offsetof(SPKG,sense);
	}
	p.spt.CdbLength = i+1;
	return p.spt.Cdb[i];
    }
    unsigned char &operator()(size_t i)	{ return p.sense[i]; }
    unsigned char *sense()		{ return p.sense;    }
    void timeout(int i)			{ p.spt.TimeOutValue=i; }
    size_t residue()			{ return 0; } // bogus
    int transport(Direction dir=NONE,void *buf=NULL,size_t sz=0)
    { DWORD bytes;
      int   ret=0;

	p.spt.DataBuffer = buf;
	p.spt.DataTransferLength = sz;
	p.spt.DataIn = dir;
	if (DeviceIoControl (fd,IOCTL_SCSI_PASS_THROUGH_DIRECT,
				&p,sizeof(p.spt),
				&p,sizeof(p),
				&bytes,FALSE) == 0) return -1;
	if (p.sense[0]&0x70)
	{   SetLastError (ERROR_GEN_FAILURE);
	    ret = ERRCODE(p.sense);
	    if (ret==0) ret=-1;
	    else	CREAM_ON_ERRNO(p.sense);
	}
	return ret;
    }
    int umount ()		{   return 0;   }	// bogus
    int is_reload_needed ()	{   return 0;   }	// bogus
};

#else
#error "Unsupported OS"
#endif

#define DUMP_EVENTS 0
static int handle_events (Scsi_Command &cmd)
{ unsigned char  event[8];
  unsigned short profile=0,started=0;
  int err,ret=0;
  unsigned int descr;
  static unsigned char events=0xFF;	// "All events"

    while (events)
    {	cmd[0] = 0x4A;		// GET EVENT
	cmd[1] = 1;		// "Polled"
	cmd[4] = events;
	cmd[8] = sizeof(event);
	cmd[9] = 0;
	if ((err=cmd.transport(READ,event,sizeof(event))))
	{   events=0;
	    sperror ("GET EVENT",err);
	    return ret;
	}

	events = event[3];

	if ((event[2]&7) == 0			||
	    (event[0]<<8|event[1]) == 2	|| 
	    (event[4]&0xF) == 0			)	// No Changes
	    return ret;

	descr  = event[4]<<24|event[5]<<16|event[6]<<8|event[7];
#if DUMP_EVENTS
	fprintf(stderr,"< %d[%08x],%x >\n",event[2],descr,events);
#endif

	switch(event[2]&7)
	{   case 0: return ret;			// No [supported] events
	    case 1: ret |= 1<<1;		// Operational Change
		if ((descr&0xFFFF) < 3)
		    goto read_profile;
	    start_unit:
	    	if (!started)
		{   cmd[0]=0x1B;	// START STOP UNIT
		    cmd[4]=1;		// "Start"
		    cmd[5]=0;
		    if ((err=cmd.transport()) && err!=0x62800)
			sperror ("START UNIT",err);
		    started=1, profile=0;
		}
	    read_profile:
		if (!profile)
		{   cmd[0] = 0x46;	// GET CONFIGURATION
		    cmd[8] = sizeof(event);
		    cmd[9] = 0;
		    if (!cmd.transport(READ,event,sizeof(event)))
			profile=event[6]<<8|event[7];
		}
		break;
	    case 2: ret |= 1<<2;		// Power Management
		if (event[5]>1)		// State is other than Active
		    goto start_unit;
		break;
	    case 3: ret |= 1<<3; break;		// External Request
	    case 4: ret |= 1<<4;		// Media
		if (event[5]&2)		// Media in
		    goto  start_unit;
		break;
	    case 5: ret |= 1<<5; break;		// Multiple Initiators
	    case 6:				// Device Busy
		if ((event[4]&0xF)==1)	// Timeout occured
		{   poll(NULL,0,(descr&0xFFFF)*100+100);
		    cmd[0] = 0;		// TEST UNIT READY
		    cmd[5] = 0;
		    if ((err=cmd.transport()))
			sperror("TEST UNIT READY",err);
		    ret |= 1<<6;
		}
		break;
	    case 7: ret |= 1<<7; break;		// Reserved
	}
    }

  return ret;
}
#undef DUMP_EVENTS

static int wait_for_unit (Scsi_Command &cmd,volatile int *progress=NULL)
{ unsigned char *sense=cmd.sense(),sensebuf[18];
  int  err;
  long msecs=1000;

    while (1)
    {	if (msecs > 0) poll(NULL,0,msecs);
	msecs = getmsecs();
	cmd[0] = 0;	// TEST UNIT READY
	cmd[5] = 0;
	if (!(err=cmd.transport ())) break;
	// I wish I could test just for sense.valid, but (at least)
	// hp dvd100i returns 0 in valid bit at this point:-( So I
	// check for all bits...
	if ((sense[0]&0x70)==0)
	{   perror (":-( unable to TEST UNIT READY");
	    return -1;
	}
	else if (sense[12]==0x3A) // doesn't get any further than "no media"
	    return err;

	while (progress)
	{   if (sense[15]&0x80)
	    {	*progress = sense[16]<<8|sense[17];
		break;
	    }
	    // MMC-3 (draft) specification says that the unit should
	    // return progress indicator in key specific bytes even
	    // in reply to TEST UNIT READY. I.e. as above! But (at
	    // least) hp dvd100i doesn't do that and I have to fetch
	    // it separately:-(
	    cmd[0] = 0x03;	// REQUEST SENSE
	    cmd[4] = sizeof(sensebuf);
	    cmd[5] = 0;
	    if ((err=cmd.transport (READ,sensebuf,sizeof(sensebuf))))
	    {   sperror ("REQUEST SENSE",err);
		return err;
	    }
	    if (sensebuf[15]&0x80)
		*progress = sensebuf[16]<<8|sensebuf[17];
	    break;
	}
	msecs = 1000 - (getmsecs() - msecs);
    }

  return 0;
}

#define FEATURE21_BROKEN 1
static void page05_setup (Scsi_Command &cmd, unsigned short profile=0,
	unsigned char p32=0xC0)
	// 5 least significant bits of p32 go to p[2], Test Write&Write Type
	// 2 most significant bits go to p[3], Multi-session field
	// 0xC0 means "Multi-session, no Test Write, Incremental"
{ unsigned int   len,bdlen;
  unsigned char  header[12],track[32],*p;
#if !FEATURE21_BROKEN
  unsigned char  feature21[24];
#endif
  int            err;
  class autofree page05;

    if (profile==0)
    { unsigned char prof[8];

	cmd[0] = 0x46;	// GET CONFIGURATION
	cmd[8] = sizeof(prof);
	cmd[9] = 0;
	if ((err=cmd.transport(READ,prof,sizeof(prof))))
	    sperror ("GET CONFIGURATION",err), exit(FATAL_START(errno));

	profile = prof[6]<<8|prof[7];
    }

#if !FEATURE21_BROKEN
    if (profile==0x11 || profile==0x14)
    {	cmd[0] = 0x46;	// GET CONFIGURATION
	cmd[1] = 2;	// ask for the only feature...
	cmd[3] = 0x21;	// the "Incremental Streaming Writable" one
	cmd[8] = 8;	// read the header only to start with
	cmd[9] = 0;
	if ((err=cmd.transport(READ,feature21,8)))
	    sperror ("GET CONFIGURATION",err), exit(FATAL_START(errno));

	len = feature21[0]<<24|feature21[1]<<16|feature21[2]<<8|feature21[3];
	len += 4;
	if (len>sizeof(feature21))
	    len = sizeof(feature21);
	else if (len<(8+8))
	    fprintf (stderr,":-( READ FEATURE DESCRIPTOR 0021h: insane length\n"),
	    exit(FATAL_START(EINVAL));

	cmd[0] = 0x46;	// GET CONFIGURATION
	cmd[1] = 2;	// ask for the only feature...
	cmd[3] = 0x21;	// the "Incremental Streaming Writable" one
	cmd[8] = len;	// this time with real length
	cmd[9] = 0;
	if ((err=cmd.transport(READ,feature21,len)))
	    sperror ("READ FEATURE DESCRIPTOR 0021h",err),
	    exit(FATAL_START(errno));

	if ((feature21[8+2]&1)==0)
	    fprintf (stderr,":-( FEATURE 0021h is not in effect\n"),
	    exit(FATAL_START(EMEDIUMTYPE));
    }
    else
	feature21[8+2]=0;
#endif

    cmd[0] = 0x52;	// READ TRACK INFORMATION
    cmd[1] = 1;		// TRACK INFORMATION
    cmd[5] = 1;		// track#1, in DVD context it's safe to assume
    			//          that all tracks are in the same mode
    cmd[8] = sizeof(track);
    cmd[9] = 0;
    if ((err=cmd.transport(READ,track,sizeof(track))))
	sperror ("READ TRACK INFORMATION",err), exit(FATAL_START(errno));

    // WRITE PAGE SETUP //
    cmd[0] = 0x5A;		// MODE SENSE
    cmd[1] = 0x08;		// "Disable Block Descriptors"
    cmd[2] = 0x05;		// "Write Page"
    cmd[8] = sizeof(header);	// header only to start with
    cmd[9] = 0;
    if ((err=cmd.transport(READ,header,sizeof(header))))
	sperror ("MODE SENSE",err), exit(FATAL_START(errno));

    len   = (header[0]<<8|header[1])+2;
    bdlen = header[6]<<8|header[7];

    if (bdlen)	// should never happen as we set "DBD" above
    {	if (len <= (8+bdlen+14))
	    fprintf (stderr,":-( LUN is impossible to bear with...\n"),
	    exit(FATAL_START(EINVAL));
    }
    else if (len < (8+2+(unsigned int)header[9]))// SANYO does this.
	len = 8+2+header[9];

    page05 = (unsigned char *)malloc(len);
    if (page05 == NULL)
	fprintf (stderr,":-( memory exhausted\n"),
	exit(FATAL_START(ENOMEM));

    cmd[0] = 0x5A;		// MODE SENSE
    cmd[1] = 0x08;		// "Disable Block Descriptors"
    cmd[2] = 0x05;		// "Write Page"
    cmd[7] = len>>8;
    cmd[8] = len;		// real length this time
    cmd[9] = 0;
    if ((err=cmd.transport(READ,page05,len)))
	sperror("MODE SENSE",err), exit(FATAL_START(errno));

    len -= 2;
    if (len < ((unsigned int)page05[0]<<8|page05[1]))	// paranoia:-)
	page05[0] = len>>8, page05[1] = len;

    len   = (page05[0]<<8|page05[1])+2;
    bdlen = page05[6]<<8|page05[7];
    len  -= bdlen;
    if (len < (8+14))
	fprintf (stderr,":-( LUN is impossible to bear with...\n"),
	exit(FATAL_START(EINVAL));

    p = page05 + 8 + bdlen;

    memset (p-8,0,8);
    p[0] &= 0x7F;

    // copy "Test Write" and "Write Type" from p32
    p[2] &= ~0x1F, p[2] |= p32&0x1F;	
    p[2] |= 0x40;	// insist on BUFE on

    // setup Preferred Link Size
#if !FEATURE21_BROKEN
    if (feature21[8+2]&1)
    {	if (feature21[8+7])	p[2] |= 0x20,  p[5] = feature21[8+8];
	else			p[2] &= ~0x20, p[5] = 0;
    }
#else	// At least Pioneer DVR-104 returns some bogus data in
	// Preferred Link Size...
    if (profile==0x11 || profile==0x14)	// Sequential recordings...
	p[2] |= 0x20, p[5] = 0x10;
#endif
    else
	p[2] &= ~0x20, p[5] = 0;

    // copy Track Mode from TRACK INFORMATION
    // [some DVD-R units (most notably Panasonic LF-D310), insist
    // on Track Mode 5, even though it's effectively ignored]
    p[3] &= ~0x0F, p[3] |= profile==0x11?5:(track[5]&0x0F);

    // copy "Multi-session" bits from p32
    p[3] &= ~0xC0, p[3] |= p32&0xC0;
    if (profile == 0x13)	// DVD-RW Restricted Overwrite
    	p[3] &= 0x3F;		// always Single-session?

    // setup Data Block Type
    // Some units [e.g. Toshiba/Samsung TS-H542A] return "unknown Data
    // Block Type" in track[6]&0x0F field. Essentially it's a firmware
    // glitch, yet it makes certain sense, as track may not be written
    // yet...
    if ((track[6]&0x0F)==1 || (track[6]&0x0F)==0x0F)
		p[4] = 8;
    else	fprintf (stderr,":-( none Mode 1 track\n"),
		exit(FATAL_START(EMEDIUMTYPE));

    // setup Packet Size
    // [some DVD-R units (most notably Panasonic LF-D310), insist
    // on fixed Packet Size of 16 blocks, even though it's effectively
    // ignored]
    p[3] |= 0x20, memset (p+10,0,4), p[13] = 0x10;
    if (track[6]&0x10)
	memcpy (p+10,track+20,4);	// Fixed
    else if (profile != 0x11)
	p[3] &= ~0x20, p[13] = 0;	// Variable

    switch (profile)
    {	case 0x13:	// DVD-RW Restricted Overwrite
	    if (!(track[6]&0x10))
		fprintf (stderr,":-( track is not formatted for fixed packet size\n"),
		exit(FATAL_START(EMEDIUMTYPE));
	    break;
	case 0x14:	// DVD-RW Sequential Recording
	case 0x11:	// DVD-R  Sequential Recording
	    if (track[6]&0x10)
		fprintf (stderr,":-( track is formatted for fixed packet size\n"),
		exit(FATAL_START(EMEDIUMTYPE));
	    break;
	default:
#if 0
	    fprintf (stderr,":-( invalid profile %04xh\n",profile);
	    exit(FATAL_START(EMEDIUMTYPE));
#endif
	    break;
    }

    p[8] = 0;		// "Session Format" should be ignored, but
			// I reset it just in case...

    cmd[0] = 0x55;	// MODE SELECT
    cmd[1] = 0x10;	// conformant
    cmd[7] = len>>8;
    cmd[8] = len;
    cmd[9] = 0;
    if ((err=cmd.transport(WRITE,p-8,len)))
	sperror ("MODE SELECT",err), exit(FATAL_START(errno));
    // END OF WRITE PAGE SETUP //
}
#undef FEATURE21_BROKEN

#undef ERRCODE
#undef CREAM_ON_ERRNO
#undef CREAM_ON_ERRNO_NAKED
