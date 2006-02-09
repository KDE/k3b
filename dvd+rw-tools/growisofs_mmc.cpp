#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE
#endif
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#if defined(__linux)
/* ... and "engage" glibc large file support */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include "transport.hxx"

#include <time.h>
#include <sys/types.h>
#include <signal.h>
#define sigs_mask(how)	do {	\
    sigset_t mask;		\
				\
    sigemptyset (&mask);	\
    sigaddset (&mask,SIGHUP),	sigaddset (&mask,SIGINT),	\
    sigaddset (&mask,SIGTERM),	sigaddset (&mask,SIGPIPE);	\
					\
    sigprocmask (how,&mask,NULL);	\
} while (0)

#define	ONEX 1352	// 1385 * 1000 / 1024

static int           media_written=0,next_track=1,velocity=0,
		     is_dao=0,quickgrown=0,do_reload=1;

static void         *ioctl_handle=(void *)-1;
#ifndef ioctl_fd
#define ioctl_fd ((long)ioctl_handle)
#endif

static unsigned int  next_wr_addr=1;	// it starts as boolean
static unsigned int  buf_size=256;	// measured in 1KBs
static unsigned char formats[260],disc_info[32];

//
// ADDED
//
static int last_buffer_fill = 0;
// ----------

extern int	dvd_compat,test_write,no_reload,mmc_profile,_argc,wrvfy,lazy;
extern double	speed_factor;
extern char	*ioctl_device,**_argv;

extern "C"
int fumount (int fd)
{ Scsi_Command cmd;
  return cmd.umount(fd);
}

extern "C"
int media_reload (char *name=NULL,struct stat *sb=NULL)
{   if (name==NULL)
    {	Scsi_Command cmd(ioctl_handle);

	while (1)	// Pioneer DVR-x06 needs this...
	{   cmd[0] = 0x1B;	// START/STOP UNIT
	    cmd[1] = 0x1;	// "IMMED"
	    cmd[4] = 0;		// "Stop"
	    cmd[5] = 0;
	    if (cmd.transport() == 0x20407)	// "OP IN PROGRESS"
	    {	poll (NULL,0,333);
		continue;
	    }
	    break;
	}

#if defined(RELOAD_NEVER_NEEDED)
#undef RELOAD_NEVER_NEEDED
#define RELOAD_NEVER_NEEDED 1
#else
#define RELOAD_NEVER_NEEDED 0
#endif
	if (RELOAD_NEVER_NEEDED || no_reload>0)
	{   cmd[0] = 0x1E;	// ALLOW MEDIA REMOVAL
	    cmd[5] = 0;
	    cmd.transport ();

	    return (errno=0);
	}
#if !RELOAD_NEVER_NEEDED

	char str[12];
	int n;
	
	if ((n=fcntl (ioctl_fd,F_GETFD))<0) n=0;
	fcntl (ioctl_fd,F_SETFD,n&~FD_CLOEXEC);

	sprintf (str,"%d",ioctl_fd);
	execlp(_argv[0],"-reload",str,ioctl_device,NULL);
    }
    else
    {
	{ Scsi_Command cmd;

	    if (!cmd.associate (name,sb)) return 1;

	    if (cmd.is_reload_needed())
	    {	fprintf (stderr,"%s: reloading tray\n",name);
		cmd[0] = 0x1E;		// ALLOW MEDIA REMOVAL
		cmd[5] = 0;
		if (cmd.transport ()) return 1;

		while (1)	// Pioneer DVR-x05 needs this...
		{   cmd[0] = 0x1B;	// START/STOP UNIT
		    cmd[1] = 0x1;	// "IMMED"
		    cmd[4] = 0x2;	// "Eject"
		    cmd[5] = 0;
		    if (cmd.transport() == 0x20407) // "OP IN PROGRESS"
		    {	poll (NULL,0,333);
			continue;
		    }
		    break;
		}
		// yes, once again, non-"IMMED"...
		cmd[0] = 0x1B;		// START/STOP UNIT
		cmd[4] = 0x2;		// "Eject"
		cmd[5] = 0;
		if (cmd.transport()) return 1;
	    }
#if defined(__sun) || defined(sun)
	    else if (volmgt_running())
	    {	setuid(getuid());
		execl("/usr/bin/volrmmount","volrmmount","-i",name,NULL);
		return 0;	// not normally reached
	    }
#endif
	    else return 0;	// m-m-m-m! patched kernel:-)
	}
	if (no_reload>=0)
	{ Scsi_Command cmd;
	    if (cmd.associate (name,sb))
	    {	cmd[0] = 0x1B;		// START/STOP UNIT
		cmd[1] = 0x1;		// "IMMED"
		cmd[4] = 0x3;		// "Load"
		cmd[5] = 0;
		cmd.transport ();
	    }
	    errno=0;	// ignore all errors on load
	}
	return 0;
#endif
    }

  return 1;
}

extern "C"
int get_mmc_profile (void *fd)
{ Scsi_Command cmd(fd);
  unsigned char buf[8],inq[128];
  int profile=0,once=1,blank=0,err;
  unsigned int len;

    // INQUIRY is considered to be "non-intrusive" in a sense that
    // it won't interfere with any other operation nor clear sense
    // data, which might be important to retain for security reasons.

    cmd[0] = 0x12;	// INQUIRY
    cmd[4] = 36;
    cmd[5] = 0;
    if ((err=cmd.transport(READ,inq,36)))
	sperror ("INQUIRY",err),
	exit (FATAL_START(errno));

    // make sure we're talking to MMC unit, for security reasons...
    if ((inq[0]&0x1F) != 5)
	fprintf (stderr,":-( not an MMC unit!\n"),
	exit (FATAL_START(EINVAL));

    do {
	cmd[0] = 0x46;
	cmd[8] = sizeof(buf);
	cmd[9] = 0;
	if ((err=cmd.transport(READ,buf,sizeof(buf))))
	    sperror ("GET CONFIGURATION",err),
	    fprintf (stderr,":-( non-MMC unit?\n"),
	    exit (FATAL_START(errno));

        if ((profile = buf[6]<<8|buf[7]) || !once) break;

	// no media?
	cmd[0] = 0;	// TEST UNIT READY
	cmd[5] = 0;
	if ((cmd.transport()&0xFFF00) != 0x23A00) break;

	// try to load tray...
	cmd[0] = 0x1B;	// START/STOP UNIT
	cmd[4] = 0x3;	// "Load"
	cmd[5] = 0;
	if ((err=cmd.transport ()))
	    sperror ("LOAD TRAY",err),
	    exit (FATAL_START(errno));

#if 1
	wait_for_unit (cmd);
#else
	// consume sense data, most likely "MEDIA MAY HAVE CHANGED"
	cmd[0] = 0;	// TEST UNIT READY
	cmd[5] = 0;
	if ((err=cmd.transport ()) == -1)
	    sperror ("TEST UNIT READY",err),
	    exit (FATAL_START(errno));
#endif
    } while (once--);

    if (profile==0 || (profile&0x30)==0)	// no or non-DVD media...
	return profile;

    cmd[0] = 0x51;	// READ DISC INFORMATION
    cmd[8] = sizeof(disc_info);
    cmd[9] = 0;
    if ((err=cmd.transport (READ,disc_info,sizeof(disc_info))))
	sperror ("READ DISC INFORMATION",err),
	exit (FATAL_START(errno));

    // see if it's blank media
    if ((disc_info[2]&3) == 0)	blank=0x10000;

    if (profile != 0x1A && profile != 0x13 && profile != 0x12)
	return blank|profile;

    cmd[0] = 0x23;	// READ FORMAT CAPACITIES
    cmd[8] = 12;
    cmd[9] = 0;
    if ((err=cmd.transport (READ,formats,12)))
	sperror ("READ FORMAT CAPACITIES",err),
	exit (FATAL_START(errno));

    len = formats[3];
    if (len&7 || len<16)
	fprintf (stderr,":-( FORMAT allocaion length isn't sane"),
	exit (FATAL_START(EINVAL));

    cmd[0] = 0x23;	// READ FORMAT CAPACITIES
    cmd[7] = (4+len)>>8;
    cmd[8] = (4+len)&0xFF;
    cmd[9] = 0;
    if ((err=cmd.transport (READ,formats,4+len)))
	sperror ("READ FORMAT CAPACITIES",err),
	exit (FATAL_START(errno));

    if (len != formats[3])
	fprintf (stderr,":-( parameter length inconsistency\n"),
	exit(FATAL_START(EINVAL));

    // see if it's not formatted
    if ((formats[8]&3) != 2) blank = 0x10000;

  return blank|profile;
}

