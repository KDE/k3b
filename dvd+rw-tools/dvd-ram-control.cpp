#include "transport.hxx"

int main (int argc,char *argv[])
{ Scsi_Command cmd;
  unsigned char buf[128],erb=0;
  int err,i,rdonly=0;
  const char *dev=NULL;

    for (i=1;i<argc;i++)
    {	if	(!strncmp (argv[i],"-arre",4))		erb|=0x40;
	else if	(!strncmp (argv[i],"-awre",4))		erb|=0x80;
	else if	(!strncmp (argv[i],"-rdonly",4))	rdonly=1;
	else if (!strncmp (argv[i],"-rdwr",4))		rdonly=-1;
	else if	(!strncmp (argv[i],"-rw",3))		rdonly=-1;
	else if (!strncmp (argv[i],"-wronly",3))	rdonly=-1;
	else						dev=argv[i];
    }

    if (!(dev && (erb || rdonly)))
    {	fprintf (stderr,"usage: %s [-arre] [-awre] [-rdonly|-rdwr] /dev/dvd\n",
			argv[0]);
	return 1;
    }

    if (!cmd.associate (dev))
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

#if 0
    cmd[0]=0x46;	// GET CONFIGURATION
    cmd[1]=1;
    cmd[8]=8;
    cmd[9]=0;
    if ((err=cmd.transport(READ,buf,8)))
    {	sperror ("GET CONFIGURATION",err);
	return 1;
    }

    if ((buf[6]<<8|buf[7]) != 0x12)
    {   fprintf (stderr,":-( not DVD-RAM!\n");
	return 1;
    }
#endif

    if (erb) do
    { unsigned char page01[8+12];

	cmd[0]=0x5A;		// MODE SENSE
	cmd[1]=0x08;		// "Disable Block Descriptors"
	cmd[2]=1;		// Page 01
	cmd[8]=sizeof(page01);
	if ((err=cmd.transport(READ,page01,sizeof(page01))))
	{   sperror("MODE SENSE",err); return 1;   }

	if ((page01[8+2]&erb) == erb)
	{   printf ("A[RW]RE bit is set already.\n");
	    break;
	}

	memset (page01,0,8);
	page01[8+2]|=erb;	// A[WR]RE on

	cmd[0]=0x55;	// MODE SELECT
	cmd[1]=0x10;	// conformant
	cmd[8]=sizeof(page01);
 	cmd[9]=0;
	if ((err=cmd.transport(WRITE,page01,sizeof(page01))))
	{   sperror("MODE SELECT",err); return 1;   }

	// Verify settings...
	cmd[0]=0x5A;		// MODE SENSE
	cmd[1]=0x08;		// "Disable Block Descriptors"
	cmd[2]=1;		// Page 01
	cmd[8]=sizeof(page01);
	if ((err=cmd.transport(READ,page01,sizeof(page01))))
	{   sperror("MODE SENSE",err); return 1;   }

	if ((page01[8+2]&erb) != erb)
	{   fprintf (stderr,":-( failed to set A[RW]RE bit.\n");
	    return 1;
	}
    } while (0);

    if (rdonly) do
    { unsigned char dvd_C0[8];

	cmd[0]=0xAD;	// READ DVD STRUCTURE
	cmd[7]=0xC0;
	cmd[9]=sizeof(dvd_C0);
	cmd[11]=0;
	if ((err=cmd.transport(READ,dvd_C0,sizeof(dvd_C0))))
	{   sperror("READ DVD STRUCTURE#C0",err); return 1;   }

	if (rdonly>0 && (dvd_C0[4]&0x02))
	{   printf ("The disc is write protected already.\n");
	    break;
	}
	else if (rdonly<0 && !(dvd_C0[4]&0x02))
	{   printf ("The disc is unprotected already.\n");
	    break;
	}

	memset (dvd_C0,0,sizeof(dvd_C0));
	dvd_C0[1]=6;
	if (rdonly>0)	dvd_C0[4]|=2;	// "PWP" on
	else		dvd_C0[4]&=~2;	// "PWP" off

	cmd[0]=0xBF;	// SEND DVD STRUCTURE
	cmd[7]=0xC0;
	cmd[9]=sizeof(dvd_C0);
	cmd[11]=0;
	if ((err=cmd.transport(WRITE,dvd_C0,sizeof(dvd_C0))))
	{   sperror("SEND DVD STRUCTURE#C0",err); return 1;   }

	// Verify...
	cmd[0]=0xAD;	// READ DVD STRUCTURE
	cmd[7]=0xC0;
	cmd[9]=sizeof(dvd_C0);
	cmd[11]=0;
	if ((err=cmd.transport(READ,dvd_C0,sizeof(dvd_C0))))
	{   sperror("READ DVD STRUCTURE#C0",err); return 1;   }

	printf ("Persistent Write Protection is %s\n",
		dvd_C0[4]&0x02 ? "on" : "off");

    } while (0);

  return 0;
}
