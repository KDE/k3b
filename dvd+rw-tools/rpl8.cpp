#if 0
#
# The code is in public domain, May-September 2002.
#
# RICOH Program Loader 8 (primarily DVD+RW units) for Linux 2.4 and
# upwards. So far has been tested with:
#	- HP dvd100i
#	- Ricoh MP5120A
#	- Philips DVDRW208
#	- HP dvd200i
#
# This is second version adding support for Program Loader 9 and
# firmware consistency check.
#
# The program never updates Program Loader code so that you should
# always be able to downgrade your firmware in case something goes
# wrong.
#
# - make sure you have no media in drive, optionally go to single user
#   mode;
# - run 'rpl8 /dev/cdrom firmware.image.bin';
# - observe the program backing up your current firmware and uploading
#   new one;
# - take system down and cycle the power;
#
# There is -dump-only option which only backs up firmware. Note that
# you might have to cycle the power even after -dump-only.
#
# "ide-scsi: Strange, packet command initiated yet DRQ isn't asserted"
# surrounded by other nasty messages logged on console is "normal."
#
/bin/sh << EOS
MODNAME=\`expr "/$0" : '\(.*[^/]\)/*$' : '.*/\(..*\)' : '\(.*\)\..*$'\`
case "`uname -s`" in
SunOS)	(set -x; g++ -fno-exceptions -O -o \$MODNAME "$0" -lvolmgt) ;;
*)	(set -x; g++ -fno-exceptions -O -o \$MODNAME "$0") ;;
esac
EOS
exit
#endif

#if defined(__linux)
#define _XOPEN_SOURCE 500
#endif

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <poll.h>
#include <signal.h>

#include "transport.hxx"

#define BUFFER_BLOCK	(16*1024)
#define FIRMWARE_SIZE	(1024*1024)
#define MAGIC_OFFSET	(32*1024)
#if FIRMWARE_SIZE%BUFFER_BLOCK!=0
#error "Invalid BUFFER_BLOCK"
#endif
#if MAGIC_OFFSET%BUFFER_BLOCK!=0
#error "Invalid BUFFER_BLOCK"
#endif

int main (int argc,char *argv[])
{ int			fd,in,out,off,j;
  struct stat		sb;
  char			dump[64],*meaning;
  unsigned char		*buffer,*firmware,sigfile[6],sigunit[6],ascq='\xFF';
  struct	{
	unsigned char	type,skip[7];
	char		vendor[8],ident[16],revision[4];
		}	inq;
  Scsi_Command		cmd;
  unsigned char		*sense=cmd.sense();

    if (argc<3)
    {	fprintf (stderr,"usage: %s /dev/cdrom "
			"[-[dump-only]|firmware.bin]\n",argv[0]);
	return 1;
    }

    if ((fd=open ("/dev/zero",O_RDWR)) < 0)
    {	fprintf (stderr,"unable to open(\"/dev/zero\"): "), perror(NULL);
	return 1;
    }

    buffer = (unsigned char *)mmap(NULL,BUFFER_BLOCK,PROT_READ|PROT_WRITE,
					MAP_PRIVATE,fd,0);
    if (buffer==MAP_FAILED)
    {	fprintf (stderr,"unable to anonymously mmap %d bytes: ",BUFFER_BLOCK),
	perror (NULL);
	return 1;
    }
    close (fd);

    if (!cmd.associate(argv[1]))
    {	fprintf (stderr,"%s: unable to open: ",argv[1]), perror (NULL);
	return 1;
    }

    cmd[0]=0x12;		// INQUIRY
    cmd[4]=sizeof(inq);
    cmd[5]=0;
    if (cmd.transport(READ,(unsigned char *)&inq,sizeof(inq)))
    {	perror("unable to INQUIRY, examine /var/log/message");
	return 1;
    }

    if ((inq.type&0x1F) != 5)
    {	fprintf (stderr,"\"%s\" is not a CD-ROM device\n",argv[1]);
	return 1;
    }

    cmd[0]=0x00;		// TEST UNIT READY
    cmd[5]=0x80;
    if (!cmd.transport())
    {	fprintf (stderr,"\"%s\" is ready, is media present?\n",argv[1]);
	return 1;
    }

    if (sense[12] != 0x3A)	// should reply with "NO MEDIA"
    {	fprintf (stderr,"\"%s\": unexpected ASC=%02xh/ASCQ=%02xh\n",
			argv[1],sense[12],sense[13]);
	return 1;
    }

    sprintf (dump,"%.8s%.16s%.4s.bin",inq.vendor,inq.ident,inq.revision);
    for(j=0;dump[j];j++) if (dump[j]==' ') dump[j]='_';

    in=-1, firmware=NULL;
    if (argv[2][0] != '-')
    { unsigned short cksum;
      int i;

	if ((in=open(argv[2],O_RDONLY)) < 0)
        {   fprintf (stderr,"unable to open(\"%s\"): ",argv[2]), perror (NULL);
	    return 1;
	}
	if (fstat(in,&sb) < 0)
        {   fprintf (stderr,"unable to stat(\"%s\"): ",argv[2]), perror (NULL);
	    return 1;
	}

	if (sb.st_size != FIRMWARE_SIZE)
        {   fprintf (stderr,"\"%s\": size is insane\n",argv[2]);
	    return 1;
	}

	firmware=(unsigned char *)mmap(NULL,FIRMWARE_SIZE,PROT_READ,
					    MAP_PRIVATE,in,0);
	if (firmware==MAP_FAILED)
	{   fprintf (stderr,"unable to mmap(\"%s\"): ",argv[2]), perror (NULL);
	    return 1;
	}

	close (in);

	memcpy(sigfile,firmware+MAGIC_OFFSET+4,6);
	if (memcmp(sigfile,"MMP5",4))
	{   fprintf (stderr,"\"%s\": signature is insane\n",argv[2]);
	    return 1;
	}

	for (cksum=0,i=MAGIC_OFFSET;i<FIRMWARE_SIZE-2;i++)
	    cksum += firmware[i];
	if ((cksum&0xFF) != firmware[FIRMWARE_SIZE-1] ||
	    (cksum>>8)   != firmware[FIRMWARE_SIZE-2]
	   )
	{   fprintf (stderr,"\"%s\" is corrupted or encrypted\n",argv[2]);
	    return 1;
	}
    }

    if ((out=open(dump,O_WRONLY|O_CREAT|O_EXCL,0666)) < 0)
    {   fprintf (stderr,"unable to creat(\"%s\"): ",dump), perror (NULL);
	return 1;
    }

    cmd[0]=0xE4;		// INVOKE PROGRAM LOADER
    cmd[9]=(1<<6)&0x40;
    if (cmd.transport())
    {	fprintf (stderr,"\"%s\": unable to invoke RICOH Program Loader "
			"[ASC=%02xh/ASCQ=%02xh]\n",
			argv[1],sense[12],sense[13]);
	return 1;
    }

    do {	// not really a loop, just a way to get rid of goto's...
	poll(NULL,0,250);

	cmd[0]=0xE4;		// INVOKE PROGRAM LOADER
	cmd[9]=(1<<6)&0x40;
	if (cmd.transport())
	{   fprintf (stderr,"\"%s\"[E4]: unexpected ASC=%02xh/ASCQ=%02xh\n",
			    argv[1],sense[12],sense[13]);
	    break;
	}

	poll(NULL,0,250);

	cmd[0]=0x12;		// INQUIRY
	cmd[4]=sizeof(inq);
	cmd[5]=0;
	if (cmd.transport(READ,(unsigned char *)&inq,sizeof(inq)))
	{   perror("unable to INQUIRY, examine /var/log/message");
	    break;
	}

	if (memcmp(&inq.vendor,"RICOH   Program Loader 8",24) &&
	    memcmp(&inq.vendor,"RICOH   Program Loader 9",24) )
	{   fprintf (stderr,"\"%s\": unknown Program Loader "
			    "%.8s|%.16s|%.4s\n",
			    argv[1],inq.vendor,inq.ident,inq.revision);
	    break;
	}

	fprintf (stderr,"\"%s\": dumping firmware image to \"%s\" ",
			argv[1],dump);
	for (off=0;off<FIRMWARE_SIZE;off+=BUFFER_BLOCK)
	{   cmd[0]=0xE3;	// DOWNLOAD FIRMWARE PAGE
	    cmd[2]=(off>>24)&0xFF;
	    cmd[3]=(off>>16)&0xFF;
	    cmd[4]=(off>>8)&0xFF;
	    cmd[5]=(off)&0xFF;
	    cmd[7]=(BUFFER_BLOCK>>8)&0xFF;
	    cmd[8]=(BUFFER_BLOCK)&0xFF;
	    cmd[9]=0;
	    if (cmd.transport(READ,buffer,BUFFER_BLOCK))
            {	fprintf (stderr,"\"%s\"[E3]: unexpected "
				"ASC=%02xh/ASCQ=%02xh\n",
				argv[1],sense[12],sense[13]);
		in=-1;
		break;
	    }
	    if (write(out,buffer,BUFFER_BLOCK) != BUFFER_BLOCK)
	    {	fprintf (stderr,"unable to write(\"%s\"): ",dump),
		perror(NULL);
		in=-1;
		break;
	    }
	    fprintf (stderr,".");
	    if (off==MAGIC_OFFSET) { memcpy(sigunit,buffer+4,6); }
	}
	fprintf (stderr,"\n");

	if (in==-1) break; /* dump only or we failed to backup */

#if 1
	if (memcmp(sigunit,sigfile,6))
	{   fprintf (stderr,"\"%s\": firmware signature mismatch "
			    "[%.6s|%.6s]\n",
			    argv[1],sigunit,sigfile);
	    break;
	}
#endif

	fprintf (stderr,"\"%s\": uploading firmware image",argv[1]);
	for (off=0;off<FIRMWARE_SIZE;off+=BUFFER_BLOCK)
	{   cmd[0]=0xE2;	// UPLOAD FIRMWARE PAGE
	    cmd[5]=(BUFFER_BLOCK>>16)&0xFF;
	    cmd[6]=(BUFFER_BLOCK>>8)&0xFF;
	    cmd[7]=(BUFFER_BLOCK)&0xFF;
	    cmd[11]=0;
	    if (cmd.transport(WRITE,firmware+off,BUFFER_BLOCK))
            {	fprintf (stderr,"\"%s\"[E2]: unexpected "
				"ASC=%02xh/ASCQ=%02xh\n",
				argv[1],sense[12],sense[13]);
		in=-1;
		break;
            }
	    fprintf (stderr,".");
	}
	fprintf (stderr,"\n");

	if (in==-1) break; /* failed to upload */

	fprintf (stderr,"\"%s\": unlocking firmware\n",argv[1]);
	cmd[0]=0xE0;		// UNLOCK FIRMWARE
	cmd[3]=0;
	cmd[9]=0;
	if (cmd.transport())
	{   fprintf (stderr,"\"%s\"[E0]: unexpected "
			    "ASC=%02xh/ASCQ=%02xh\n",
			    argv[1],sense[12],sense[13]);
	    break;
	}

	fprintf (stderr,"\"%s\": ¡POINT OF NO RETURN!\n",argv[1]);
	poll(NULL,0,3000);
	signal (SIGINT,SIG_IGN);
	signal (SIGTERM,SIG_IGN);

	cmd[0]=0xE1;		// FIRE IT UP
	// -----v----- don't update the Program Loader code
	cmd[9]=(0<<6)&0x40;
	if (cmd.transport())
	{   fprintf (stderr,"\"%s\"[E1]: unexpected "
			    "ASC=%02xh/ASCQ=%02xh\n",
			    argv[1],sense[12],sense[13]);
	    break;
	}

	while(1)
	{   poll(NULL,0,250);
	    cmd[0]=0x00;		// TEST UNIT READY
	    cmd[5]=0x80;
	    if (!cmd.transport()) continue; // never happens?
	    if (sense[12] == 0xF7)
	    {	if (ascq != sense[13])
		{   switch (ascq=sense[13])
		    {	case 0: meaning="preparing";		break;
			case 1: meaning="erasing contents";	break;
			case 2: meaning="writing contents";	break;
			case 3: meaning="verifying contents";	break;
			default:meaning="unknown meaning";	break;
		    }
		    fprintf (stderr,"\"%s\": %s\n",argv[1],meaning);
		}
	    }
	    else if (sense[12] == 0x3A) /* no media */
	    {	fprintf (stderr,"\"%s\": firmware upgrade succeeded:-)\n",
				argv[1]),
		fprintf (stderr,"\"%s\": "
				"¡CYCLE THE POWER BEFORE PROCEEDING!\n",
				argv[1]);
		break;
	    }
	    else
            {	fprintf (stderr,"\"%s\"[0]: unexpected "
				"ASC=%02xh/ASCQ=%02xh\n",
				argv[1],sense[12],sense[13]);
		in=-1;
		break;
	    }
	}
    } while (0);

    cmd[0]=0xE4;	// TERMINATE PROGRAM LOADER
    cmd[9]=(0<<6)&0x40;
    if (cmd.transport())
    {	fprintf (stderr,"\"%s\": unexpected ASC=%02xh/ASCQ=%02xh\n",
			argv[1],sense[12],sense[13]);
	return 1;
    }

  return in<0;
}