static unsigned int get_2k_capacity (Scsi_Command &cmd)
{ unsigned char	buf[32];
  unsigned int	ret=0;
  unsigned int	nwa,free_blocks;
  int		i,obligatory,len,err;

    obligatory=0x00;
    switch (mmc_profile&0xFFFF)
    {	case 0x1A:		// DVD+RW
	    obligatory=0x26;
	case 0x13:		// DVD-RW Restricted Overwrite
	    for (i=8,len=formats[3];i<len;i+=8)
		if ((formats [4+i+4]>>2) == obligatory) break;

	    if (i==len)
	    {	fprintf (stderr,":-( can't locate obligatory format descriptor\n");
		return 0;
	    }

	    ret  = formats[4+i+0]<<24;
	    ret |= formats[4+i+1]<<16;
	    ret |= formats[4+i+2]<<8;
	    ret |= formats[4+i+3];
	    nwa = formats[4+5]<<16|formats[4+6]<<8|formats[4+7];
	    if (nwa>2048)	ret *= nwa/2048;
	    else if (nwa<2048)	ret /= 2048/nwa;
	    break;

	case 0x12:		// DVD-RAM
	    // As for the moment of this writing I don't format DVD-RAM.
	    // Therefore I just pull formatted capacity for now...
	    ret  = formats[4+0]<<24;
	    ret |= formats[4+1]<<16;
	    ret |= formats[4+2]<<8;
	    ret |= formats[4+3];
	    nwa = formats[4+5]<<16|formats[4+6]<<8|formats[4+7];
	    if (nwa>2048)	ret *= nwa/2048;
	    else if (nwa<2048)	ret /= 2048/nwa;
	    break;

	case 0x11:		// DVD-R
	case 0x14:		// DVD-RW Sequential
	case 0x15:		// DVD-R Dual Layer Sequential
	case 0x1B:		// DVD+R
	case 0x2B:		// DVD+R Double Layer
	    cmd[0] = 0x52;	// READ TRACK INFORMATION
	    cmd[1] = 1;
	    cmd[4] = next_track>>8;
	    cmd[5] = next_track&0xFF;	// last track, set up earlier
	    cmd[8] = sizeof(buf);
	    cmd[9] = 0;
	    if ((err=cmd.transport (READ,buf,sizeof(buf))))
	    {	sperror ("READ TRACK INFORMATION",err);
		return 0;
	    }

	    nwa = 0;
	    if (buf[7]&1)	// NWA_V
	    {	nwa  = buf[12]<<24;
		nwa |= buf[13]<<16;
		nwa |= buf[14]<<8;
		nwa |= buf[15];
	    }
	    free_blocks  = buf[16]<<24;
	    free_blocks |= buf[17]<<16;
	    free_blocks |= buf[18]<<8;
	    free_blocks |= buf[19];
	    ret = nwa + free_blocks;
	    break;

	default:
	    break;
    }

  return ret;
}

extern "C"
off64_t get_capacity (void *fd)
{ Scsi_Command	cmd(fd);
  return (off64_t)get_2k_capacity(cmd)*2048;
}

ssize_t poor_mans_pwrite64 (int fd,const void *_buff,size_t size,off64_t foff)
{ Scsi_Command		cmd(ioctl_handle);	/* screw first argument */
  unsigned char		bcap[12];
  const unsigned char  *buff=(const unsigned char *)_buff;
  unsigned int		lba,nbl,bsize,bfree;
  int			retries=0,errcode;
  static int		dao_toggle=-1;

    if (foff&0x7FFF || size&0x7FFF)	// 32K block size
	return (errno=EINVAL,-1);

    lba = foff>>11;
    nbl = size>>11;

    if (!media_written && next_wr_addr)
    {	if ((lba+nbl) <= next_wr_addr)
	    return size;
	else if (next_wr_addr > lba)
	    nbl  -= (next_wr_addr-lba),
	    size -= (next_wr_addr-lba)<<11,
	    buff += (next_wr_addr-lba)<<11,
	    lba   = next_wr_addr;
    }
#if defined(__sun) || defined(sun)
    else    next_wr_addr = lba;
#endif

    if (dao_toggle<0) dao_toggle=is_dao;

    { static unsigned int first_wr_addr=0;

	if (!media_written)
	{   sigs_mask (SIG_BLOCK); first_wr_addr = lba;	}
	else if (lba >= (first_wr_addr+buf_size/2)) // measured in 2KBs!
	{   sigs_mask (SIG_UNBLOCK);			}
    }

    while (1)
    {	
      cmd[0] = wrvfy?0x2E:0x2A;	// WRITE [AND VERIFY] (10)
	cmd[2] = (lba>>24)&0xff;	// Logical Block Addrss
	cmd[3] = (lba>>16)&0xff;
	cmd[4] = (lba>>8)&0xff;
	cmd[5] = lba&0xff;
	cmd[7] = (nbl>>8)&0xff;
	cmd[8] = nbl&0xff;
	cmd[9] = 0;
#if 0
	cmd[0] = 0xAA;			// WRITE(12)
	cmd[2] = (lba>>24)&0xff;	// Logical Block Addrss
	cmd[3] = (lba>>16)&0xff;
	cmd[4] = (lba>>8)&0xff;
	cmd[5] = lba&0xff;
	cmd[8] = (nbl>>8)&0xff;
	cmd[9] = nbl&0xff;
	cmd[10] = 0x80;			// "Streaming"
	cmd[11] = 0;
#endif
	//
	// First writes can be long, especially in DAO mode...
	// I wish I could complement this with "if (lba==0),"
	// but some units might choose to fill the buffer before
	// they take the first nap...
	//
	cmd.timeout(dao_toggle?180:60);
	//
	// It should also be noted that under Linux these values
	// (if actually respected by kernel!) can turn out bogus.
	// The problem is that I scale them to milliseconds as
	// documentation requires/implies, while kernel treats
	// them as "jiffies." I could/should have used HZ macro
	// (or sysconf(_SC_CLK_TCK)), but recent kernels maintain
	// own higher HZ value and disrespects the user-land one.
	// Sending them down as milliseconds is just safer...
	//
	if (!(errcode=cmd.transport (WRITE,(void *)buff,size))) {

	  cmd[0] = 0x5C;		// READ BUFFER CAPACITY
	  cmd[8] = sizeof(bcap);
	  cmd[9] = 0;
	  if(!cmd.transport (READ,bcap,sizeof(bcap))) {
	    bsize = bcap[4]<<24|bcap[5]<<16|bcap[6]<<8|bcap[7];
	    bfree = bcap[8]<<24|bcap[9]<<16|bcap[10]<<8|bcap[11];
	    int bufferFill = 100*(bsize-bfree)/bsize;
	    if( bufferFill != last_buffer_fill ) {
	      last_buffer_fill = bufferFill;
	      fprintf (stderr, "Buffer fill: %d\n", last_buffer_fill );
	    }
	  }

	  break;
	}

	//--- WRITE failed ---//
#if defined(__sun) || defined(sun)
	//
	// Solaris can slice USB WRITEs to multiple ones. Here I try
	// to find out which slice has failed. I expect we get here
	// only when we re-enter the loop...
	//
	if (lba==next_wr_addr &&
	    errcode==0x52102)		// "INVALID ADDRESS FOR WRITE"
	{ unsigned char track[32];

	    cmd[0] = 0x52;		// READ TRACK INFORMATION
	    cmd[1] = 1;
	    cmd[4] = next_track>>8;
	    cmd[5] = next_track&0xFF;
	    cmd[8] = sizeof(track);
	    cmd[9] = 0;
	    if (!cmd.transport (READ,track,sizeof(track)))
	    {	if (track[7]&1)	// NWA_V
		{   next_wr_addr  = track[12]<<24;
		    next_wr_addr |= track[13]<<16;
		    next_wr_addr |= track[14]<<8;
		    next_wr_addr |= track[15];
		    if (lba<next_wr_addr && (lba+nbl)>next_wr_addr)
		    {	nbl  -= next_wr_addr-lba,
			size -= (next_wr_addr-lba)<<11,
			buff += (next_wr_addr-lba)<<11,
			lba   = next_wr_addr;
			continue;
		    }
		}
	    }
	}
#endif

	if (errcode==0x20408)		// "LONG WRITE IN PROGRESS"
	{   // Apparently only Pioneer units do this...
	    if (velocity == 0)
	    {	if (handle_events(cmd) & (1<<6))
		    continue;
		goto sync_cache;
	    }

	    cmd[0] = 0x5C;		// READ BUFFER CAPACITY
	    cmd[8] = sizeof(bcap);
	    cmd[9] = 0;
	    if (cmd.transport (READ,bcap,sizeof(bcap)))
		bfree=0, bsize=buf_size;
	    else
	    {	bsize = bcap[4]<<24|bcap[5]<<16|bcap[6]<<8|bcap[7];
		bfree = bcap[8]<<24|bcap[9]<<16|bcap[10]<<8|bcap[11];
		bsize /= 1024, bfree /= 1024;	// xlate to KB
	    }
	    // check for sanity and drain 1/2 of the buffer
	    if (bsize < buf_size/2)	bsize = buf_size/2;
	    else			bsize /= 2;
	    if (bfree > bsize)		bfree = bsize;

	    int msecs=0;

	    if ((msecs=(bsize-bfree)*1000) > velocity)
	    {	msecs /= velocity;
		retries++;
		if (dao_toggle) dao_toggle=-1;
	    }
	    else	// lots of free buffer reported?
	    {	if (dao_toggle)
		{   dao_toggle=-1;
		    if ((handle_events(cmd) & (1<<6)))
			continue;
		    msecs = bsize*1000;
		    msecs /= velocity;
		}
		else if (!retries++)	continue;
	    }

	    if (retries > (dao_toggle?192:96))
		fprintf (stderr,":-? the LUN appears to be stuck "
				"writing LBA=%xh, retry in %dms\n",
				lba,msecs),
		retries=0, msecs--;

	    if (msecs>=0)
	    {	poll (NULL,0,msecs);
		continue;
	    }

	    lba |= 0x80000000;		// signal insane bfree...
	}
	else if (errcode==0x20404 ||	// "FORMAT IN PROGRESS"
		 errcode==0x20407 ||	// "OPERATION IN PROGRESS"
		 errcode==0x52C00)	// "COMMAND SEQUENCE ERROR"
	{   // Happens under automounter control? In general I recommend
	    // to disable it! This code is a tribute to users who disregard
	    // the instructions...
	sync_cache:
	    cmd[0] = 0x35;	// SYNC CACHE
	    cmd[9] = 0;
	    if (!cmd.transport())
	    {	if (++retries > 1)
	    	{   fprintf (stderr,":-! the LUN appears to be stuck at %xh, "
		    		    "retrying in 5 secs...\n",lba),
		    retries=0;
		    sigs_mask (SIG_UNBLOCK);
		    poll (NULL,0,5000);
		}
		else if (errcode==0x52C00)
		    fprintf (stderr,":-! \"COMMAND SEQUENCE ERROR\"@LBA=%xh. "
				    "Is media being read?\n",lba);
		continue;
	    }
	    lba |= 0x40000000;	// signal "can't SYNC CACHE"
	}
	{ char cmdname[64];
	    sprintf (cmdname,"WRITE@LBA=%xh",lba),
	    sperror (cmdname,errcode);
	}
	if (lba==0)
	{   if (ASC(errcode)==0x30)
		fprintf (stderr,":-( media is not formatted or unsupported.\n");
	    else if ((mmc_profile&0xFFFF)==0x14 /*&& ASC(errcode)==0x64*/)
		fprintf (stderr,":-( attempt to re-run with "
				"-dvd-compat -dvd-compat to engage DAO or "
				"apply full blanking procedure\n");
	}
#if 1	// safety net: leave DL sessions open and resumable...
	if ((mmc_profile&0xFFFF)==0x2B && errcode!=0x52100) media_written=0;
#endif
	return -1;
    }

    next_wr_addr = lba+nbl;
    media_written = 1;

    if (dao_toggle<0) dao_toggle=0;

  return size;
}

