#if 0
#
# Utility for manipulating Book Type Field of Physical Format Descriptor
# located in lead-in of DVD+RW media. This is 9th version. 2nd version
# added initial support for 2nd generation DVD+RW drives. 3rd version
# adds support for DVD+R unit settings. 4th version checks if the unit
# is of RICOH design and reliably recognizes drive generation. 5th
# version adds support for Benq derivatives. 6th version fixes problem
# with USB connected units? 7th version adds support for BTC units.
# 8th version fixes typos in BTC support, adds support for fraudulent
# NEC firmwares and Lite-On based units. 9th version adds support for
# LG, Plextor and dual-format and double-layer Benq units. Keep in mind
# that booktyping support might appear in certain firmware revisions,
# not necessarily all.
#
# The code is in public domain.
#
# See http://www.dvdplusrw.org/resources/bitsettings.html
# for further details.
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

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "transport.hxx"

#define OPT_MEDIA	0x01
#define OPT_UNIT	0x02
#define OPT_UNITRW	0x03
#define OPT_UNITR	0x04
#define OPT_MASK	0x0F
#define OPT_INQ		0x10
#define INTERNAL_RELOAD	0x20

unsigned short profile;
const char    *dev;

const char *bookname (unsigned char book,char *unknown=NULL)
{ const char *ret;

    switch(book&0xF0)
    {	case 0x00:	ret="-ROM";	break;
	case 0x10:	ret="-RAM";	break;
	case 0x20:	ret="-R";	break;
	case 0x30:	ret="-RW";	break;
	case 0x90:	ret="+RW";	break;
	case 0xA0:	ret="+R";	break;
	case 0xE0:	ret="+R DL";	break;
	default:	ret=NULL;
			if (unknown)	sprintf (unknown,"?%02X",book&0xFF);
			break;
    }

  return ret;
}

int mediainfo (Scsi_Command &cmd)
{ unsigned char buf[8],book;
  const char *brand;
  int err;

    cmd[0]=0xAD;	// READ DVD STRUCTURE
    cmd[9]=sizeof(buf);
    cmd[11]=0;
    if ((err=cmd.transport(READ,buf,sizeof(buf))))
    {	sperror ("READ DVD STRUCTURE#0",err);
	return 1;
    }

    book=buf[4];
    brand=bookname (book);

    printf ("Current media Book Type Field is %02xh: ",book);
    if (brand)  printf ("DVD%s specification [revision %d]\n",
			brand,book&0xF);
    else	printf ("unrecognized value\n");

  return 0;
}

