/*
 * $Id$
 *
 * This file is part of WorkMan, the civilized CD player library
 * (c) 1991-1997 by Steven Grimm (original author)
 * (c) by Dirk Försterling (current 'author' = maintainer)
 * The maintainer can be contacted by his e-mail address:
 * milliByte@DeathsDoor.com 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * establish connection to cddb server and get data
 * socket stuff gotten from gnu port of the finger command
 *
 * provided by Sven Oliver Moll
 *
 */

static char cddb_id[] = "$Id$";

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "include/wm_config.h"
#include "include/wm_struct.h"
#include "include/wm_cdinfo.h"
#include "include/wm_helpers.h"

/*
 * This is for identifying WorkMan at CDDB servers
 */
#define PROGRAM WORKMAN_NAME
#define VERSION WORKMAN_VERSION

struct wm_cddb cddb;

int cur_cddb_protocol;
char *cur_cddb_server;
char *cur_cddb_mail_adress;
char *cur_cddb_path_to_cgi;
char *cur_cddb_proxy_server;

int Socket;
FILE *Connection;

/*
 *
 */
void
cddb_cur2struct(void)
{
	cddb.protocol = cur_cddb_protocol;
	strcpy(cddb.cddb_server,  cur_cddb_server);
	strcpy(cddb.mail_adress,  cur_cddb_mail_adress);
	strcpy(cddb.path_to_cgi,  cur_cddb_path_to_cgi);
	strcpy(cddb.proxy_server, cur_cddb_proxy_server);
} /* cddb_cur2struct() */

/*
 *
 */
void
cddb_struct2cur(void)
{
	cur_cddb_protocol = cddb.protocol;
	cur_cddb_server =       (cddb.cddb_server);
	cur_cddb_mail_adress =  (cddb.mail_adress);
	cur_cddb_path_to_cgi =  (cddb.path_to_cgi);
	cur_cddb_proxy_server = (cddb.proxy_server);
} /* cddb_struct2cur() */


/*
 * Subroutine from cddb_discid
 */
int
cddb_sum(int n)
{
	char	buf[12],
		*p;
	int	ret = 0;

	/* For backward compatibility this algorithm must not change */
	sprintf(buf, "%lu", (unsigned long)n);
	for (p = buf; *p != '\0'; p++)
	  ret += (*p - '0');

	return (ret);
} /* cddb_sum() */


/*
 * Calculate the discid of a CD according to cddb
 */
unsigned long
cddb_discid(void)
{
	int	i,
		t,
		n = 0;

	/* For backward compatibility this algorithm must not change */
	for (i = 0; i < thiscd.ntracks; i++) {
	    
		n += cddb_sum(thiscd.trk[i].start / 75);
	/* 
	 * Just for demonstration (See below)
	 * 
	 *	t += (thiscd.trk[i+1].start / 75) -
	 *	     (thiscd.trk[i  ].start / 75);
	 */	    
	}

	/*
	 * Mathematics can be fun. Example: How to reduce a full loop to
	 * a simple statement. The discid algorhythm is so half-hearted
	 * developed that it doesn't even use the full 32bit range.
	 * But it seems to be always this way: The bad standards will be
	 * accepted, the good ones turned down.
         * Boy, you pulled out the /75. This is not correct here, because
         * this calculation needs the integer division for both .start
         * fields.
         */

        t = (thiscd.trk[thiscd.ntracks].start / 75)
          - (thiscd.trk[0].start / 75);
	return ((n % 0xff) << 24 | t << 8 | thiscd.ntracks);
} /* cddb_discid() */

/*
 * Split a string into two components according to the first occurance of
 * the delimiter.
 */
char *
string_split(char *line, char delim)
{
	char *p1;

	for (p1=line;*p1;p1++)
	{
		if(*p1 == delim)
		{
			*p1 = 0;
			return ++p1;
		}
	}
	return (NULL);
} /* string_split() */

/*
 * Generate the hello string according to the cddb protocol
 * delimiter is either ' ' (cddbp) or '+' (http)
 */
void
string_makehello(char *line,char delim)
{
	char mail[84],*host;
	
	strcpy(mail,cddb.mail_adress);
	host=string_split(mail,'@');
	
	sprintf(line,"%shello%c%s%c%s%c%s%c%s",
		delim == ' ' ? "cddb " : "&",
		delim == ' ' ? ' ' : '=',
		mail,delim,
		host,delim,
		PROGRAM,delim,
		VERSION);
} /* string_makehello() */

/*
 * Open the TCP connection to the cddb/proxy server
 */