static void plus_rw_format (Scsi_Command &cmd)
{ int err,i,len;
  unsigned char descr[12];

    if ((formats[4+4]&3) == 1)	// Unformatted media
    {	fprintf (stderr,"%s: pre-formatting blank DVD+RW...\n",ioctl_device);

	for (i=8,len=formats[3];i<len;i+=8)
	    if ((formats [4+i+4]>>2) == 0x26) break;

	if (i==len)
	    fprintf (stderr,":-( can't locate DVD+RW format descriptor\n"),
	    exit(FATAL_START(EMEDIUMTYPE));

	memset (descr,0,sizeof(descr));
	descr[1]=0x02;		// "IMMED" flag
	descr[3]=0x08;		// "Descriptor Length" (LSB)
	memcpy (descr+4,formats+4+i,4);
	descr[8]=0x26<<2;	// "Format type" 0x26

	cmd[0] = 0x04;		// FORMAT UNIT
	cmd[1] = 0x11;		// "FmtData" and "Format Code"
	cmd[5] = 0;
	if ((err=cmd.transport(WRITE,descr,sizeof(descr))))
	    sperror ("FORMAT UNIT",err),
	    exit(FATAL_START(errno));

	wait_for_unit (cmd);

	cmd[0] = 0x35;		// FLUSH CACHE
	cmd[9] = 0;
	if ((err=cmd.transport()))
	    sperror ("FLUSH CACHE",err),
	    exit(FATAL_START(errno));
    }
}

static int plus_rw_restart_format (Scsi_Command &cmd, off64_t size)
{ unsigned char descr[12];
  unsigned int lead_out,blocks;
  int err,i,len;

    if (!dvd_compat && size!=0)
    {	blocks = size/2048;
	blocks += 15, blocks &= ~15;

	lead_out = 0;
	lead_out |= formats[4+0], lead_out <<= 8;
	lead_out |= formats[4+1], lead_out <<= 8;
	lead_out |= formats[4+2], lead_out <<= 8;
	lead_out |= formats[4+3];

	if (blocks<=lead_out)   // no need to resume format...
	    return 0;
    }

    fprintf (stderr,"%s: restarting DVD+RW format...\n",ioctl_device);

    for (i=8,len=formats[3];i<len;i+=8)
	if ((formats [4+i+4]>>2) == 0x26) break;

    if (i==len)
	fprintf (stderr,":-( can't locate DVD+RW format descriptor\n"),
	exit(FATAL_START(EMEDIUMTYPE));

    memset (descr,0,sizeof(descr));
    descr[1]=0x02;		// "IMMED" flag
    descr[3]=0x08;		// "Descriptor Length" (LSB)
#if 1
    memcpy (descr+4,formats+4+i,4);
#else
    memset (descr+4,-1,4);
#endif
    descr[8]=0x26<<2;		// "Format type" 0x26
    descr[11]=1;		// "Restart Format"

    cmd[0] = 0x04;		// FORMAT UNIT
    cmd[1] = 0x11;		// "FmtData" and "Format Code"
    cmd[5] = 0;
    if ((err=cmd.transport(WRITE,descr,sizeof(descr))))
    {	sperror ("RESTART FORMAT",err);
	return 1;
    }

    wait_for_unit(cmd);

  return 0;
}

