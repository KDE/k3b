/*
 * DVD±RW format 4.10 by Andy Polyakov <appro@fy.chalmers.se>.
 *
 * Use-it-on-your-own-risk, GPL bless...
 *
 * For further details see http://fy.chalmers.se/~appro/linux/DVD+RW/.
 *
 * Revision history:
 *
 * 2.0:
 * - deploy "IMMED" bit in "FORMAT UNIT";
 * 2.1:
 * - LP64 fix;
 * - USB workaround;
 * 3.0:
 * - C++-fication for better portability;
 * - SYSV signals for better portability;
 * - -lead-out option for improved DVD+RW compatibility;
 * - tested with SONY DRU-500A;
 * 4.0:
 * - support for DVD-RW formatting and blanking, tool name becomes
 *   overloaded...
 * 4.1:
 * - re-make it work under Linux 2.2 kernel;
 * 4.2:
 * - attempt to make DVD-RW Quick Format truly quick, upon release
 *   is verified to work with Pioneer DVR-x05;
 * - media reload is moved to growisofs where is actually belongs;
 * 4.3:
 * - -blank to imply -force;
 * - reject -blank in DVD+RW context and -lead-out in DVD-RW;
 * 4.4:
 * - support for -force=full in DVD-RW context;
 * - ask unit to perform OPC if READ DISC INFORMATION doesn't return
 *   any OPC descriptors;
 * 4.5:
 * - increase timeout for OPC, NEC multi-format derivatives might
 *   require more time to fulfill the OPC procedure;
 * 4.6:
 * - -force to ignore error from READ DISC INFORMATION;
 * - -force was failing under FreeBSD with 'unable to unmount';
 * - undocumented -gui flag to ease progress indicator parsing for
 *   GUI front-ends;
 * 4.7:
 * - when formatting DVD+RW, Pioneer DVR-x06 remained unaccessible for
 *   over 2 minutes after dvd+rw-format exited and user was frustrated
 *   to poll the unit himself, now dvd+rw-format does it for user;
 * 4.8:
 * - DVD-RW format fails if preceeded by dummy recording;
 * - make sure we talk to MMC unit, be less intrusive;
 * - unify error reporting;
 * - permit for -lead-out even for blank DVD+RW, needed(?) for SANYO
 *   derivatives;
 * 4.9:
 * - permit for DVD-RW blank even if format descriptors are not present;
 * 4.10:
 * - add support for DVD-RAM;
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <errno.h>

#include "transport.hxx"

static void usage (char *prog)
{ fprintf (stderr,"- usage: %s [-force[=full]] [-lead-out|-blank[=full]]\n"
		  "         [-ssa[=none|default|max]] /dev/cdrom\n",prog),
  exit(1);
}

static volatile int *progress;
static const char   *gui_action=NULL;

extern "C" void alarm_handler (int no)
{ static int         i=0,len=1,old_progress=0;
  static const char *str = "|/-\\",*backspaces="\b\b\b\b\b\b\b\b\b\b";
  int new_progress = *progress;

    if (gui_action)
    {	fprintf (stderr,"* %s %.1f%%\n",gui_action,
			100.0*new_progress/65536.0);
	alarm(3);
	return;
    }

    if (new_progress != old_progress)
        len = fprintf (stderr,"%.*s%.1f%%",len,backspaces,
				100.0*new_progress/65536.0) - len,
	old_progress = new_progress;
    else
        fprintf (stderr,"\b%c",str[i]),
	i++, i&=3;

    alarm(1);
}

int main (int argc, char *argv[])
{ Scsi_Command	cmd;
  unsigned char	formats[260],dinfo[32],inq[128];
  char		*dev=NULL,*p;
  unsigned int	capacity,lead_out,mmc_profile,err;
  int		force=0,full=0,compat=0,blank=0,ssa=0,do_opc=0,gui=0,len,i;
  pid_t		pid;

    fprintf (stderr,"* DVD±RW/-RAM format utility by <appro@fy.chalmers.se>, "
		    "version 4.10.\n");

    for (i=1;i<argc;i++) {
	if (*argv[i] == '-')
	    if (argv[i][1] == 'f')	// -format|-force
	    {	force = 1;
		if ((p=strchr(argv[i],'=')) && p[1]=='f') full=1;
	    }
	    else if (argv[i][1] == 'l')	// -lead-out
		force = compat = 1;
	    else if (argv[i][1] == 'b')	// -blank
	    {	blank=0x11;
		if ((p=strchr(argv[i],'=')) && p[1]=='f') blank=0x10;
		force=1;
	    }
	    else if (argv[i][1] == 's')	// -ssa
	    {	force=ssa=1;
		if ((p=strchr(argv[i],'=')))
		{   if (p[1]=='n')	ssa=-1;	// =none
		    else if (p[1]=='d')	ssa=2;	// =default
		    else if (p[3]=='x')	ssa=3;	// =max
		    else		ssa=0;	// invalid
		}
	    }
	    else if (argv[i][1] == 'g')	gui=1;
	    else			usage(argv[0]);
	else if (*argv[i] == '/')
	    dev = argv[i];
	else
	    usage (argv[0]);
    }

    if (dev==NULL) usage (argv[0]);

    if (!cmd.associate(dev))
	fprintf (stderr,":-( unable to open(\"%s\"): ",dev), perror (NULL),
	exit(1);

    cmd[0] = 0x12;	// INQUIRY
    cmd[4] = 36;
    cmd[5] = 0;
    if ((err=cmd.transport(READ,inq,36)))
	sperror ("INQUIRY",err), exit (1);

    if ((inq[0]&0x1F) != 5)
	fprintf (stderr,":-( not an MMC unit!\n"),
	exit (1);

    cmd[0] = 0x46;		// GET CONFIGURATION
    cmd[8] = 8;
    cmd[9] = 0;
    if ((err=cmd.transport(READ,formats,8)))
	sperror ("GET CONFIGURATION",err), exit (1);

    mmc_profile = formats[6]<<8|formats[7];

    if (mmc_profile!=0x1A
	&& mmc_profile!=0x14 && mmc_profile!=0x13
	&& mmc_profile!=0x12)
	fprintf (stderr,":-( mounted media doesn't appear to be "
			"DVD±RW or DVD-RAM\n"),
	exit (1);

    /*
     * First figure out how long the actual list is. Problem here is
     * that (at least Linux) USB units absolutely insist on accurate
     * cgc.buflen and you can't just set buflen to arbitrary value
     * larger than actual transfer length.
     */
    int once=1;
    do
    {	cmd[0] = 0x23;		// READ FORMAT CAPACITIES
	cmd[8] = 4;
	cmd[9] = 0;
	if ((err=cmd.transport(READ,formats,4)))
	{   if (err==0x62800 && once)	// "MEDIUM MAY HAVE CHANGED"
	    {	cmd[0] = 0;		// TEST UNIT READY
		cmd[5] = 0;
		cmd.transport();	// just swallow it...
		continue;
	    }
	    sperror ("READ FORMAT CAPACITIES",err), exit (1);
	}
    } while (once--);

    len = formats[3];
    if (len&7 || len<8)
	fprintf (stderr,":-( allocation length isn't sane\n"),
	exit(1);

    cmd[0] = 0x23;		// READ FORMAT CAPACITIES
    cmd[7] = (4+len)>>8;	// now with real length...
    cmd[8] = (4+len)&0xFF;
    cmd[9] = 0;
    if ((err=cmd.transport(READ,formats,4+len)))
	sperror ("READ FORMAT CAPACITIES",err), exit (1);

    if (len != formats[3])
	fprintf (stderr,":-( parameter length inconsistency\n"),
	exit(1);

    if (mmc_profile==0x1A)	// DVD+RW
    {	for (i=8;i<len;i+=8)	// look for "DVD+RW Full" descriptor
	    if ((formats [4+i+4]>>2) == 0x26) break;
    }
    else if (mmc_profile==0x12)	// DVD-RAM
    { unsigned int   v,ref;
      unsigned char *f,descr=0x01;
      int j;

	switch (ssa)
	{   case -1:	// no ssa
		for (ref=0,i=len,j=8;j<len;j+=8)
		{   f = formats+4+j;
		    if ((f[4]>>2) == 0x00)
		    {	v=f[0]<<24|f[1]<<16|f[2]<<8|f[3];
			if (v>ref) ref=v,i=j;
		    }
		}
		break;
	    case 0:
		i=8;	// just grab the first descriptor?
		break;
	    case 1:	// first ssa
		for (i=8;i<len;i+=8)
		    if ((formats[4+i+4]>>2) == 0x01) break;
		break;
	    case 2:	// default ssa
		descr=0x00;
	    case 3:	// max ssa
		for (ref=0xFFFFFFFF,i=len,j=8;j<len;j+=8)
		{   f = formats+4+j;
		    if ((f[4]>>2) == descr)
		    {	v=f[0]<<24|f[1]<<16|f[2]<<8|f[3];
			if (v<ref) ref=v,i=j;
		    }
		}
		break;
	}
    }
    else			// DVD-RW
    { int descr=full?0x10:0x15;
	for (i=8;i<len;i+=8)	// look for "DVD-RW Quick" descriptor
	    if ((formats [4+i+4]>>2) == descr) break;
	if (descr==0x15 && i==len)
	{   fprintf (stderr,":-( failed to locate \"Quick Format\" descriptor.\n");
	    for (i=8;i<len;i+=8)// ... alternatively for "DVD-RW Full"
		if ((formats [4+i+4]>>2) == 0x10) break;
	}
    }

    if (i==len)
    {	fprintf (stderr,":-( can't locate appropriate format descriptor\n");
	if (blank)	i=0;
	else		exit(1);
    }

    capacity = 0;
    capacity |= formats[4+i+0], capacity <<= 8;
    capacity |= formats[4+i+1], capacity <<= 8;
    capacity |= formats[4+i+2], capacity <<= 8;
    capacity |= formats[4+i+3];

    if (mmc_profile==0x1A)	// DVD+RW
	fprintf (stderr,"* %.1fGB DVD+RW media detected.\n",
			2048.0*capacity/1e9);
    else if (mmc_profile==0x12)	// DVD-RAM
	fprintf (stderr,"* %.1fGB DVD-RAM media detected.\n",
			2048.0*capacity/1e9);
    else			// DVD-RW
	fprintf (stderr,"* %.1fGB DVD-RW media in %s mode detected.\n",
			2048.0*capacity/1e9,
			mmc_profile==0x13?"Restricted Overwrite":"Sequential");

    lead_out = 0;
    lead_out |= formats[4+0], lead_out <<= 8;
    lead_out |= formats[4+1], lead_out <<= 8;
    lead_out |= formats[4+2], lead_out <<= 8;
    lead_out |= formats[4+3];

    cmd[0] = 0x51;		// READ DISC INFORMATION
    cmd[8] = sizeof(dinfo);
    cmd[9] = 0;
    if ((err=cmd.transport(READ,dinfo,sizeof(dinfo))))
    {	sperror ("READ DISC INFORMATION",err);
	if (!force) exit(1);
	memset (dinfo,0xff,sizeof(dinfo));
	cmd[0] = 0x35;
	cmd[9] = 0;
	cmd.transport();
    }

    do_opc = ((dinfo[0]<<8|dinfo[1])<=0x20);

    if (dinfo[2]&3)		// non-blank media
    {	if (!force)
	{   if (mmc_profile==0x1A || mmc_profile==0x13 || mmc_profile==0x12)
		fprintf (stderr,"- media is already formatted, lead-out is currently at\n"
				"  %d KiB which is %.1f%% of total capacity.\n",
				lead_out*2,(100.0*lead_out)/capacity);
	    else
		fprintf (stderr,"- media is not blank\n");
	offer_options:
	    if (mmc_profile == 0x1A)
		fprintf (stderr,"- you have the option to re-run %s with:\n"
				"  -lead-out  to elicit lead-out relocation for better\n"
				"             DVD-ROM compatibility, data is not affected;\n"
				"  -force     to enforce new format (not recommended)\n"
				"             and wipe the data.\n",
				argv[0]);
	    else if (mmc_profile == 0x12)
		fprintf (stderr,"- you have the option to re-run %s with:\n"
				"  -format=full  to perform full (lengthy) reformat;\n"
				"  -ssa[=none|default|max]\n"
				"                to grow, eliminate, reset to default or\n"
				"                maximize Supplementary Spare Area.\n",
				argv[0]);
	    else
	    {	fprintf (stderr,"- you have the option to re-run %s with:\n",
				argv[0]);
		if (i) fprintf (stderr,
				"  -force[=full] to enforce new format or mode transition\n"
				"                and wipe the data;\n");
		fprintf (stderr,"  -blank[=full] to change to Sequential mode.\n");
	    }
	    exit (0);
	}
	else if (cmd.umount())
	    perror (errno==EBUSY ? ":-( unable to proceed with format" :
				   ":-( unable to umount"),
	    exit (1);
    }
    else
    {	if (mmc_profile==0x14 && blank==0 && !force)
	{   fprintf (stderr,"- media is blank\n");
	    fprintf (stderr,"- given the time to apply full blanking procedure I've chosen\n"
			    "  to insist on -force option upon mode transition.\n");
	    exit (0);
	}
	force = 0;
    }

    if ((mmc_profile == 0x1A && blank)
	|| (mmc_profile != 0x1A && compat)
	|| (mmc_profile != 0x12 && ssa) )
    {	fprintf (stderr,"- illegal command-line option for this media.\n");
	goto offer_options;
    }
    else if (mmc_profile == 0x1A && full)
    {	fprintf (stderr,"- unimplemented command-line option for this media.\n");
	goto offer_options;
    }

    { int fd;
      char *s;

	if ((fd=mkstemp (s=strdup("/tmp/dvd+rw.XXXXXX"))) < 0)
	    fprintf (stderr,":-( unable to mkstemp(\"%s\")",s),
	    exit(1);

	ftruncate(fd,sizeof(*progress));
	unlink(s);

	progress = (int *)mmap(NULL,sizeof(*progress),PROT_READ|PROT_WRITE,
				MAP_SHARED,fd,0);
	close (fd);
	if (progress == MAP_FAILED)
            perror (":-( unable to mmap anonymously"),
	    exit(1);
    }
    *progress = 0;

    if ((pid=fork()) == (pid_t)-1)
	perror (":-( unable to fork()"),
	exit(1);

    if (pid)
    { struct sigaction sa;
      const char *str;

	cmd.~Scsi_Command();
	if (compat && force)	str="relocating lead-out";
	else if (blank)		str="blanking";
	else			str="formatting";
	if (gui)		gui_action=str;
	else			fprintf (stderr,"* %s .",str);

	sigaction (SIGALRM,NULL,&sa);
	sa.sa_flags &= ~SA_RESETHAND;
	sa.sa_flags |= SA_RESTART;
	sa.sa_handler = alarm_handler;
	sigaction (SIGALRM,&sa,NULL);
	alarm(1);
	while ((waitpid(pid,&i,0) != pid) && !WIFEXITED(i)) ;
	if (WEXITSTATUS(i) == 0) fprintf (stderr,"\n");
	exit (0);
    }

    /*
     * You can suspend, terminate, etc. the parent. We will keep on
     * working in background...
     */
    setsid();
    signal(SIGHUP,SIG_IGN);
    signal(SIGINT,SIG_IGN);
    signal(SIGTERM,SIG_IGN);

    // formats[i] becomes "Format Unit Parameter List"
    formats[i+0] = 0;		// "Reserved"
    formats[i+1] = 2;		// "IMMED" flag
    formats[i+2] = 0;		// "Descriptor Length" (MSB)
    formats[i+3] = 8;		// "Descriptor Length" (LSB)

    handle_events(cmd);

    if (mmc_profile==0x1A)	// DVD+RW
    {	if (compat && force && (dinfo[2]&3))
	    formats[i+4+0]=formats[i+4+1]=formats[i+4+2]=formats[i+4+3]=0,
	    formats[i+4+7] = 1;	// "Restart format"

	cmd[0] = 0x04;		// FORMAT UNIT
	cmd[1] = 0x11;		// "FmtData" and "Format Code"
	cmd[5] = 0;
	if ((err=cmd.transport(WRITE,formats+i,12)))
	    sperror ("FORMAT UNIT",err), exit(1);

	if (wait_for_unit (cmd,progress)) exit (1);

	if (!compat)
	{   cmd[0] = 0x5B;	// CLOSE TRACK/SESSION
	    cmd[1] = 1;		// "IMMED" flag on
	    cmd[2] = 0;		// "Stop De-Icing"
	    cmd[9] = 0;
	    if ((err=cmd.transport()))
		sperror ("STOP DE-ICING",err), exit(1);

	    if (wait_for_unit (cmd,progress)) exit (1);
	}

	cmd[0] = 0x5B;		// CLOSE TRACK/SESSION
	cmd[1] = 1;		// "IMMED" flag on
	cmd[2] = 2;		// "Close Session"
	cmd[9] = 0;
	if ((err=cmd.transport()))
	    sperror ("CLOSE SESSION",err), exit(1);

	if (wait_for_unit (cmd,progress)) exit (1);
    }
    else if (mmc_profile==0x12)	// DVD-RAM
    {	cmd[0] = 0x04;		// FORMAT UNIT
	if ((formats[i+4+4]>>2) == 0x01)
	    formats[i+1] = 0,
	    cmd[1] = 0x11;	// "FmtData"|"Format Code"
	else if (full)
	    formats[i+1] = 0x82,// "FOV"|"IMMED"
	    cmd[1] = 0x11;	// "FmtData"|"Format Code"
	else
	    formats[i+1] = 0xA2,// "FOV"|"DCRT"|"IMMED"
	    cmd[1] = 0x19;	// "FmtData"|"CmpLst"|"Format Code"
	cmd[5] = 0;
	if ((err=cmd.transport(WRITE,formats+i,12)))
	    sperror ("FORMAT UNIT",err), exit(1);

	if (wait_for_unit (cmd,progress)) exit(1);
    }
    else			// DVD-RW
    {	page05_setup (cmd,mmc_profile);

	if (do_opc)
	{   cmd[0] = 0x54;	// SEND OPC INFORMATION
	    cmd[1] = 1;		// "Perform OPC"
	    cmd[9] = 0;
	    cmd.timeout(120);	// NEC units can be slooo...w
	    if ((err=cmd.transport())) {
	      if (err==0x52000)	// "INVALID COMMAND"
		{ /* do nothing */ }	// LD-F321 doesn't support SEND OPC?
	      else if (err==0x52700)	// "WRITE PROTECTED"
		{ /* do nothing */ }	// Plextor PX-712A?
	      else if (err==0x17301)	// "POWER CALIBRATION AREA ALMOST FULL"
		{
		  fprintf (stderr,":-! WARNING: Power Calibration Area "
			   "is almost full\n");
		}
	      else if (err==0x37303)   // "POWER CALIBRATION AREA ERROR"
		{
		  fprintf (stderr, ":-! WARNING: Power Calibration Area "
			   "error\n");
		}
	      else {
		sperror ("PERFORM OPC",err);
		exit (1);
	      }
	    }
	}

	if (blank)		// DVD-RW blanking procedure
    	{   cmd[0] = 0xA1;	// BLANK
	    cmd[1] = blank;
	    cmd[11] = 0;
	    if ((err=cmd.transport()))
		sperror ("BLANK",err), exit(1);

	    if (wait_for_unit (cmd,progress)) exit (1);
	}
	else			// DVD-RW format
	{   if ((formats[i+4+4]>>2)==0x15)	// make it really quick
		formats[i+4+0]=formats[i+4+1]=formats[i+4+2]=formats[i+4+3]=0;

	    cmd[0] = 0x04;	// FORMAT UNIT
	    cmd[1] = 0x11;	// "FmtData" and "Format Code"
	    cmd[5] = 0;
	    if ((err=cmd.transport(WRITE,formats+i,12)))
		sperror ("FORMAT UNIT",err), exit(1);

	    if (wait_for_unit (cmd,progress)) exit (1);

	    cmd[0] = 0x35;	// FLUSH CACHE
	    cmd[9] = 0;
	    cmd.transport ();
	}
    }

    while (1)	// Pioneer DVR-x06 needs this...
    {	cmd[0] = 0x1B;		// START/STOP UNIT
	cmd[1] = 0x1;		// "IMMED"
	cmd[4] = 0;		// "Stop"
	cmd[5] = 0;
	if (cmd.transport() == 0x20407)	// "OP IN PROGRESS"
	{   poll (NULL,0,333);
	    continue;
	}
	break;
    }

#if 0
    cmd[0] = 0x1E;	// ALLOW MEDIA REMOVAL
    cmd[5] = 0;
    if (cmd.transport ()) return 1;

    cmd[0] = 0x1B;	// START/STOP UNIT
    cmd[4] = 0x2;	// "Eject"
    cmd[5] = 0;
    if (cmd.transport()) return 1;

    cmd[0] = 0x1B;	// START/STOP UNIT
    cmd[1] = 0x1;	// "IMMED"
    cmd[4] = 0x3;	// "Load"
    cmd[5] = 0;
    cmd.transport ();
#endif

  return 0;
}