int ricoh (Scsi_Command &cmd,int action,int book,int gen=2)
{ int err,cnt=0;
  unsigned char buf[6];
  const char *brand;
  char unknown [16]="?unknown";

    switch (action)
    {	case OPT_INQ|OPT_MEDIA:
		break;
	case OPT_INQ:
	case OPT_INQ|OPT_UNIT:
		if (gen==1)	goto inq_unitrw;
		cmd[0]=0xFA;
		cmd[1]=0x10;
		cmd[8]=sizeof(buf);
		cmd[11]=0;
		if (!cmd.transport(READ,buf,sizeof(buf)))
		{   brand = bookname (buf[4],unknown),
		    printf ("The unit will brand DVD+R media as DVD%s\n",
			    brand?brand:unknown),
		    cnt++;
		}
	case OPT_INQ|OPT_UNITRW:
	inq_unitrw:
		cmd[0]=0xFA;
		cmd[1]=0x00;
		cmd[8]=sizeof(buf);
		cmd[11]=0;
		if (!cmd.transport(READ,buf,sizeof(buf)))
		{   brand = bookname (buf[4],unknown),
		    printf ("The unit will format DVD+RW media as DVD%s\n",
			    brand?brand:unknown),
		    cnt++;
		}
		if (cnt==0)
		    printf ("Unable to determine unit settings. "
			    "Default settings vary from\n"
			    "firmware to firmware. Set "
			    "explicitly to be certain.\n");
		break;
	case OPT_INQ|OPT_UNITR:
		if (gen==1)
		{   fprintf (stderr,":-( not applicable to 1st gen unit\n");
		    return 1;
		}
		cmd[0]=0xFA;
		cmd[1]=0x10;
		cmd[8]=sizeof(buf);
		cmd[11]=0;
		if (!cmd.transport(READ,buf,sizeof(buf)))
		{   brand = bookname (buf[4],unknown),
		    printf ("The unit will brand DVD+R media as DVD%s\n",
			    brand?brand:unknown);
		}
		break;
	case OPT_MEDIA:
		if (profile!=0x1A)
		{   fprintf (stderr,":-( action is applicable to DVD+RW only\n");
		    break;
		}
		if (book != 0x92 && book != 0x01)
		{   fprintf (stderr,":-( BookType#%02x is not applicable\n",book);
		    break;
		}
		cmd[0]=0xF9;
		cmd[1]=0x02;
		cmd[2]=book;
		cmd[3]=(gen==1?0:0xFF);
		cmd[11]=0;
		if ((err=cmd.transport()))
		    return sperror("RICOH_F9h(2)",err),1;
		else
		    return mediainfo(cmd);
		break;
	case OPT_UNIT:
		if (profile==0x1B || profile==0x2B) goto set_unitr;
	case OPT_UNITRW:
		if (book != 0x92 && book != 0x01)
		{   fprintf (stderr,":-( BookType#%02x is not applicable\n",book);
		    break;
		}
		cmd[0]=0xF9;
		cmd[1]=(gen==1?0x01:0x0C);
		cmd[2]=book;
		cmd[3]=(gen==1?0:0xFF);
		cmd[11]=0;
		if ((err=cmd.transport()))
		    sperror (gen==1?"RICOH(1)":"RICOH(0Ch)",err);
		else
		    printf ("Unit was instructed to format DVD+RW as DVD%s\n",
			    book==0x01?"-ROM":"+RW");
		break;
	case OPT_UNITR:
	set_unitr:
		if (gen==1)
		{   fprintf (stderr,":-( not applicable to 1st gen unit\n");
		    return 1;
		}
		if (book != 0xA1 && book != 0x01)
		{   fprintf (stderr,":-( BookType#%02x is not applicable\n",book);
		    break;
		}
		cmd[0]=0xF9;
		cmd[1]=0x14;
		cmd[2]=book;
		cmd[3]=0xFF;
		cmd[11]=0;
		if ((err=cmd.transport()))
		    sperror ("RICOH(14h)",err);
		else
		    printf ("Unit was instructed to brand DVD+R as DVD%s\n",
			    book==0x01?"-ROM":"+R");
		break;
	default:
		break;
    }

  return 0;
}