extern "C"
int plusminus_r_C_parm (void *fd,char *C_parm)
{ unsigned int next_session, prev_session,err;
  Scsi_Command cmd(fd);
  unsigned char buf[36];

    if ((disc_info[2]&3) != 1)
	fprintf (stderr,":-( media is not appendable\n"),
	exit(FATAL_START(EMEDIUMTYPE));

    // allow to resume even --v-- incomplete sessions //
    if (((disc_info[2]>>2)&3) > 1)
	fprintf (stderr,":-( last session is not empty\n"),
	exit(FATAL_START(EMEDIUMTYPE));

    next_track = disc_info[5]|disc_info[10]<<8;

    cmd[0] = 0x52;		// READ TRACK INFORMATION
    cmd[1] = 1;
    cmd[4] = next_track>>8;
    cmd[5] = next_track;	// ask for last track
    cmd[8] = sizeof(buf);
    cmd[9] = 0;
    if ((err=cmd.transport (READ,buf,sizeof(buf))))
    {	sperror ("READ TRACK INFORMATION",err);
	if (ASC(err)==0x24) // hp dvd200i returns 24 if media is full
	    fprintf (stderr,":-( media must be full already\n");
	exit (FATAL_START(errno));
    }

#if 0
    if (buf[7]&1)		// NWA_V
    {	next_session  = buf[12]<<24;
	next_session |= buf[13]<<16;
	next_session |= buf[14]<<8;
	next_session |= buf[15];
    }
    else
	fprintf (stderr,":-( Next Writable Address is invalid\n"),
	exit(FATAL_START(EMEDIUMTYPE));
#else
    next_session  = buf[8]<<24;	// Track Start Address
    next_session |= buf[9]<<16;
    next_session |= buf[10]<<8;
    next_session |= buf[11];
#endif

    //
    // All manuals say the data is fabricated, presumably implying
    // that one should use another command. But we stick to this one
    // because kernel uses this very command to mount multi-session
    // discs.
    //
    cmd[0] = 0x43;		// READ TOC
    cmd[2] = 1;			// "Session info"
    cmd[8] = 12;
    cmd[9] = 0;
    if ((err=cmd.transport (READ,buf,12)))
	sperror ("READ SESSION INFO",err),
	exit (FATAL_START(errno));

    prev_session  = buf[8]<<24;
    prev_session |= buf[9]<<16;
    prev_session |= buf[10]<<8;
    prev_session |= buf[11];

    sprintf (C_parm,"%d,%d",prev_session+16,next_session);

  return next_session;
}

static unsigned char *pull_page2A (Scsi_Command &cmd)
{ unsigned char *page2A,header[12];
  unsigned int   len,bdlen;
  int            err;

    cmd[0] = 0x5A;		// MODE SENSE
    cmd[1] = 0x08;		// "Disable Block Descriptors"
    cmd[2] = 0x2A;		// "Capabilities and Mechanical Status"
    cmd[8] = sizeof(header);	// header only to start with
    cmd[9] = 0;
    if ((err=cmd.transport(READ,header,sizeof(header))))
	sperror ("MODE SENSE#2A",err), exit(FATAL_START(errno));

    len   = (header[0]<<8|header[1])+2;
    bdlen = header[6]<<8|header[7];

    if (bdlen)	// should never happen as we set "DBD" above
    {	if (len < (8+bdlen+30))
	    fprintf (stderr,":-( LUN is impossible to bear with...\n"),
	    exit(FATAL_START(EINVAL));
    }
    else if (len < (8+2+(unsigned int)header[9]))// SANYO does this.
	len = 8+2+header[9];

    page2A = (unsigned char *)malloc(len);
    if (page2A == NULL)
	fprintf (stderr,":-( memory exhausted\n"), exit(FATAL_START(ENOMEM));

    cmd[0] = 0x5A;		// MODE SENSE
    cmd[1] = 0x08;		// "Disable Block Descriptors"
    cmd[2] = 0x2A;		// "Capabilities and Mechanical Status"
    cmd[7] = len>>8;
    cmd[8] = len;		// real length this time
    cmd[9] = 0;
    if ((err=cmd.transport(READ,page2A,len)))
	sperror ("MODE SENSE#2A",err),
	exit(FATAL_START(errno));

    len -= 2;
    if (len < ((unsigned int)page2A[0]<<8|page2A[1]))	// paranoia:-)
	page2A[0] = len>>8, page2A[1] = len;

  return page2A;
}

static int pull_velocity (Scsi_Command &cmd,unsigned char *d)
{ unsigned int len;
  int v,err;
  class autofree perf;

#if 0	// 8x AccessTek derivatives, such as OptoRite DD0405
    if ((d[4]&2) == 0)	return -1;
#endif

    len = (d[0]<<24|d[1]<<16|d[2]<<8|d[3])-4;
    if (len%16)		return -1;
    if (len==0)		return -1;	// LG GCA-4040N:-(

    len += 8;
    if (len == (8+16))
    {	velocity = d[8+ 4]<<24|d[8+ 5]<<16|d[8+ 6]<<8|d[8+ 7];
	v =        d[8+12]<<24|d[8+13]<<16|d[8+14]<<8|d[8+15];
	if (v>velocity) velocity=v;	// CAV?
    }
    else	// ZCLV
    { unsigned int n = (len-8)/16;
      unsigned char *p;

	perf = (unsigned char *)malloc (len);
	if (perf == NULL)
	    fprintf (stderr,":-( memory exhausted\n"),
	    exit(FATAL_START(ENOMEM));

	cmd[0]=0xAC;	// GET PERFORMANCE
	cmd[1]=4;	// ask for "Overall Write Performance"
	cmd[8]=n>>8;
	cmd[9]=n;	// real number of descriptors
	cmd[10]=0;	// ask for descriptor in effect
	cmd[11]=0;
	if ((err=cmd.transport(READ,perf,len)))
	    sperror ("GET CURRENT PERFORMANCE",err),
	    exit (FATAL_START(errno));

	// Pick the highest speed...
	for (p=perf+8,len-=8;len;p+=16,len-=16)
	{   v=p[ 4]<<24|p[ 5]<<16|p[ 6]<<8|p[ 7];
	    if (v > velocity) velocity = v;
	    v=p[12]<<24|p[13]<<16|p[14]<<8|p[15];
	    if (v > velocity) velocity = v;	// ZCAV?
	}
    }

  return 0;
}

