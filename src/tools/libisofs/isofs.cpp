/***************************************************************************
                 	isofs.c  -  libisofs implementation
                             -------------------
    begin                : Oct 25 2002
    copyright            : (C) 2002 by Szombathelyi György
    email                : gyurco@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "isofs.h"
#include "rock.h"



/* internal function from the linux kernel (isofs fs) */
static time_t getisotime(int year,int month,int day,int hour,
						 int minute,int second,int tz) {

	int days, i;
	time_t crtime;

	year-=1970;
	
	if (year < 0) {
		crtime = 0;
	} else {
		int monlen[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

		days = year * 365;
		if (year > 2)
			days += (year+1) / 4;
		for (i = 1; i < month; i++)
			days += monlen[i-1];
		if (((year+2) % 4) == 0 && month > 2)
			days++;
		days += day - 1;
		crtime = ((((days * 24) + hour) * 60 + minute) * 60)
			+ second;

		/* sign extend */
		if (tz & 0x80)
			tz |= (-1 << 8);
			
		/* 
		 * The timezone offset is unreliable on some disks,
		 * so we make a sanity check.  In no case is it ever
		 * more than 13 hours from GMT, which is 52*15min.
		 * The time is always stored in localtime with the
		 * timezone offset being what get added to GMT to
		 * get to localtime.  Thus we need to subtract the offset
		 * to get to true GMT, which is what we store the time
		 * as internally.  On the local system, the user may set
		 * their timezone any way they wish, of course, so GMT
		 * gets converted back to localtime on the receiving
		 * system.
		 *
		 * NOTE: mkisofs in versions prior to mkisofs-1.10 had
		 * the sign wrong on the timezone offset.  This has now
		 * been corrected there too, but if you are getting screwy
		 * results this may be the explanation.  If enough people
		 * complain, a user configuration option could be added
		 * to add the timezone offset in with the wrong sign
		 * for 'compatibility' with older discs, but I cannot see how
		 * it will matter that much.
		 *
		 * Thanks to kuhlmav@elec.canterbury.ac.nz (Volker Kuhlmann)
		 * for pointing out the sign error.
		 */
		if (-52 <= tz && tz <= 52)
			crtime -= tz * 15 * 60;
	}
	return crtime;

}

/**
 * Returns the Unix from the ISO9660 9.1.5 time format
 */
time_t isodate_915(char * p, int hs) {

	return getisotime(1900+p[0],p[1],p[2],p[3],p[4],p[5],hs==0 ? p[6] : 0);
}		

/**
 * Returns the Unix from the ISO9660 8.4.26.1 time format
 * BUG: hundredth of seconds are ignored, because Unix time_t has one second
 * resolution (I think it's no problem at all)
 */
time_t isodate_84261(char * p, int hs) {
	int year,month,day,hour,minute,second;
	year=(p[0]-'0')*1000 + (p[1]-'0')*100 + (p[2]-'0')*10 + p[3]-'0';
	month=(p[4]-'0')*10 + (p[5]-'0');
	day=(p[6]-'0')*10 + (p[7]-'0');
	hour=(p[8]-'0')*10 + (p[9]-'0');
	minute=(p[10]-'0')*10 + (p[11]-'0');
	second=(p[12]-'0')*10 + (p[13]-'0');
	return getisotime(year,month,day,hour,minute,second,hs==0 ? p[16] : 0);
}

void FreeBootTable(boot_head *boot) {
	boot_entry *be,*next;

	be=boot->defentry;
	while (be) {
		next=be->next;
		free(be);
		be=next;	
	}
	boot->defentry=NULL;
}

int BootImageSize(readfunc* read,int media,sector_t start,int len,void* udata) {
	int ret;

	switch(media & 0xf) {
		case 0:
			ret=len; /* No emulation */
			break;
		case 1:
			ret=80*2*15; /* 1.2 MB */
			break;
		case 2:
			ret=80*2*18; /* 1.44 MB */
			break;
		case 3:
			ret=80*2*36; /* 2.88 MB */
			break;
		case 4:
			/* FIXME!!! */
			ret=len; /* Hard Disk */
			break;
		default:
			ret=len;
	}	
	return ret;
}

static boot_entry *CreateBootEntry(char *be) {
	boot_entry *entry;
	
	entry = (boot_entry*) malloc(sizeof(boot_entry));
	if (!entry) return NULL;
	memset(entry, 0, sizeof(boot_entry));
	memcpy(entry->data,be,0x20);
	return entry;
}

int ReadBootTable(readfunc *read,sector_t sector, boot_head *head, void *udata) {

	char buf[2048], *c, *be;
	int i,end=0;
	unsigned short sum;
	boot_entry *defcur=NULL,*deflast=NULL;
	register struct validation_entry *ventry=NULL;
	register struct default_entry *dentry=NULL;
	register struct section_header *sheader=NULL;
	register struct section_entry *sentry=NULL;
	register struct section_entry_ext *extsentry=NULL;
	
	head->sections=NULL;
	head->defentry=NULL;
	while (1) {
		be = (char*) &buf;
		if ( read(be, sector, 1, udata) != 1 ) goto err;

		/* first entry needs to be a validation entry */
		if (!ventry) {
			ventry=(struct validation_entry *) be;
			if ( isonum_711(ventry->type) !=1 ) goto err;
			sum=0;
			c = (char*) ventry;
			for (i=0;i<16;i++) { sum += isonum_721(c); c+=2; }
			if (sum) goto err;
			memcpy(&head->ventry,be,0x20);
			be += 0x20;
		}

		while (!end && (be < (buf+1))) {
			switch (isonum_711(be)) {
				case 0x88:
					defcur=CreateBootEntry(be);
					if (!defcur) goto err;
					if (deflast)
						deflast->next=defcur;
					else
						head->defentry=defcur;
					defcur->prev=deflast;
					deflast=defcur;
					break;	
				case 0x90:
				case 0x91:
					break;
				default:
					end=1;
					break;
			}
			be += 0x20;
		}
		if (end) break;

		sector ++;		
	}

	return 0;

err:
	FreeBootTable(head);
	return -1;
}


/**
 * Creates the linked list of the volume descriptors
 */
iso_vol_desc *ReadISO9660(readfunc *read,sector_t sector,void *udata) {
				
	int i;
	struct iso_volume_descriptor buf;
	iso_vol_desc *first=NULL,*current=NULL,*prev=NULL;

	for (i=0;i<100;i++) {
		if (read( (char*) &buf, sector+i+16, 1, udata) != 1 ) {
			FreeISO9660(first);
			return NULL;
		}
		if (!memcmp(ISO_STANDARD_ID,&buf.id,5)) {
			switch ( isonum_711(&buf.type[0]) ) {

				case ISO_VD_BOOT:
				case ISO_VD_PRIMARY:
				case ISO_VD_SUPPLEMENTARY:
					current=(iso_vol_desc*) malloc(sizeof(iso_vol_desc));
					if (!current) {
						FreeISO9660(first);
						return NULL;
					}
					current->prev=prev;
					current->next=NULL;
					if (prev) prev->next=current;
					memcpy(&(current->data),&buf,2048);
					if (!first) first=current;
					prev=current;
					break;

				case ISO_VD_END:
					return first;
					break;
			}
		} else if (!memcmp(HS_STANDARD_ID,(struct hs_volume_descriptor*) &buf,5)) {
			/* High Sierra format not supported (yet) */
		}
	}
	
	return first;
}

/**
 * Frees the linked list of volume descriptors
 */
void FreeISO9660(iso_vol_desc *data) {

	iso_vol_desc *current;

	
	while (data) {
		current=data;	
		data=current->next;
		free(current);
	}
}

/**
 * Frees the strings in 'rrentry'
 */
void FreeRR(rr_entry *rrentry) {
	if (rrentry->name) {
		free(rrentry->name);
		rrentry->name=NULL;
	}
	if (rrentry->sl) {
		free(rrentry->sl);
		rrentry->name=NULL;
	}
}

static int str_nappend(char **d,char *s,int n) {
	int i=0;
	char *c;
	
/*	i=strnlen(s,n)+1; */
	while (i<n && s[i]) i++;
	i++;
	if (*d) i+=(strlen(*d)+1);
	c=(char*) malloc(i);
	if (!c) return -ENOMEM;
	if (*d) {
		strcpy(c,*d);
		strncat(c,s,n);

		free(*d);
	} else
		strncpy(c,s,n);
	c[i-1]=0;
	*d=c;
	return 0;
}

static int str_append(char **d, const char *s) {
	int i;
	char *c;
	
	i=strlen(s)+1;
	if (*d) i+=(strlen(*d)+1);
	c=(char*) malloc(i);
	if (!c) return -ENOMEM;
	if (*d) {
		strcpy(c,*d);
		strcat(c,s);
		free(*d);
	} else
		strcpy(c,s);
	c[i-1]=0;
	*d=c;
	return 0;
}

#define rrtlen(c) (((unsigned char) c & 0x80) ? 17 : 7)
#define rrctime(f,c) ((unsigned char) f & 0x80) ? isodate_84261(c,0) : isodate_915(c,0)
/**
 * Parses the System Use area and fills rr_entry with values
 */
int ParseRR(struct iso_directory_record *idr, rr_entry *rrentry) {

	int suspoffs,susplen,i,f,ret=0;
	char *r, *c;
	struct rock_ridge *rr;

	suspoffs=33+isonum_711(idr->name_len);
	if (!(isonum_711(idr->name_len) & 1)) suspoffs++;
	susplen=isonum_711(idr->length)-suspoffs;
	r= & (((char*) idr)[suspoffs]);
	rr = (struct rock_ridge*) r;

	memset(rrentry,0,sizeof(rr_entry));
	rrentry->len = sizeof(rr_entry);	

	while (susplen > 0) {
		if (isonum_711(&rr->len) > susplen || rr->len == 0) break;
		if (rr->signature[0]=='N' && rr->signature[1]=='M') {
			if (!(rr->u.NM.flags & 0x26) && rr->len>5 && !rrentry->name) {
				
				if (str_nappend(&rrentry->name,rr->u.NM.name,isonum_711(&rr->len)-5)) {
					FreeRR(rrentry); return -ENOMEM;
				}
				ret++;
			}
		} else if (rr->signature[0]=='P' && rr->signature[1]=='X' && 
			(isonum_711(&rr->len)==44 || isonum_711(&rr->len)==36)) {
				rrentry->mode=isonum_733(rr->u.PX.mode);
				rrentry->nlink=isonum_733(rr->u.PX.n_links);
				rrentry->uid=isonum_733(rr->u.PX.uid);
				rrentry->gid=isonum_733(rr->u.PX.gid);
				if (isonum_711(&rr->len)==44) rrentry->serno=isonum_733(rr->u.PX.serno);
				ret++;
		} else if (rr->signature[0]=='P' && rr->signature[1]=='N' && 
			isonum_711(&rr->len)==20) {
				rrentry->dev_major=isonum_733(rr->u.PN.dev_high);
				rrentry->dev_minor=isonum_733(rr->u.PN.dev_low);
				ret++;
		} else if (rr->signature[0]=='P' && rr->signature[1]=='L' && 
			isonum_711(&rr->len)==12) {
				rrentry->pl=isonum_733(rr->u.PL.location);
				ret++;
		} else if (rr->signature[0]=='C' && rr->signature[1]=='L' && 
			isonum_711(&rr->len)==12) {
				rrentry->cl=isonum_733(rr->u.CL.location);
				ret++;
		} else if (rr->signature[0]=='R' && rr->signature[1]=='E' && 
			isonum_711(&rr->len)==4) {
				rrentry->re=1;
				ret++;
		} else if (rr->signature[0]=='S' && rr->signature[1]=='L' &&
			isonum_711(&rr->len)>7) {
			i = isonum_711(&rr->len)-5;
			c = (char*) rr;
			c += 5;
			while (i>0) {
				switch(c[0] & ~1) {
					case 0x2:
						if (str_append(&rrentry->sl,".")) {
							FreeRR(rrentry); return -ENOMEM;
						}
						break;
					case 0x4:
						if (str_append(&rrentry->sl,"..")) {
							FreeRR(rrentry); return -ENOMEM;
						}
						break;
				}
				if ( (c[0] & 0x08) == 0x08 || (c[1] && rrentry->sl && 
					 strlen(rrentry->sl)>1) ) {
					if (str_append(&rrentry->sl,"/")) {
						FreeRR(rrentry); return -ENOMEM;
					}
				}

				if ((unsigned char)c[1]>0) {
					if (str_nappend(&rrentry->sl,c+2,(unsigned char)c[1])) {
						FreeRR(rrentry); return -ENOMEM;
					}
				}
			 	i -= ((unsigned char)c[1] + 2);
				c += ((unsigned char)c[1] + 2);
			}
			ret++;
		} else if (rr->signature[0]=='T' && rr->signature[1]=='F' && 
			isonum_711(&rr->len)>5) {

			i = isonum_711(&rr->len)-5;
			f = rr->u.TF.flags;
			c = (char*) rr;
			c += 5;

			while (i >= rrtlen(f)) {
				if (f & 1) {
					rrentry->t_creat=rrctime(f,c);
					f &= ~1;
				} else if (f & 2) {
					rrentry->rr_st_mtime=rrctime(f,c);
					f &= ~2;
				} else if (f & 4) {
					rrentry->rr_st_atime=rrctime(f,c);
					f &= ~4;
				} else if (f & 8) {
					rrentry->rr_st_ctime=rrctime(f,c);
					f &= ~8;
				} else if (f & 16) {
					rrentry->t_backup=rrctime(f,c);
					f &= ~16;
				} else if (f & 32) {
					rrentry->t_expire=rrctime(f,c);
					f &= ~32;
				} else if (f & 64) {
					rrentry->t_effect=rrctime(f,c);
					f &= ~64;
				}

				i -= rrtlen(f);
				c += rrtlen(f);
			}
			ret++;

		} else if (rr->signature[0]=='Z' && rr->signature[1]=='F' && 
			isonum_711(&rr->len)==16) {
				/* Linux-specific extension: transparent decompression */
				rrentry->z_algo[0]=rr->u.ZF.algorithm[0];
				rrentry->z_algo[1]=rr->u.ZF.algorithm[1];
				rrentry->z_params[0]=rr->u.ZF.parms[0];
				rrentry->z_params[1]=rr->u.ZF.parms[1];
				rrentry->z_size=isonum_733(rr->u.ZF.real_size);
				ret++;
		} else {
/*			printf("SUSP sign: %c%c\n",rr->signature[0],rr->signature[1]); */
		}
		
		susplen -= isonum_711(&rr->len);
		r += isonum_711(&rr->len);
		rr = (struct rock_ridge*) r;
	}

	return ret;
}

/**
 * Iterates over the directory entries. The directory is in 'buf',
 * the size of the directory is 'size'. 'callback' is called for each
 * directory entry with the parameter 'udata'.
 */
int ProcessDir(readfunc *read,int extent,int size,dircallback *callback,void *udata) {

	int pos=0,ret=0,siz;
	char *buf;
	struct iso_directory_record *idr;

	if (size & 2047) {
		siz=((size>>11)+1)<<11;
	} else {
		siz=size;
	}

	buf=(char*) malloc(siz);
	if (!buf) return -ENOMEM;
	if (read(buf,extent,siz>>11,udata)!=siz>>11) {
		free(buf);
		return -EIO;
	}

	while (size>0) {
		idr=(struct iso_directory_record*) &buf[pos];
		if (isonum_711(idr->length)==0) {
			size-=(2048 - (pos & 0x7ff));
			if (size<=2) break;
			pos+=0x800;
			pos&=0xfffff800;
			idr=(struct iso_directory_record*) &buf[pos];
		}
		pos+=isonum_711(idr->length);
		pos+=isonum_711(idr->ext_attr_length);
		size-=isonum_711(idr->length);
		size-=isonum_711(idr->ext_attr_length);
		if (size<0) break;
		
		if (isonum_711(idr->length)
<33 ||
			isonum_711(idr->length)<33+isonum_711(idr->name_len)) {
			/* Invalid directory entry */			
			continue;
		}
		if ((ret=callback(idr,udata))) break;
	}

	free(buf);
	return ret;	
}

/**
 * returns the joliet level from the volume descriptor
 */
int JolietLevel(struct iso_volume_descriptor *ivd) {
	int ret=0;
	register struct iso_supplementary_descriptor *isd;
	
	isd = (struct iso_supplementary_descriptor *) ivd;
	
	if (isonum_711(ivd->type)==ISO_VD_SUPPLEMENTARY) {
		if (isd->escape[0]==0x25 && 
			isd->escape[1]==0x2f) {

			switch (isd->escape[2]) {
				case 0x40:
					ret=1;
					break;
				case 0x43:
					ret=2;
					break;
				case 0x45:
					ret=3;
					break;
			}
		}
	}
	return ret;
}

/********************************************************************/
#if 0

#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iconv.h>

int level=0,joliet=0,dirs,files;
iconv_t iconv_d;
int fd;

int readf(char *buf, int start, int len,void *udata) {
	int ret;
	
	if ((ret=lseek(fd, start << 11, SEEK_SET))<0) return ret;
	ret=read(fd, buf, len << 11);
	if (ret<0) return ret;
	return (ret >> 11);
}

void dumpchars(char *c,int len) {
	while (len>0) {
		printf("%c",*c);
		len--;
		c++;
	}	
}

void sp(int num) {
	int i;
	for (i=0;i<num*5;i++) { printf(" "); };
}

void dumpflags(char flags) {
	if (flags & 1) printf("HIDDEN ");
	if (flags & 2) printf("DIR ");
	if (flags & 4) printf("ASF ");
}

void dumpjoliet(char *c,int len) {

	char outbuf[255];
	size_t out;
	int ret;
	char *outptr;
	
	outptr=(char*) &outbuf;
	out=255;
	if ((iconv(iconv_d,&c,&len,&outptr,&out))<0) {
		printf("conversion error=%d",errno);
		return;
	}
	ret=255-out;
	dumpchars((char*) &outbuf,ret);
}

void dumpchardesc(char *c,int len) {

	if (joliet) 
		dumpjoliet(c,len);
	else {
		dumpchars(c,len);
	}
}

void dumpiso915time(char *t, int hs) {
	
	time_t time;
	char *c;
	
	time=isodate_915(t,hs);
	c=(char*) ctime(&time);
	if (c && c[strlen(c)-1]==0x0a) c[strlen(c)-1]=0;
	if (c) printf("%s",c);
}

void dumpiso84261time(char *t, int hs) {
	
	time_t time;
	char *c;
	
	time=isodate_84261(t,hs);
	c=(char*) ctime(&time);
	if (c && c[strlen(c)-1]==0x0a) c[strlen(c)-1]=0;
	if (c) printf("%s",c);
}

void dumpdirrec(struct iso_directory_record *dir) {
	
	if (isonum_711(dir->name_len)==1) {
		switch (dir->name[0]) {
		case 0:
			printf(".");
			break;
		case 1:
			printf("..");
			break;
		default:
			printf("%c",dir->name[0]);	
			break;
		}
	}
	dumpchardesc(dir->name,isonum_711(dir->name_len));
	printf(" size=%d",isonum_733(dir->size));
	printf(" extent=%d ",isonum_733(dir->extent));
	dumpflags(isonum_711(dir->flags));
	dumpiso915time((char*) &(dir->date),0);
}

void dumprrentry(rr_entry *rr) {
	printf("  NM=[%s] uid=%d gid=%d nlink=%d mode=%o ",
		rr->name,rr->uid,rr->gid,rr->nlink,rr->mode);
	if (S_ISCHR(rr->mode) || S_ISBLK(rr->mode))
		printf("major=%d minor=%d ",rr->dev_major,rr->dev_minor);
	if (rr->mode & S_IFLNK && rr->sl) printf("slink=%s ",rr->sl);
/*
	printf("\n");
	if (rr->t_creat) printf("t_creat: %s",ctime(&rr->t_creat));
	if (rr->rr_st_mtime) printf("rr_st_mtime: %s",ctime(&rr->rr_st_mtime));
	if (rr->rr_st_atime) printf("rr_st_atime: %s",ctime(&rr->rr_st_atime));
	if (rr->rr_st_ctime) printf("rr_st_ctime: %s",ctime(&rr->rr_st_ctime));
	if (rr->t_backup) printf("t_backup: %s",ctime(&rr->t_backup));
	if (rr->t_expire) printf("t_expire: %s",ctime(&rr->t_expire));
	if (rr->t_effect) printf("t_effect: %s",ctime(&rr->t_effect));
*/
}

void dumpsusp(char *c, int len) {
	dumpchars(c,len);
}

void dumpboot(struct el_torito_boot_descriptor *ebd) {
	printf("version: %d\n",isonum_711(ebd->version));
	printf("system id: ");dumpchars(ebd->system_id,ISODCL(8,39));printf("\n");
	printf("boot catalog start: %d\n",isonum_731(ebd->boot_catalog));
}

void dumpdefentry(struct default_entry *de) {
	printf("Default entry: \n");
	printf("  bootid=%x\n",isonum_711(de->bootid));
	printf("  media emulation=%d (",isonum_711(de->media));
	switch(isonum_711(de->media) & 0xf) {
		case 0:
			printf("No emulation");
			break;
		case 1:
			printf("1.2 Mb floppy");
			break;
		case 2:
			printf("1.44 Mb floppy");
			break;
		case 3:
			printf("2.88 Mb floppy");
			break;
		case 4:
			printf("Hard Disk");
			break;
		default:
			printf("Unknown/Invalid");
			break;
	}	
	printf(")\n");
	printf("  loadseg=%d\n",isonum_721(de->loadseg));
	printf("  systype=%d\n",isonum_711(de->systype));
	printf("  start lba=%d count=%d\n",isonum_731(de->start),
		isonum_721(de->seccount));
}

void dumpbootcat(boot_head *bh) {
	boot_entry *be;

	printf("System id: ");dumpchars(bh->ventry.id,ISODCL(28,5));printf("\n");
	be=bh->defentry;
	while (be) {
		dumpdefentry(be->data);
		be=be->next;
	}
}

void dumpdesc(struct iso_primary_descriptor *ipd) {

	printf("system id: ");dumpchardesc(ipd->system_id,ISODCL(9,40));printf("\n");
	printf("volume id: ");dumpchardesc(ipd->volume_id,ISODCL(41,72));printf("\n");
	printf("volume space size: %d\n",isonum_733(ipd->volume_space_size));
	printf("volume set size: %d\n",isonum_723(ipd->volume_set_size));
	printf("volume seq num: %d\n",isonum_723(ipd->volume_set_size));
	printf("logical block size: %d\n",isonum_723(ipd->logical_block_size));
	printf("path table size: %d\n",isonum_733(ipd->path_table_size));
	printf("location of type_l path table: %d\n",isonum_731(ipd->type_l_path_table));
	printf("location of optional type_l path table: %d\n",isonum_731(ipd->opt_type_l_path_table));
	printf("location of type_m path table: %d\n",isonum_732(ipd->type_m_path_table));		
	printf("location of optional type_m path table: %d\n",isonum_732(ipd->opt_type_m_path_table));		
/*
	printf("Root dir record:\n");dumpdirrec((struct iso_directory_record*) &ipd->root_directory_record);
*/
	printf("Volume set id: ");dumpchardesc(ipd->volume_set_id,ISODCL(191,318));printf("\n");
	printf("Publisher id: ");dumpchardesc(ipd->publisher_id,ISODCL(319,446));printf("\n");
	printf("Preparer id: ");dumpchardesc(ipd->preparer_id,ISODCL(447,574));printf("\n");
	printf("Application id: ");dumpchardesc(ipd->application_id,ISODCL(575,702));printf("\n");
	printf("Copyright id: ");dumpchardesc(ipd->copyright_file_id,ISODCL(703,739));printf("\n");
	printf("Abstract file id: ");dumpchardesc(ipd->abstract_file_id,ISODCL(740,776));printf("\n");
	printf("Bibliographic file id: ");dumpchardesc(ipd->bibliographic_file_id,ISODCL(777,813));printf("\n");
	printf("Volume creation date: ");dumpiso84261time(ipd->creation_date,0);printf("\n");
	printf("Volume modification date: ");dumpiso84261time(ipd->modification_date,0);printf("\n");
	printf("Volume expiration date: ");dumpiso84261time(ipd->expiration_date,0);printf("\n");
	printf("Volume effective date: ");dumpiso84261time(ipd->effective_date,0);printf("\n");
	printf("File structure version: %d\n",isonum_711(ipd->file_structure_version));
}

int mycallb(struct iso_directory_record *idr,void *udata) {
	rr_entry rrentry;

	sp(level);dumpdirrec(idr);
	if (level==0) printf(" (Root directory) ");
	printf("\n");
	
	if (ParseRR(idr,&rrentry)>0) {
		sp(level);printf("  ");dumprrentry(&rrentry);printf("\n");
	}
	FreeRR(&rrentry);
	if ( !(idr->flags[0] & 2) ) files++;
	if ( (idr->flags[0] & 2) && (level==0 || isonum_711(idr->name_len)>1) ) {
		level++;
		dirs++;
		ProcessDir(&readf,isonum_733(idr->extent),isonum_733(idr->size),&mycallb,udata);
		level--;
	}
	return 0;
}

/************************************************/

int main(int argc, char *argv[]) {
	
	int i=1,sector=0;
	iso_vol_desc *desc;
	boot_head boot;
	
	if (argc<2) {
		fprintf(stderr,"\nUsage: %s iso-file-name or device [starting sector]\n\n",argv[0]);
		return 0;
	}
	if (argc>=3) {
		sector=atoi(argv[2]);
		printf("Using starting sector number %d\n",sector);
	}
	fd=open(argv[1],O_RDONLY);
	if (fd<0) {
		fprintf(stderr,"open error\n");
		return -1;
	}
	iconv_d=iconv_open("ISO8859-2","UTF16BE");
	if (iconv_d==0) {
		fprintf(stderr,"iconv open error\n");
		return -1;
	}
	
	desc=ReadISO9660(&readf,sector,NULL);
	if (!desc) {
		printf("No volume descriptors\n");
		return -1;
	}
	while (desc) {
		
		printf("\n\n--------------- Volume descriptor (%d.) type %d: ---------------\n\n",
			i,isonum_711(desc->data.type));
		switch (isonum_711(desc->data.type)) {
			case ISO_VD_BOOT: {

				struct el_torito_boot_descriptor* bootdesc;
				bootdesc=&(desc->data);
				dumpboot(bootdesc);
				if ( !memcmp(EL_TORITO_ID,bootdesc->system_id,ISODCL(8,39)) ) {
					
					if (ReadBootTable(&readf,isonum_731(bootdesc->boot_catalog),&boot,NULL)) {
						printf("Boot Catalog Error\n");
					} else {
						dumpbootcat(&boot);
						FreeBootTable(&boot);
					}
				}
			}
				break;

			case ISO_VD_PRIMARY:
			case ISO_VD_SUPPLEMENTARY:
				joliet=0;
				joliet = JolietLevel(&desc->data);
				printf("Joliet level: %d\n",joliet);
				dumpdesc((struct iso_primary_descriptor*) &desc->data);
				printf("\n\n--------------- Directory structure: -------------------\n\n");
				dirs=0;files=0;
				mycallb( &( ((struct iso_primary_descriptor*) &desc->data)->root_directory_record), NULL );
				printf("\nnumber of directories: %d\n",dirs);
				printf("\nnumber of files: %d\n",files);
				break;

		}
		desc=desc->next;
		i++;
	}
	iconv_close(iconv_d);
	close(fd);
	FreeISO9660(desc);
	return 0;
}

#endif