int benq (Scsi_Command &cmd,int action,int book,int gen=2)
{ int err;
  unsigned char word[2];
  const char *brand;
  char unknown [16]="?unknown";

    switch (action)
    {	case OPT_INQ|OPT_MEDIA:
		break;
	case OPT_INQ:
	case OPT_INQ|OPT_UNIT:
		cmd[0]=0xFF;
		cmd[1]=0x10;
		cmd[11]=0;
		if ((err=cmd.transport(READ,word,sizeof(word))))
		    sperror ("BENQ_FFh(10h)",err);
		else
		{   brand = bookname (word[0],unknown),
		    printf ("The unit will brand DVD+R media as DVD%s\n",
			    brand?brand:unknown);
		}
	case OPT_INQ|OPT_UNITRW:
		cmd[0]=0xFF;
		cmd[1]=0x00;
		cmd[11]=0;
		if ((err=cmd.transport(READ,word,sizeof(word))))
		    sperror ("BENQ_FFh(00h)",err);
		else
		{   brand = bookname (word[0],unknown),
		    printf ("The unit will format DVD+RW media as DVD%s\n",
			    brand?brand:unknown);
		}
		break;
	case OPT_INQ|OPT_UNITR:
		cmd[0]=0xFF;
		cmd[1]=0x10;
		cmd[11]=0;
		if ((err=cmd.transport(READ,word,sizeof(word))))
		    sperror ("BENQ_FFh(10h)",err);
		else
		{   brand = bookname (word[0],unknown),
		    printf ("The unit will brand DVD+R media as DVD%s\n",
			    brand?brand:unknown);
		}

		if (gen<3)	break;

		cmd[0]=0xFF;
		cmd[1]=0x10;
		cmd[2]=0x01;
		cmd[11]=0;
		if ((err=cmd.transport(READ,word,sizeof(word))))
		    sperror ("BENQ_FFh(10h,1)",err);
		else
		{   brand = bookname (word[0],unknown),
		    printf ("The unit will brand DVD+R DL meda as DVD%s\n",
			    brand?brand:unknown);
		}
		break;
	case OPT_MEDIA:
		if (profile!=0x1A)
		{   fprintf (stderr,":-( action is applicable to DVD+RW only\n");
		    break;
		}
		if (book != 0x92 && book != 0x01)
		{   fprintf (stderr,":-( BookType#%02x is not applicable\n",book);
		    break;
		}
#if 0
		cmd[0]=0x1E;	// PREVENT/ALLOW MEDIA REMOVAL
		cmd[4]=1;	// "Prevent"
		cmd[5]=0;
		if ((err=cmd.transport()))
		{   sperror ("PREVENT MEDIA REMOVAL",err);
		    break;
		}
#endif
		cmd[0]=0xFE;
		cmd[1]=2;
		cmd[2]=book;
		cmd[11]=0;
		if ((err=cmd.transport()))
		    sperror ("BENQ_FEh(2)",err);
		else
		    action = INTERNAL_RELOAD;
		break;
	case OPT_UNIT:
		if (profile==0x1B || profile==0x2B) goto set_unitr;
	case OPT_UNITRW:
		if (book != 0x92 && book != 0x01)
		{   fprintf (stderr,":-( BookType#%02x is not applicable\n",book);
		    break;
		}
		cmd[0]=0xFE;
		cmd[1]=4;
		cmd[2]=book;
		cmd[11]=0;
		if ((err=cmd.transport()))
		    sperror ("BENQ_FEh(4)",err);
		else
		    printf ("Unit was instructed to format DVD+RW as DVD%s\n",
			    book==0x01?"-ROM":"+RW");
		break;
	case OPT_UNITR:
	set_unitr:
		if (book != 0xA1 && book != 0x01)
		{   fprintf (stderr,":-( BookType#%02x is not applicable\n",book);
		    break;
		}
		cmd[0]=0xFE;
		cmd[1]=5;
		cmd[2]=book;
		cmd[11]=0;
		if ((err=cmd.transport()))
		    sperror ("BENQ_FEh(5)",err);
		else
		    printf ("Unit was instructed to brand DVD+R as DVD%s\n",
			    book==0x01?"-ROM":"+R");

		if (gen<3)	break;

		cmd[0]=0xFE;
		cmd[0]=5;
		cmd[2]=book==0x01?0x01:0xE1;
		cmd[3]=1;
		cmd[11]=0;
		if ((err=cmd.transport()))
		    sperror ("BENQ_FEh(5,1)",err);
		else
		    printf ("Unit was instructed to brand DVD+R DL as DVD%s\n",
			    book==0x01?"-ROM":"+R DL");
		break;
	default:
		break;
    }

    cmd[0]=0xFD;
    cmd[1]=0xF2;
    cmd[2]='B';
    cmd[3]='E';
    cmd[4]='N';
    cmd[5]='Q';
    cmd[11]=0;
    if ((err=cmd.transport()))
	sperror ("BENQ_FDh(F2h)",err);

    if (action!=INTERNAL_RELOAD)
	return 0;

    cmd[0]=0x1E;	// PREVENT/ALLOW MEDIA REMOVAL
    cmd[5]=0;
    if ((err=cmd.transport()))
	return sperror ("ALLOW MEDIA REMOVAL",err),1;

    cmd[0]=0x1B;	// START/STOP UNIT
    cmd[4]=0x2;		// "Eject"
    cmd[5]=0;
    if ((err=cmd.transport()))
	return sperror ("EJECT",err),1;

    cmd[0]=0x1B;	// START/STOP UNIT
    cmd[4]=0x3;		// "Load"
    cmd[5]=0;
    if ((err=cmd.transport()))
	return sperror ("LOAD TRAY",err),1;

    if (wait_for_unit (cmd))	return 1;

  return mediainfo (cmd);
}