static int set_speed_B6h (Scsi_Command &cmd,unsigned int dvddash,
			  unsigned char *page2A)
{ unsigned int len;
  int err;
  unsigned char d[8+16];
  class autofree perf;

    cmd[0]=0xAC;		// GET PERFORMACE
    cmd[9]=1;			// start with one descriptor
    cmd[10]=0x3;		// ask for "Write Speed Descriptor"
    cmd[11]=0;
    if ((err=cmd.transport(READ,d,sizeof(d))))
    {	sperror ("GET PERFORMANCE",err);
	fprintf (stderr,":-( falling down to SET CD SPEED\n");
	return -1;
    }

    len = (d[0]<<24|d[1]<<16|d[2]<<8|d[3])-4;

    if (len%16)			// insane length
    {	fprintf (stderr,":-( GET PERFORMANCE: insane Performance Data Length\n");
	return -1;
    }

    perf = (unsigned char *)malloc(len+=8);
    if (perf == NULL)
	fprintf (stderr,":-( memory exhausted\n"),
	exit(FATAL_START(ENOMEM));

    if (len == sizeof(d))
	memcpy (perf,d,sizeof(d));
    else
    { unsigned int n=(len-8)/16;

	cmd[0]=0xAC;		// GET PERFORMANCE
	cmd[8]=n>>8;
	cmd[9]=n;		// real number of descriptors
	cmd[10]=0x3;		// ask for "Write Speed Descriptor"
	cmd[11]=0;
	if ((err=cmd.transport(READ,perf,len)))
	    sperror ("GET PERFORMANCE",err),
	    exit (FATAL_START(errno));
    }

    int targetv=0,errp=0;
    do {
	memset (d,0,sizeof(d));
	cmd[0]=0xAC;		// GET PERFORMANCE
	cmd[1]=4;		// ask for "Overall Write performance"
	cmd[9]=1;
	cmd[10]=0;		// ask for descriptor in effect
	cmd[11]=0;
	if (errp || (errp=cmd.transport(READ,d,sizeof(d))))
#if 0
	    sperror ("GET CURRENT PERFORMANCE",errp),
	    exit (FATAL_START(errno));	// well, if it passed above, we
					// expect it to pass this too...
#else	// Pioneer doesn't report current speed through GET PERFORMANCE:-(
	{   emulated_err:

	    if (page2A == NULL)	return -1;

	    unsigned int  plen,hlen;

	    plen = (page2A[0]<<8|page2A[1]) + 2;
	    hlen = 8 + (page2A[6]<<8|page2A[7]);

	    unsigned char * const p = page2A + hlen;

	    if (plen<(hlen+32) || p[1]<(32-2))
		return -1;	// well, SET CD SPEED wouldn't work...

	    velocity = p[28]<<8|p[29];
	}
#endif
	else if ((errp = pull_velocity (cmd,d)) < 0)
		goto emulated_err;

	if (speed_factor != 0.0)
	{ int i,j=len-8,v,v0,v1,minv,closesti,closestv=0;
	  unsigned char *wsdp=perf+8;

	    for (minv=0x7fffffff,i=0;i<j;i+=16)
	    {	v=wsdp[i+12]<<24|wsdp[i+13]<<16|wsdp[i+14]<<8|wsdp[i+15];
		if (v<minv) minv=v;
	    } // minv will be treated as 1x

	    // 2.4x is like 1x for DVD+, but it turned out that there're
	    // units, most notably "DVDRW IDE1004," burning DVD+ at
	    // lower speed, so I have to watch out for it...
	    if (!dvddash && minv>(2*ONEX))
		speed_factor /= 2.4;
	    // some units, e.g. NEC ND-2500, offer 2x as minimal speed...
	    else if (minv>=(2*ONEX))
		speed_factor /= 2.0;

	    v0=(int)(speed_factor*minv + 0.5);
	    for (closesti=0,minv=0x7fffffff,i=0;i<j;i+=16)
	    {	v1=wsdp[i+12]<<24|wsdp[i+13]<<16|wsdp[i+14]<<8|wsdp[i+15];
		v=abs(v0-v1);
		if (v<minv) minv=v, closesti=i, closestv=v1;
	    } // currenti is index of descriptor with *closest* velocity

	    targetv=closestv;
	    speed_factor=0.0;

	    if (velocity==targetv)
		break;	// already in shape, nothing to do...

	    unsigned char pd[28];

	    memset (pd,0,sizeof(pd));	// setup "Performance Descriptor"

	    pd[0]=*(wsdp+closesti);	// copy "WRC" and other flags
#if 0
	    memset(pd+8,0xFF,4);
#else	    // I might have to copy the value from current descriptor...
	    unsigned int cap=get_2k_capacity(cmd)-1;

	    pd[8]=cap>>24;
	    pd[9]=cap>>16;
	    pd[10]=cap>>8;
	    pd[11]=cap;
#endif
	    memcpy(pd+12,wsdp+closesti+8,4);	// copy "Read Speed"
	    memcpy(pd+20,wsdp+closesti+12,4);	// copy "Write Speed"
	    pd[18]=pd[26]=1000>>8;		// set both "Read Time" and
	    pd[19]=pd[27]=1000&0xFF;		// "Write Time" to 1000ms

	    cmd[0]=0xB6;		// SET STREAMING
	    cmd[10]=sizeof(pd);
	    cmd[11]=0;
	    if ((err=cmd.transport (WRITE,pd,sizeof(pd))))
		sperror ("SET STREAMING",err),
		exit (FATAL_START(errno));

	    // I pull the Page 2A in either case, because unit might
	    // have provided irrelevant information (Plextor). Not to
	    // mention cases when it simply failed to reply to GET
	    // CURRENT PERFORMANCE (Pioneer, LG).

	    unsigned int plen  = (page2A[0]<<8|page2A[1]) + 2;

	    cmd[0] = 0x5A;	// MODE SENSE
	    cmd[1] = 0x08;	// "Disable Block Descriptors"
	    cmd[2] = 0x2A;	// "Capabilities and Mechanical Status"
	    cmd[7] = plen>>8;
	    cmd[8] = plen;	// real length this time
	    cmd[9] = 0;
	    if ((err=cmd.transport(READ,page2A,plen)))
		sperror ("MODE SENSE#2A",err),
		exit(FATAL_START(errno));
	}
	else if (targetv)
	{   if (targetv!=velocity)
	    {	if (errp==0)	// check Page 2A for speed then...
		{   errp=-1; continue;   }
        	    fprintf (stderr,":-( Failed to change write speed: "
			 "%d->%d\n",velocity,targetv);
		  if (lazy)
		    return velocity;
		exit (FATAL_START(EINVAL));
	    }
	    break;
	}
    } while (targetv);

  return targetv;
}

static int set_speed_BBh (Scsi_Command &cmd,unsigned char *page2A)
{ unsigned int   plen,hlen;
  int err;

    plen  = (page2A[0]<<8|page2A[1]) + 2;
    hlen = 8 + (page2A[6]<<8|page2A[7]);

    unsigned char * const p = page2A + hlen;

    if (plen<(hlen+32) || p[1]<(32-2))
	return -1;

    int targetv=0;
    do {
	velocity = p[28]<<8|p[29];

	if (speed_factor != 0.0)
	{ int i,j,v,v0,v1,minv,closesti,closestv=0;
	  unsigned char *wsdp=p+32;

	    j=(p[30]<<8|p[31])*4;
	    for (minv=0x7fffffff,i=0;i<j;i+=4)
	    {	v=wsdp[i+2]<<8|wsdp[i+3];
		if (v<minv) minv=v;
	    } // minv will be treated as 1x

	    v0=(int)(speed_factor*minv + 0.5);
	    for (closesti=0,minv=0x7fffffff,i=0;i<j;i+=4)
	    {	v1=wsdp[i+2]<<8|wsdp[i+3];
		v=abs(v0-v1);
		if (v<minv) minv=v, closesti=i, closestv=v1;
	    } // currenti is index of descriptor with *closest* velocity

	    targetv=closestv;
	    speed_factor=0.0;

	    if (velocity==targetv)
		break;	// already in shape, nothing to do...

	    cmd[0]=0xBB;		// SET CD SPEED
	    cmd[1]=wsdp[closesti+1];	// Rotation Control
	    cmd[2]=cmd[3]=0xff;		// Read Speed
	    cmd[4]=wsdp[closesti+2];	// Write Speed
	    cmd[5]=wsdp[closesti+3];
	    cmd[11]=0;
	    if ((err=cmd.transport()))
		sperror ("SET CD SPEED",err),
		exit (FATAL_START(errno));

	    cmd[0] = 0x5A;	// MODE SENSE
	    cmd[1] = 0x08;	// "Disable Block Descriptors"
	    cmd[2] = 0x2A;	// "Capabilities and Mechanical Status"
	    cmd[7] = plen>>8;
	    cmd[8] = plen;	// real length this time
	    cmd[9] = 0;
	    if ((err=cmd.transport(READ,page2A,plen)))
		sperror ("MODE SENSE#2A",err),
		exit(FATAL_START(errno));
	}
	else if (targetv)
	  {   
	    if (targetv!=velocity) {
	      fprintf (stderr,":-( Failed to change write speed: %d->%d\n",
		       velocity,targetv);
	      if (lazy)
		return velocity;
	      exit (FATAL_START(EINVAL));
	    }
	    break;
	}
    } while (targetv);

  return targetv;
}