int
connect_open(void)
{
	char *host;
	struct hostent *hp;
	struct sockaddr_in soc_in;
	int port;
	
	if(cddb.protocol == 3) /* http proxy */
	  host = wm_strdup(cddb.proxy_server);
	else
	  host = wm_strdup(cddb.cddb_server);
	/*	
	 * t=string_split(host,':'); 
	 */
	port=atoi(string_split(host,':'));
	if(!port)
	  port=8880;

	printf("%s:%d\n",host,port);
	hp  =gethostbyname(host);
  
	if (hp == NULL)
	{
		static struct hostent def;
		static struct in_addr defaddr;
		static char *alist[1];
		static char namebuf[128];
		int inet_addr();
		
		defaddr.s_addr = inet_addr(host);
		if (defaddr.s_addr == -1) 
		{
			printf("unknown host: %s\n", host);
			return (-1);
		}
		strcpy(namebuf, host);
		def.h_name = namebuf;
		def.h_addr_list = alist, def.h_addr = (char *)&defaddr;
		def.h_length = sizeof (struct in_addr);
		def.h_addrtype = AF_INET;
		def.h_aliases = 0;
		hp = &def;
	}
	soc_in.sin_family = hp->h_addrtype;
	bcopy(hp->h_addr, (char *)&soc_in.sin_addr, hp->h_length);
	soc_in.sin_port = htons(port);
	Socket = socket(hp->h_addrtype, SOCK_STREAM, 0);
	if (Socket < 0) 
	{ 
		perror("socket");
		return (-1);
	}
	fflush(stdout);
	if (connect(Socket, (struct sockaddr *)&soc_in, sizeof (soc_in)) < 0) 
	{   
		perror("connect");
		close(Socket);
		return (-1);
	}
	
	Connection = fdopen(Socket, "r");
	return (0);
} /* connect_open() */


/*
 * Close the connection
 */
void
connect_close(void)
{	   
	(void)fclose(Connection);
	close(Socket);
} /* connect_close() */

/*
 * Get a line from the connection with CR and LF stripped
 */
void
connect_getline(char *line)
{
	char c;
    
	while ((c = getc(Connection)) != '\n')
	{
		*line = c;
		if ((c != '\r') && (c != (char)0xff))
		  line++;
	}
	*line=0;
} /* connect_getline() */

/*
 * Read the CD data from the server and place them into the cd struct
 */
void
connect_read_entry(void)
{
	char type;
	int trknr;
	
	char *t,*t2,tempbuf[2000];
	
	while(strcmp(tempbuf,"."))
	{
		connect_getline(tempbuf);
		
		t=string_split(tempbuf,'=');
		if(t != NULL)
		{
			type=tempbuf[0];
			
			if(strncmp("TITLE",tempbuf+1,5))
			  continue;
			
			if('D' == type)
			{
				/*
				 * Annahme: "Interpret / Titel" ist falsch.
				 * Daher: NULL-String erwarten.
                                 */
				t2=string_split(t,'/');
				if(t2 == NULL)
					t2 = t;
				if(*t2 == ' ')
				  t2++;
				strcpy(cd->cdname,t2);
				
				for(t2=t;*t2;t2++)
				{
					if((*t2 == ' ') && (*(t2+1) == 0))
					  *t2=0;
				}
				strcpy(cd->artist,t);
			}
			
			if('T' == type)
			{
				trknr=atoi(tempbuf+6);
				/*
				 * printf("Track %d:%s\n",trknr,t);
				 */
				wm_strmcpy(&cd->trk[trknr].songname,t);
			}
			/*
			 * fprintf(stderr, "%s %s\n",tempbuf,t);
			 */
		}
	}
} /* connect_read_entry() */

/*
 * Send a command to the server using cddbp
 */
void
cddbp_send(char *line)
{
	write(Socket, line, strlen(line));
	write(Socket, "\n", 1);
} /* cddbp_send() */

/*
 * Send the "read from cddb" command to the server using cddbp
 */
void
cddbp_read(char *category, unsigned int id)
{
	char tempbuf[84];
	sprintf(tempbuf, "cddb read %s %08x", category, id);
	cddbp_send(tempbuf);
} /* cddbp_read() */

/*
 * Send a command to the server using http
 */
void
http_send(char* line)
{
	char tempbuf[2000];
	
	write(Socket, "GET ", 4);
	printf("GET ");
	if(cddb.protocol == 3)
	{
		write(Socket, "http://", 7);
		write(Socket, cddb.cddb_server, strlen(cddb.cddb_server));
		printf("http://%s",cddb.cddb_server);
	}
	write(Socket, cddb.path_to_cgi, strlen(cddb.path_to_cgi));
	write(Socket, "?cmd=" ,5);
	write(Socket, line, strlen(line));
	printf("%s?cmd=%s",cddb.path_to_cgi,line);
	string_makehello(tempbuf,'+');
	write(Socket, tempbuf, strlen(tempbuf));
	printf("%s",tempbuf);
	write(Socket, "&proto=1 HTTP/1.0\n\n", 19);
	printf("&proto=1 HTTP/1.0\n");
	do
	  connect_getline(tempbuf);
	while(strcmp(tempbuf,""));
} /* http_send() */