int btc (Scsi_Command &cmd,int action,int book,int gen=0)
{ int err,obligatory=0;
  const char *brand="";

    switch (action)
    {	case OPT_INQ|OPT_MEDIA:
		break;
	case OPT_INQ:
	case OPT_INQ|OPT_UNIT:
	case OPT_INQ|OPT_UNITRW:
	case OPT_INQ|OPT_UNITR:
		fprintf (stderr,":-( Can't inquiry unit settings, "
				"you have to set booktype prior every "
				"recording to be sure.\n");
		return 1;
		break;
	case OPT_MEDIA:
		if (profile!=0x1A && profile!=0x14 && profile!=13)
		{   fprintf (stderr,":-( action is applicable to DVD±RW only\n");
		    break;
		}

		obligatory = (profile==0x1A)?0x92:0x32;
		if (book!=obligatory && book!=0x01)
		{   fprintf (stderr,":-( BookType#%02x is not applicable\n",book);
		    break;
		}

		cmd[0]=0x1E;	// PREVENT/ALLOW MEDIA REMOVAL
		cmd[4]=1;	// "Prevent"
		cmd[5]=0;
		if ((err=cmd.transport()))
		{   sperror ("PREVENT MEDIA REMOVAL",err);
		    break;
		}

		cmd[0]=0xFA;
		cmd[2]=book;
		cmd[10]=0xAA;
		cmd[11]=0xFF;
		if ((err=cmd.transport()))
		    sperror ("BTC_FAh[2]",err);
		else
		    wait_for_unit (cmd);

		cmd[0]=0x1E;	// PREVENT/ALLOW MEDIA REMOVAL
		cmd[5]=0;
		if ((err=cmd.transport()))
		    return sperror ("ALLOW MEDIA REMOVAL",err),1;

		break;
	case OPT_UNIT:
		if (profile==0x1B || profile==0x2B || profile==0x11)
		    goto set_unitr;
	case OPT_UNITRW:
		fprintf (stderr,":-( Can't set RW unit settings, "
				"manipulate media.\n");
		return 1;
		break;
	case OPT_UNITR:
	set_unitr:
		// unit requires media in, so we can rely on profile...
		if (profile==0x1B)	obligatory=0xA1, brand="+R";
		else if (profile==0x2B)	obligatory=0xE1, brand="+R DL";
		else			obligatory=0x20, brand="-R";

		if (book!=obligatory && book!=0x01)
		{   fprintf (stderr,":-( BookType#%02x is not applicable\n",book);
		    return 1;
		}

		cmd[0]=0xFA;
		cmd[3]=book;
		cmd[4]=book==0x01?0:1;
		cmd[10]=0xAA;
		cmd[11]=0xFF;
		if ((err=cmd.transport()))
		    return sperror ("BTC_FAh[3]",err),1;
		else
		    printf ("Unit was instructed to brand DVD%s as DVD%s\n",
			    brand,book==0x01?"-ROM":brand);
		return 0;
		break;
	default:
		break;
    }

  return mediainfo (cmd);
}