static int plusminus_pages_setup (Scsi_Command &cmd,int profile)
{ unsigned int   plen,hlen,dvddash;
  unsigned char  header[16],p32;
  int            err;
  class autofree page2A;

    switch (profile)
    {	case 0x1A:	// DVD+RW
	    p32 = 0;
	    dvddash = 0;
	    break;
	case 0x1B:	// DVD+R
	    //
	    // Even though MMC-4 draft explicitely exempts DVD+RW/+R media
	    // from those being affected by Page 05h settings, some
	    // firmwares apparently pay attention to Multi-session flag
	    // when finalizing DVD+R media. Well, we probably can't blame
	    // vendors as specification is still a work in progress, not to
	    // mention that it was published after DVD+R was introduced to
	    // the market.
	    //
	    p32 = dvd_compat?0x02:0xC0;
	    dvddash = 0;
	    break;
	case 0x13:	// DVD-RW Restricted Overwrite
	    p32 = 0xC0;	// NB! Test Write in not option here
	    dvddash = 1;
	    break;
	case 0x11:	// DVD-R Sequential
	case 0x14:	// DVD-RW Sequential
	case 0x15:	// DVD-R Dual Layer Sequential
	    p32 = 0xC0;
	    if (next_track==1) do
	    {	if (dvd_compat >= (profile==0x14?2:256))
		{   is_dao = 1,
		    fprintf (stderr,"%s: engaging DVD-%s DAO upon user request...\n",
				    ioctl_device,profile==0x14?"RW":"R");
		    break;
		}

		// Try to figure out if we have to go for DAO...
		cmd[0] = 0x46;	// GET CONFIGURATION
		cmd[1] = 2;	// ask for the only feature...
		cmd[3] = 0x21;	// the "Incremental Streaming Writable" one
		cmd[8] = 16;	// The feature should be there, right?
		cmd[9] = 0;
		if ((err=cmd.transport (READ,header,16)))
		    sperror ("GET FEATURE 21h",err),
		    exit(FATAL_START(errno));

		hlen = header[0]<<24|header[1]<<16|header[2]<<8|header[3];
		// See if Feature 21h is "current," if not, engage DAO...
		if (hlen>=12 && (header[8+2]&1)==0)
		{   is_dao = dvd_compat = 1;
		    fprintf (stderr,"%s: FEATURE 21h is not on, engaging DAO...\n",
				    ioctl_device);
		    break;
		}
	    } while (0);
	
	    if (is_dao)		p32 |=2;	// DAO
	    if (dvd_compat)	p32 &= 0x3F;	// Single-session

	    if (test_write)
		p32 |= 0x10;	// Test Write for debugging purposes

	    dvddash=1;
	    break;
	default:
	    p32 = 0;
	    dvddash = 0;
	    break;
    }

    if (dvddash || p32)	page05_setup (cmd,profile,p32);

    page2A = pull_page2A (cmd);

    plen = (page2A[0]<<8|page2A[1]) + 2;
    hlen = 8 + (page2A[6]<<8|page2A[7]);

    unsigned char * const p = page2A + hlen;

    if (plen<(hlen+14) || p[1]<(14-2))	// no "Buffer Size Supported"
	buf_size = 256;			// bogus value
    else
    	buf_size = (p[12]<<8|p[13]);
    if (buf_size<256)			// bogus value
	buf_size = 256;

    // GET PERFORMANCE/SET STREAMING are listed as mandatory, so that
    // this is actually the most likely path...
    if (set_speed_B6h (cmd,dvddash,page2A) >= 0)
	goto opc;

    if (plen<(hlen+30) || p[1]<(30-2))	// no "Current Write Speed" present
    {	if (dvddash)
	{   fprintf (stderr,":-( \"Current Write Speed\" descriptor "
			    "is not present, faking 1x...\n"),
	    velocity = ONEX;
	    speed_factor = 0.0;
	}
	else
	    velocity = 0;
    }
    else do
    {	velocity = p[28]<<8|p[29];

	// Some units, most notably NEC and derivatives, were observed
	// to report CD-R descriptors...
	if (velocity < ONEX)			// must be bogus
	{   velocity=0;
	    break;
	}

	if (plen<(hlen+32) || p[1]<(32-2))	// no write descriptors
	    break;

	int            n = p[30]<<8|p[31],minv=1<<16;
	unsigned char *p_= p+32;
	for (;n--;p_+=4)
	{   int v=p_[2]<<8|p_[3];
	    if (v==0) continue;		// Lite-On returns zeros here...
	    if (v<ONEX)			// must be bogus
		velocity=0;
	    if (v < minv)
		minv = v;
	}
	if (!dvddash && minv>2*ONEX)
	    // See corresponding comment in set_speed_B6h()
	    speed_factor /= 2.4;

    } while (0);

    if (speed_factor != 0.0 &&
	(velocity==0 || set_speed_BBh (cmd,page2A)<0) )
	fprintf (stderr,":-( can't control writing velocity, "
			"-speed option is ignored\n");
opc:

    if (!dvddash) return 0;

    // See if OPC is required...
    cmd[0] = 0x51;	// READ DISC INFORMATION
    cmd[8] = 8;
    cmd[9] = 0;
    if ((err=cmd.transport (READ,header,8)))
	sperror ("READ DISC INFORMATION",err),
	exit(FATAL_START(errno));

    if ((header[0]<<8|header[1]) <= 0x20)
    {	cmd[0] = 0x54;	// SEND OPC INFORMATION
	cmd[1] = 1;	// "Perform OPC"
	cmd[9] = 0;
	cmd.timeout(120);	// NEC units can be slooo...w
	if ((err=cmd.transport())) do
	{   if (err==0x52000)	// "INVALID COMMAND"
		break;		// LD-F321 doesn't support SEND OPC?
	    if (err==0x52700)	// "WRITE PROTECTED"
		break;		// Plextor PX-712A?
	    if (err==0x17301)	// "POWER CALIBRATION AREA ALMOST FULL"
	    {	fprintf (stderr,":-! WARNING: Power Calibration Area "
	    			    "is almost full\n");
		break;
	    }
	    if (err==0x37303)   // "POWER CALIBRATION AREA ERROR"
	    {  fprintf (stderr, ":-! WARNING: Power Calibration Area "
			        "error\n");
	    if (lazy)
	      break;
	    }
	    sperror ("PERFORM OPC",err),
	    exit(FATAL_START(errno));
	} while (0);
    }

    handle_events(cmd);

  return 0;
}

static int minus_rw_quickgrow (Scsi_Command &cmd,off64_t size)
{ unsigned char	format[12];
  unsigned int	lead_out,blocks,type;
  int		i,len,err;

    type = formats[4+4]&3;

    if (type==2 && size!=0)
    {	blocks = size/2048;
	blocks += 15, blocks &= ~15;

	lead_out = 0;
	lead_out |= formats[4+0], lead_out <<= 8;
	lead_out |= formats[4+1], lead_out <<= 8;
	lead_out |= formats[4+2], lead_out <<= 8;
	lead_out |= formats[4+3];

	if (blocks<=lead_out)	// no need to grow the session...
	    return 0;
    }

    // look for Quick Grow descriptor...
    for (i=8,len=formats[3];i<len;i+=8)
	if ((formats [4+i+4]>>2) == 0x13) break;

    if (i==len)			// no Quick Grow descriptor
    {	if (type != 2)
	    quickgrown=1;	// in reality quick formatted...
	return 0;
    }
    else
    {	blocks = 0;
	blocks |= formats[i+0], blocks <<= 8;
	blocks |= formats[i+1], blocks <<= 8;
	blocks |= formats[i+2], blocks <<= 8;
	blocks |= formats[i+3];
	if (type==2 && blocks==0)	// nowhere no grow...
	    return 0;
    }

    quickgrown=1;
    fprintf (stderr,"%s: \"Quick Grow\" session...\n",ioctl_device);

    memset (format,0,sizeof(format));
    format [1] = 2;		// "IMMED"
    format [3] = 8;		// "Length"
    format [8] = 0x13<<2;	// "Quick Grow"
    format [11] = 16;

    cmd[0] = 0x4;		// FORMAT UNIT
    cmd[1] = 0x11;
    cmd[5] = 0;
    if ((err=cmd.transport (WRITE,format,sizeof(format))))
    {	sperror ("QUICK GROW",err);
	return 1;
    }

  return wait_for_unit (cmd);
}

static int minus_r_reserve_track (Scsi_Command &cmd,off64_t size)
{ int          err;
  unsigned int blocks;

    blocks = size/2048;
    blocks += 15, blocks &= ~15;

    fprintf (stderr,"%s: reserving %u blocks",ioctl_device,blocks);
    if (is_dao && blocks<380000)
	fprintf (stderr,"\b, warning for short DAO recording"),
	poll (NULL,0,3000);
    fprintf (stderr,"\n");
			

    cmd[0] = 0x53;		// RESERVE TRACK
    cmd[5] = blocks>>24;
    cmd[6] = blocks>>16;
    cmd[7] = blocks>>8;
    cmd[8] = blocks;
    cmd[9] = 0;
    if ((err=cmd.transport ()))
    {	sperror ("RESERVE TRACK",err);
	return 1;
    }

  return 0;
}