/*
 * Send the "read from cddb" command to the server using http
 */
void
http_read(char *category, unsigned int id)
{
	char tempbuf[84];
	sprintf(tempbuf, "cddb+read+%s+%08x", category, id);
	http_send(tempbuf);
} /* http_read() */

/*
 * The main routine called from the ui
 */
void
cddb_request(void)
{
	int i;
	char tempbuf[2000];
	extern int cur_ntracks;
	
	int status;
	char category[20];
	unsigned int id;
	
	strcpy(cddb.cddb_server,"localhost:888");
	strcpy(cddb.mail_adress,"svolli@bigfoot.com");
	/*
	 * cddb.protocol = 1;
	 */
	wipe_cdinfo();
	
	switch(cddb.protocol)
	{
	 case 1: /* cddbp */
		printf("USING CDDBP\n");
		printf("open\n");
		connect_open();
		connect_getline(tempbuf);
		printf("[%s]\n",tempbuf);
		/*
		 * if(atoi(tempbuf) == 201) return;
		 */

		/*
		 * strcpy(tempbuf,"cddb hello svolli bigfoot.com Eierkratzer eins");
		 */
		string_makehello(tempbuf,' ');
		fprintf(stderr, "%s\n", tempbuf);
		cddbp_send(tempbuf);
		connect_getline(tempbuf);
		printf("[%s]\n",tempbuf);
		
		printf("query\n");
		sprintf(tempbuf, "cddb query %08x %d",thiscd.cddbid,thiscd.ntracks);
		for (i = 0; i < cur_ntracks; i++)
		  if (thiscd.trk[i].section < 2)
		    sprintf(tempbuf + strlen(tempbuf), " %d",
			    thiscd.trk[i].start);
		sprintf(tempbuf + strlen(tempbuf), " %d\n", thiscd.length);
		printf(">%s<\n",tempbuf);
		cddbp_send(tempbuf);
		connect_getline(tempbuf);
		printf("[%s]\n",tempbuf);
		
		status=atoi(tempbuf);
		/*
		 * fprintf(stderr, "status:%d\n",status);
		 * fprintf(stderr,"category:%s\n",category);
		 * fprintf(stderr,"id:%s\n",id);
		 */
		if(status == 200) /* Exact match */
		{
			sscanf(tempbuf,"%d %s %08x",&status,category,&id);
			cddbp_read(category,id);
			connect_read_entry();
		}
		
		if(status == 211) /* Unexact match, multiple possible
				   * Hack: always use first. */
		{
			connect_getline(tempbuf);
			sscanf(tempbuf,"%s %08x",category,&id);
			while(strcmp(tempbuf,"."))
			  connect_getline(tempbuf);
			cddbp_read(category,id);
			connect_read_entry();
		}
		
		cddbp_send("quit");
		connect_close();
		printf("close\n");
		break;
	 case 2: /* http */
	 case 3: /* http proxy */
		printf("USING HTTP%s\n",
		       (cddb.protocol == 3) ? " WITH PROXY" : "");
		printf("query\n");
		sprintf(tempbuf, "cddb+query+%08x+%d",thiscd.cddbid,thiscd.ntracks);
		for (i = 0; i < cur_ntracks; i++)
		  if (thiscd.trk[i].section < 2)
		    sprintf(tempbuf + strlen(tempbuf), "+%d",
			    thiscd.trk[i].start);
		sprintf(tempbuf + strlen(tempbuf), "+%d", thiscd.length);
		printf(">%s<\n",tempbuf);
		connect_open();
		http_send(tempbuf);
		connect_getline(tempbuf);
		printf("[%s]\n",tempbuf);
		
		status=atoi(tempbuf);
		/*
		 * fprintf(stderr, "status:%d\n",status);
		 * fprintf(stderr, "category:%s\n",category);
		 * fprintf(stderr, "id:%s\n",id);
		 */

		if(status == 200) /* Exact match */
		{
			connect_close();
			connect_open();
			sscanf(tempbuf,"%d %s %08x",&status,category,&id);
			http_read(category,id);
			connect_read_entry();
		}
		
		if(status == 211) /* Unexact match, multiple possible
				   * Hack: always use first. */
		{
			connect_getline(tempbuf);
			sscanf(tempbuf,"%s %08x",category,&id);
			while(strcmp(tempbuf,"."))
			  connect_getline(tempbuf);
			connect_close();
			connect_open();
			http_read(category,id);
			connect_read_entry();
		}
		/* moved close above break */
		connect_close();
		break;
	 default: /* off */
		break;
	}
} /* cddb_request() */