int liteon (Scsi_Command &cmd,int action,int book,unsigned char *buf=NULL)
{ int err;

    switch (action)
    {	case OPT_INQ|OPT_MEDIA:
		break;
	case OPT_INQ:
	case OPT_INQ|OPT_UNIT:
	case OPT_INQ|OPT_UNITRW:
	case OPT_INQ|OPT_UNITR:
		if (buf==NULL || buf[0]==0)
		{   printf ("Unit will brand DVD+plus media with "
			    "corresponding booktype, e.g. DVD+R as DVD+R...\n");
		    return buf==NULL?1:0;
		}
		if (buf[0]==1)
		    printf ("Unit will brand DVD+plus media as DVD-ROM\n");
		else if (buf[0]==2)
		    printf ("Unit will format DVD+RW media as DVD+RW\n");
		else
		    fprintf (stderr,":-? Insane unit setting %02x\n",buf[0]);
		return buf[0]==1?0:1;
		break;
	case OPT_MEDIA:
		fprintf (stderr,":-( Direct DVD+RW media manipulations "
				"are not supported.\n"
				"    Use -unit flag instead and apply "
				"-dvd-compat recordinging procedure.\n");
		return 1;
		break;
	case OPT_UNIT:
	case OPT_UNITRW:
	case OPT_UNITR:
		cmd[0]=0xDF;
		cmd[2]=0x0F;
		cmd[3]=1;
		cmd[4]=book==0x01?1:0;
		cmd[11]=0;
		if ((err=cmd.transport()))
		    return sperror ("LITEON_DFh[1]",err),1;

		if (book==0x01)
		    printf ("Unit was instructed to brand DVD+plus media "
			    "as DVD-ROM\n");
		else
		    printf ("Unit was instructed to brand DVD+plus media with "
			    "corresponding booktype, e.g. DVD+R as DVD+R...\n");
		return 0;
		break;
	default:
		break;
    }

  return mediainfo (cmd);
}

// As per http://www-user.tu-chemnitz.de/~noe/Bitsetting/
int plextor (Scsi_Command &cmd,int action,int book,int gen=2)
{ int err;
  unsigned char vector[8];

    switch (action)
    {	case OPT_INQ|OPT_MEDIA:
		break;
	case OPT_INQ|OPT_UNITRW:
	case OPT_UNITRW:
	case OPT_MEDIA:
		fprintf (stderr,":-( DVD+RW booktyping is not supported\n");
		return 1;
		break;
	case OPT_INQ:
	case OPT_INQ|OPT_UNIT:
	case OPT_INQ|OPT_UNITR:
		cmd[0] = 0xE9;
		cmd[2] = 0x22;
		cmd[3] = 0x0A;
		cmd[9] = sizeof(vector);
		cmd[11] = 0;
		if ((err=cmd.transport(READ,vector,sizeof(vector))))
		    return sperror ("PLEXTOR_E9h(A)",err),1;

		printf ("Unit will brand DVD+R media as DVD%s\n",
			vector[2]?"-ROM":"+R");

		if (gen<3)	return 0;

		cmd[0] = 0xE9;
		cmd[2] = 0x22;
		cmd[3] = 0x0E;
		cmd[9] = sizeof(vector);
		cmd[11] = 0;
		if ((err=cmd.transport(READ,vector,sizeof(vector))))
		    return sperror ("PLEXTOR_E9h(E)",err),1;

		printf ("Unit will brand DVD+R DL media as DVD%s\n",
			vector[2]?"-ROM":"+R DL");

		return 0;
		break;
	case OPT_UNIT:
	case OPT_UNITR:
		cmd[0] = 0xE9;
		cmd[1] = 0x10;
		cmd[2] = 0x22;
		cmd[3] = 0x0A;
		cmd[5] = book&0xF0?0:1;
		cmd[9] = sizeof(vector);
		cmd[11] = 0;
		if ((err=cmd.transport(READ,vector,sizeof(vector))))
		    return sperror ("PLEXTOR_E9h(A)",err),1;

		printf ("Unit was instruction to brand DVD+R media as DVD%s\n",
			book&0xF0?"+R":"-ROM");

		if (gen<3)	return 0;

		cmd[0] = 0xE9;
		cmd[1] = 0x10;
		cmd[2] = 0x22;
		cmd[3] = 0x0E;
		cmd[5] = book&0xF0?0:1;
		cmd[9] = sizeof(vector);
		cmd[11] = 0;
		if ((err=cmd.transport(READ,vector,sizeof(vector))))
		    return sperror ("PLEXTOR_E9h(E)",err),1;

		printf ("Unit was instruction to brand DVD+R DL media as DVD%s\n",
			book&0xF0?"+R DL":"-ROM");

		return 0;
		break;
	default:
		break;
    }

  return mediainfo (cmd);
}