static void plus_r_dl_split (Scsi_Command &cmd,off64_t size)
{ int           err;
  unsigned int  blocks,split;
  unsigned char dvd_20[4+8];

    cmd[0] = 0xAD;	// READ DVD STRUCTURE
    cmd[7] = 0x20;	// "DVD+R Double Layer Boundary Information"
    cmd[9] = sizeof(dvd_20);
    cmd[11] = 0;
    if ((err=cmd.transport(READ,dvd_20,sizeof(dvd_20))))
    	sperror ("READ DVD+R DL BOUNDARY INFORMATION",err),
	exit (FATAL_START(errno));

    if ((dvd_20[0]<<8|dvd_20[1]) < 10) 
	fprintf (stderr,":-( insane DVD+R DL BI structure length\n"),
	exit (FATAL_START(EINVAL));

    if (dvd_20[4]&0x80)
    {	fprintf (stderr,":-? L0 Data Zone Capacity is set already\n");
	return;
    }

    split = dvd_20[8]<<24|dvd_20[9]<<16|dvd_20[10]<<8|dvd_20[11];

    blocks = size/2048;
    blocks += 15, blocks &= ~15;

    if (blocks <= split)
	fprintf (stderr,":-( more than 50%% of space will be *wasted*!\n"
			"    use single layer media for this recording\n"),
	exit (FATAL_START(EMEDIUMTYPE));

    blocks /= 16;
    blocks += 1;
    blocks /= 2;
    blocks *= 16;

    fprintf (stderr,"%s: splitting layers at %u blocks\n",
		    ioctl_device,blocks);

    memset (dvd_20,0,sizeof(dvd_20));
    dvd_20[1]  = sizeof(dvd_20)-2;
    dvd_20[8]  = blocks>>24;
    dvd_20[9]  = blocks>>16;
    dvd_20[10] = blocks>>8;
    dvd_20[11] = blocks;

    cmd[0] = 0xBF;	// SEND DVD STRUCTURE
    cmd[7] = 0x20;	// "DVD+R Double Layer Recording Information"
    cmd[9] = sizeof(dvd_20);
    cmd[11] = 0;
    if ((err=cmd.transport(WRITE,dvd_20,sizeof(dvd_20))))
	sperror ("SEND DVD+R DOUBLE LAYER RECORDING INFORMATION",err),
	exit (FATAL_START(errno));

}

static int flush_cache (Scsi_Command &cmd)
{ int err;

    cmd[0] = 0x35;		// FLUSH CACHE
    cmd[1] = 0x02;		// "IMMED"
    cmd[9] = 0;
    if (!(err=cmd.transport()))
	wait_for_unit (cmd);
    else
	sperror ("FLUSH CACHE",err);

#if 1	// Pioneer apparently needs this, non-IMMED FLUSH that is...
    cmd[0] = 0x35;		// FLUSH CACHE
    cmd[9] = 0;
    if (is_dao) cmd.timeout (15*60);
    if ((err=cmd.transport()))
    {	sperror ("SYNCHRONOUS FLUSH CACHE",err);
	return err;
    }
#endif

  return 0;
}

//
// atexit/signal handlers
//
extern "C"
void no_r_finalize ()
{   while (media_written)
    { Scsi_Command cmd(ioctl_handle);

	fprintf (stderr,"%s: flushing cache\n",ioctl_device);
	if (flush_cache (cmd))	break;

	media_written = 0;
	errno = 0;
    }
    _exit (errno);
}

extern "C"
void plus_rw_finalize ()
{   sigs_mask(SIG_BLOCK);
    while (media_written)
    { Scsi_Command cmd(ioctl_handle);
      int err;

	fprintf (stderr,"%s: flushing cache\n",ioctl_device);
	if (flush_cache (cmd))	break;

	if (!dvd_compat)
	{   fprintf (stderr,"%s: stopping de-icing\n",ioctl_device);
	    cmd[0] = 0x5B;	// CLOSE TRACK/SESSION
	    cmd[1] = 0x01;	// "IMMED"
	    cmd[2] = 0;		// "Stop De-Icing"
	    cmd[9] = 0;
	    if ((err=cmd.transport()))
		sperror ("STOP DE-ICING",err);

	    if (wait_for_unit (cmd)) break;
	}

	fprintf (stderr,"%s: writing lead-out\n",ioctl_device);
	cmd[0] = 0x5B;		// CLOSE TRACK/SESSION
	cmd[1] = 0x01;		// "IMMED"
	cmd[2] = 0x02;		// "Close session"
	cmd[9] = 0;
	if ((err=cmd.transport()))
	    sperror ("CLOSE SESSION",err);

	if (wait_for_unit (cmd)) break;
 
	media_written = 0;
	next_wr_addr = 0;
	errno = 0;

	if (do_reload && media_reload())
	    perror (":-( unable to reload tray");// not actually reached
    }
    _exit (errno);
}

extern "C"
void plus_r_finalize ()
{   sigs_mask(SIG_BLOCK);
    while (media_written)
    { Scsi_Command cmd(ioctl_handle);
      int          mode,err;

	fprintf (stderr,"%s: flushing cache\n",ioctl_device);
	if (flush_cache(cmd))	break;

	fprintf (stderr,"%s: closing track\n",ioctl_device);
	cmd[0] = 0x5B;		// CLOSE TRACK/SESSION
	cmd[1] = 0x01;		// "IMMED"
	cmd[2] = 0x01;		// "Close Track"
	cmd[4] = next_track>>8;
	cmd[5] = next_track;
	cmd[9] = 0;
	if ((err=cmd.transport()))
	    sperror ("CLOSE TRACK",err);

	if (wait_for_unit (cmd)) break;

	if (dvd_compat)
	    // HP dvd520n insists on "finalize with minimum radius"
	    // with DVD+R DL ---------------vv
	    mode = ((mmc_profile&0xFFFF)==0x2B)?0x05:0x06,
	    fprintf (stderr,"%s: closing disc\n",ioctl_device);
	else
	    mode = 0x02,
	    fprintf (stderr,"%s: closing session\n",ioctl_device);

	cmd[0] = 0x5B;		// CLOSE TRACK/SESSION
	cmd[1] = 0x01;		// "IMMED"
	cmd[2] = mode;		// "Close session"
	cmd[9] = 0;
	
	// it seems, that pioneer is a bit crappy
	while (err=cmd.transport()) {
	    if (SK(err)==0x2 && ASC(err)==0x04 && ASCQ(err)==0x07) {
		    sperror ("CLOSE SESSION (but try to continue)",err);
		    usleep(10000);
	    } else {
		    sperror ("CLOSE SESSION",err);
		    break;
	    }
	}

	if (wait_for_unit (cmd)) break;
 
	media_written = 0;
	errno = 0;

	if (do_reload && media_reload())
	    perror (":-( unable to reload tray");// not actually reached
    }
    _exit (errno);
}

extern "C"
void minus_rw_finalize ()
{   sigs_mask(SIG_BLOCK);
    while (media_written)
    { Scsi_Command cmd(ioctl_handle);
      int err;

	fprintf (stderr,"%s: flushing cache\n",ioctl_device);
	if (flush_cache(cmd))	break;

	if (quickgrown)
	{   fprintf (stderr,"%s: writing lead-out\n",ioctl_device);
	    cmd[0] = 0x5B;		// CLOSE TRACK/SESSION
	    cmd[1] = 0x01;		// "IMMED"
	    cmd[2] = 0x02;		// "Close Session"
	    cmd[9] = 0;
	    if ((err=cmd.transport()))
		sperror ("CLOSE SESSION",err);

	    if (wait_for_unit (cmd)) break;

	    quickgrown=0;
	}

	media_written = 0;
	next_wr_addr = 0;
	errno = 0;

	if (do_reload)
	{   if (media_reload())
		perror (":-( unable to reload tray");// not actually reached
	}
	else return;	// Restricted Overwrite is just ugly!
    }
    _exit (errno);
}