int lg (Scsi_Command &cmd,int action,int book)
{ int err;
  unsigned char vector[4];

    switch (action)
    {	case OPT_INQ|OPT_MEDIA:
		break;
	case OPT_INQ|OPT_UNITRW:
	case OPT_UNITRW:
	case OPT_MEDIA:
		fprintf (stderr,":-( DVD+RW booktyping is not supported\n");
		return 1;
		break;
	case OPT_INQ:
	case OPT_INQ|OPT_UNIT:
	case OPT_INQ|OPT_UNITR:
		cmd[0] = 0xFA;
		cmd[8] = sizeof(vector);
		cmd[9] = 0;
		if ((err=cmd.transport(READ,vector,sizeof(vector))))
		    return sperror ("LG_FAh",err),1;

		printf ("Unit will brand DVD+R media as DVD%s\n",
			vector[0]&0xF0?"+R":"-ROM");
		printf ("Unit will brand DVD+R DL media as DVD%s\n",
			vector[1]&0xF0?"+R DL":"-ROM");
		return 0;
		break;
	case OPT_UNIT:
	case OPT_UNITR:
		memset (vector,0,sizeof(vector));
		if (book&0xF0)
		    vector[0] = 0xA0,	// DVD+R
		    vector[1] = 0xE0;	// DVD+R DL

		cmd[0] = 0xFC;
		cmd[2] = '+';
		cmd[3] = 'R';
		cmd[4] = 'T';
		cmd[5] = 'B';
		cmd[8] = sizeof(vector);
		cmd[9] = 0;
		if ((err=cmd.transport(WRITE,vector,sizeof(vector))))
		    return sperror ("LG_FCh",err),1;

		printf ("Unit was instruction to brand DVD+R media as DVD%s\n",
			book&0xF0?"+R":"-ROM");
		return 0;
		break;
	default:
		break;
    }

  return mediainfo (cmd);
}

int main(int argc,char *argv[])
{ Scsi_Command  cmd;
  unsigned char buf[128];
  int book=0,action=0;
  int plusgeneration=0,dashcapable=0,ramcapable=0;
  int i,err,hp=0,plx=0;

    for(dev=NULL,i=1;i<argc;i++)
    {	if	(!strncmp (argv[i],"-dvd-rom",7))   book=0x01;
	else if (!strncmp (argv[i],"-dvd+rw",7))    book=0x92;
	else if (!strncmp (argv[i],"-dvd+r",6))     book=0xA1;
	else if (!strncmp (argv[i],"-dvd-rw",7))    book=0x32;
	else if (!strncmp (argv[i],"-dvd-r",6))     book=0x20;
	else if (!strncmp (argv[i],"-inq",4))       book=0xFF, action|=OPT_INQ;
	else if (!strncmp (argv[i],"-unit+rw",8))   action&=~OPT_MASK, action|=OPT_UNITRW;
	else if (!strncmp (argv[i],"-unit+r",7))    action&=~OPT_MASK, action|=OPT_UNITR;
	else if (!strncmp (argv[i],"-unit",3))      action&=~OPT_MASK, action|=OPT_UNIT;
	else if (!strncmp (argv[i],"-media",3))     action&=~OPT_MASK, action|=OPT_MEDIA;
	else					    dev=argv[i];
    }

    if (!(dev && book && action))
    {	fprintf (stderr,"Usage: %s "
			"[-dvd-rom-spec|-dvd+rw-spec|-dvd+r-spec|-inq] \\\n"
			"       [-media|-unit|-unit+rw|-unit+r] "
#if defined(_WIN32)
			"d:\n",argv[0]),
	fprintf (stderr,"For further information see http://"
			"www.dvdplusrw.org/resources/bitsettings.html\n");
#else
			"/dev/dvd\n",argv[0]);
#endif
	return 1;
    }

    if (!cmd.associate(dev))
    {	fprintf (stderr,"%s: unable to open: ",dev), perror (NULL);
	return 1;
    }

    cmd[0] = 0x12;	// INQUIRY
    cmd[4] = 36;
    cmd[5] = 0;
    if ((err=cmd.transport(READ,buf,36)))
    {	sperror ("INQUIRY",err);
	return 1;
    }

    if ((buf[0]&0x1F) != 5)
    {	fprintf (stderr,":-( not an MMC unit!\n");
	return 1;
    }
    hp =       !memcmp (buf+8,"HP      ",8);
    hp = hp || !memcmp (buf+8,"_NEC    ",8);
    plx =      !memcmp (buf+8,"PLEXTOR ",8);

    cmd[0]=0x46;	// GET CONFIGURATION
    cmd[1]=2;
    cmd[8]=8;
    cmd[9]=0;
    if ((err=cmd.transport(READ,buf,8)))
    {	sperror ("GET CONFIGURATION",err);
	return 1;
    }

    // See if it's 2 gen drive by checking if DVD+R profile is an option.
    // Catch dual- and triple-format units as well...
    {	int len=4+(buf[0]<<24|buf[1]<<16|buf[2]<<8|buf[3]);
	if (len>264)
	{   fprintf (stderr,":-( insane profile list length [%d]\n",len);
	    return 1;
	}
	unsigned char *list=new unsigned char[len];

	cmd[0]=0x46;	// GET CONFIGURATION
	cmd[1]=2;
	cmd[7]=len>>8;
	cmd[8]=len;
	cmd[9]=0;
	if ((err=cmd.transport(READ,list,len)))
	    return sperror("GET CONFIGURATION",err),1;

	for (plusgeneration=1,i=12;i<list[11];i+=4)
	{   switch (list[i]<<8|list[i+1])
	    {	case 0x1B:	// DVD+R supported
			plusgeneration=2;	break;
		case 0x2B:	// DVD+R DL supported
			plusgeneration=3;	break;
		case 0x11:	// DVD-R supported
			dashcapable=1;		break;
		case 0x12:	// DVD-RAM supported
			ramcapable=1;		break;
	    }
	}

	delete list;
    }

    profile=buf[6]<<8|buf[7];

    if (action==(OPT_INQ|OPT_MEDIA))
    { int ret=1;
	if (profile&0x10 || profile==0x2B)	ret=mediainfo (cmd);
	else if (profile==0)	fprintf (stderr,":-( no media mounted\n");
	else			fprintf (stderr,":-( non-DVD media\n");
	return ret;
    }

    if (!dashcapable || hp)
    {	// See if it's a RICOH design...
	cmd[0]=0x5A;	// MODE SENSE
	cmd[1]=0x08;	// "Disable Block Descriptors"
	cmd[2]=0x30;	// RICOH specific page
	cmd[8]=12;
	cmd[9]=0;
	if (!(err=cmd.transport(READ,buf,12)) && buf[8] == 0x30)
	    return ricoh (cmd,action,book,plusgeneration);

	goto benq;
    }
    else if (plx)	// Plextor doesn't seem to re-badge their units...
	return plextor (cmd,action,book,plusgeneration);
    else if (ramcapable) // LG is the only one we know...
	return lg (cmd,action,book);
    else
    {	// See if it's BTC design...
	cmd[0]=0xFA;
	cmd[4]=1;
	cmd[10]=0xAA;
	cmd[11]=0xFF;
	if (!(err=cmd.transport()) || ASC(err)==0x24)
	    return btc (cmd,action,book);
	else if (ASC(err)==0x3A)
	    return fprintf (stderr,":-( BTC_FAh: no media mounted\n"),1;

	// See if it's LITE-ON design...
	unsigned char buf[0x80];

	cmd[0]=0xDF;
	cmd[2]=0x0F;
	cmd[7]=sizeof(buf);
	cmd[11]=0;
	if (!(err=cmd.transport(READ,buf,sizeof(buf))))
	    return liteon (cmd,action,book,buf);

    benq:
	// See if it's BENQ design...
	cmd[0]=0xFD;
	cmd[1]=0xF1;
	cmd[2]='B';
	cmd[3]='E';
	cmd[4]='N';
	cmd[5]='Q';
	cmd[11]=0;
	if (!(err=cmd.transport()))
	    return benq (cmd,action,book,plusgeneration);
    }

    fprintf (stderr,"This program targets units of "
		    "RICOH, BENQ, BTC, LITE-ON, LG and PLEXTOR designs.\n"
		    "%s doesn't appear to be one. Exiting.\n",
		    dev);

  return 1;
}