extern "C"
void minus_r_finalize ()
{   sigs_mask(SIG_BLOCK);
    while (media_written)
    { Scsi_Command cmd(ioctl_handle);
      int err;

	fprintf (stderr,"%s: flushing cache\n",ioctl_device);
	if (flush_cache(cmd))	break;

	if (!is_dao)
	{   fprintf (stderr,"%s: updating RMA\n",ioctl_device);
	    cmd[0] = 0x5B;	// CLOSE TRACK/SESSION
	    cmd[1] = 0x01;	// "IMMED"
	    cmd[2] = 0x01;	// "Close Track"
	    cmd[4] = next_track>>8;
	    cmd[5] = next_track;
	    cmd[9] = 0;
	    if ((err=cmd.transport()))
		sperror ("CLOSE TRACK",err);

	    if (wait_for_unit (cmd)) break;

	    fprintf (stderr,"%s: closing %s\n",ioctl_device,
			    dvd_compat?"disc":"session");
	    cmd[0] = 0x5B;	// CLOSE TRACK/SESSION
	    cmd[1] = 0x01;	// "IMMED"
	    cmd[2] = 0x02;	// "Close Session"
	    cmd[9] = 0;
	    if ((err=cmd.transport()))
		sperror (dvd_compat?"CLOSE DISC":"CLOSE SESSION",err);

	    if (wait_for_unit (cmd)) break;
	}
 
	media_written = 0;
	errno = 0;

	if (do_reload && media_reload())
	    perror (":-( unable to reload tray");// not actually reached
    }
    _exit (errno);
}

extern "C"
void ram_reload ()
{   sigs_mask(SIG_BLOCK);
    if (media_written && do_reload) media_reload();
    _exit (errno);
}

//
// poor_mans_setup takes care of a lot of things.
// It's invoked right before first write and if necessary/applicable
// prepares Page 05, reserves track, sets up atexit and signal handlers.
//
#define sigit(SIG,f)	do {	\
    sigaction (SIG,NULL,&sa);	\
    sa.sa_mask = mask;		\
    sa.sa_handler = f;		\
    sa.sa_flags &= ~SA_NODEFER;	\
    sa.sa_flags |= SA_RESETHAND;\
    sigaction (SIG,&sa,NULL);	\
} while (0)
static void atsignals(void(*func)())
{ void           (*f)(int)=(void(*)(int))func;
  sigset_t         mask;
  struct sigaction sa;

    sigemptyset (&mask);
    sigaddset (&mask,SIGHUP),	sigaddset (&mask,SIGINT),
    sigaddset (&mask,SIGTERM),	sigaddset (&mask,SIGPIPE);

    sigit (SIGHUP,f);	sigit (SIGINT,f);
    sigit (SIGTERM,f);	sigit (SIGPIPE,f);
}
#undef sigit

typedef ssize_t (*pwrite64_t)(int,const void *,size_t,off64_t);

extern "C"
pwrite64_t poor_mans_setup (void *fd,off64_t leadout)
{ Scsi_Command cmd(ioctl_handle=fd);
  int err,profile=mmc_profile&0xFFFF;

    // We might have loaded media ourselves, in which case we
    // should lock the door...
    cmd[0] = 0x1E;  // PREVENT/ALLOW MEDIA REMOVAL
    cmd[4] = 1;     // "Prevent"
    cmd[5] = 0;
    if ((err=cmd.transport ()))
	sperror ("PREVENT MEDIA REMOVAL",err),
	exit (FATAL_START(errno));

    handle_events(cmd);

    switch (profile)
    {	case 0x1A:	// DVD+RW
	    switch (disc_info[7]&3)	// Background formatting
	    {	case 0:	// blank
			plus_rw_format (cmd);
			break;
		case 1:	// suspended
			plus_rw_restart_format (cmd,leadout);
			break;
		case 2:	// in progress
		case 3: // complete
			break;
	    }

	    plusminus_pages_setup (cmd,profile);
	    atexit (plus_rw_finalize);
	    atsignals (plus_rw_finalize);
	    break;
	case 0x1B:	// DVD+R
	case 0x2B:	// DVD+R Double Layer
	    plusminus_pages_setup(cmd,profile);
	    if (profile==0x2B && next_track==1 && dvd_compat && leadout)
		plus_r_dl_split (cmd,leadout);
	    atexit (plus_r_finalize);
	    if (next_wr_addr)
	    {	atsignals (no_r_finalize);
		next_wr_addr=(unsigned int)-1;
	    }
	    else atsignals (plus_r_finalize);
	    break;
	case 0x13:	// DVD-RW Restricted Overwrite
	    plusminus_pages_setup (cmd,profile);
	    //
	    // A side note. Quick Grow can't be performed earlier,
	    // as then reading is not possible.
	    //
	    minus_rw_quickgrow (cmd,leadout);
	    atexit (minus_rw_finalize);
	    atsignals (minus_rw_finalize);
	    break;
	case 0x11:	// DVD-R Sequential
	case 0x14:	// DVD-RW Sequential
	case 0x15:	// DVD-R Dual Layer Sequential
	    plusminus_pages_setup (cmd,profile);
	    if (is_dao && leadout)
		minus_r_reserve_track(cmd,leadout);
	    atexit (minus_r_finalize);
	    if (next_wr_addr)
	    {	atsignals (no_r_finalize);
		next_wr_addr=(unsigned int)-1;
	    }
	    else atsignals (minus_r_finalize);
	    break;
	case 0x12:	// DVD-RAM
	    // exclusively for speed settings...
	    plusminus_pages_setup (cmd,profile);
	    atexit (ram_reload);
	    atsignals (ram_reload);
	    break;
	default:
	    fprintf (stderr,":-( mounted media[%X] is not supported\n",profile),
	    exit(FATAL_START(EMEDIUMTYPE));
	    break;
    }

    if (velocity)
	fprintf (stderr,"%s: \"Current Write Speed\" is %.1fx1385KBps.\n",
			ioctl_device,velocity/(double)ONEX);

    if (next_wr_addr==(unsigned int)-1) do
    { unsigned char track[32];

	next_wr_addr = 0;

	cmd[0] = 0x52;	// READ TRACK INFORMATION
	cmd[1] = 1;
	cmd[4] = next_track>>8;
	cmd[5] = next_track&0xFF;	// last track, set up earlier
	cmd[8] = sizeof(track);
	cmd[9] = 0;
	if (cmd.transport (READ,track,sizeof(track)))
	    break;

	if (track[7]&1)	// NWA_V
	{   next_wr_addr  = track[12]<<24;
	    next_wr_addr |= track[13]<<16;
	    next_wr_addr |= track[14]<<8;
	    next_wr_addr |= track[15];
	}

	if (next_wr_addr != // track start
	    (unsigned int)(track[8]<<24|track[9]<<16|track[10]<<8|track[11]))
	    fprintf (stderr,":-? resuming track#%d from LBA#%d\n",
			    next_track,next_wr_addr);

    } while (0);
    else next_wr_addr = 0;

  return poor_mans_pwrite64;
}

extern "C"
int poor_man_rewritable (void *fd, void *buf)
{ Scsi_Command	cmd(fd);
  int		err,profile=mmc_profile&0xFFFF;

    if (profile!=0x13 &&	// not DVD-RW Restricted Overwrite
	profile!=0x12 &&	// nor DVD-RAM
	profile!=0x1A)		// nor DVD+RW
	return 0;

    if (profile==0x13)		// DVD-RW Restricted Overwrite
    {	// Yet another restriction of Restricted Overwrite mode?
	// Pioneer DVR-x05 can't read a bit otherwise...
	do_reload=0;
	minus_rw_finalize ();	// with do_reload==0!
	do_reload=1;
    }
    else if (profile==0x1A)	// DVD+RW
    {	fprintf (stderr,"%s: flushing cache\n",ioctl_device);
	if (flush_cache(cmd))	exit (errno);
    }

    cmd[0] = 0x28;	// READ(10)
    cmd[5] = 16;	// LBA#16, volume descriptor set
    cmd[8] = 16;	// 32KB
    cmd[9] = 0;
    if ((err=cmd.transport (READ,buf,16*2048)))
	sperror ("READ@LBA=10h",err),
	exit(errno);

  return 1;
}
