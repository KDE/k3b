/***************************************************************************
             isofs.h  -  include this file to use libisofs
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

#ifndef ISOFS_H
#define ISOFS_H

#include <sys/time.h>

#include "iso_fs.h"
#include "el_torito.h"

typedef long sector_t;

typedef struct _rr_entry {
	int		len;	/* length of structure */
	char	*name; /* Name from 'NM' */
	char 	*sl;   /* symbolic link data */
	time_t	t_creat;
	time_t  rr_st_mtime;
	time_t  rr_st_atime;
	time_t  rr_st_ctime;
	time_t  t_backup;
	time_t  t_expire;
	time_t  t_effect;
	int		mode;  /* POSIX file modes */
	int		nlink;
	int		uid;
	int		gid;
	int		serno;
	int		dev_major;
	int		dev_minor;
	int		pl;	/* parent location */
	int		cl; /* child location */
	int		re; /* relocated */
	char	z_algo[2]; /* zizofs algorithm */
	char	z_params[2]; /* zizofs parameters */
	int		z_size; /* zizofs real_size */
} rr_entry;

typedef struct _iso_vol_desc {
	struct _iso_vol_desc	*next;
	struct _iso_vol_desc	*prev;
	struct iso_volume_descriptor	data;	
} iso_vol_desc;

typedef struct _boot_entry {
	struct _boot_entry	*next;
	struct _boot_entry	*prev;
	struct _boot_entry  *parent;
	struct _boot_entry  *child;
	char	data[32];
} boot_entry;

typedef struct _boot_head {
	struct validation_entry	ventry;
	struct _boot_entry	*defentry;
	struct _boot_entry  *sections;
} boot_head;

/**
 * this callback function needs to read 'len' sectors from 'start' into 'buf' 
 */
typedef int readfunc(char *buf,sector_t start, int len,void *);

/**
 * ProcessDir uses this callback
 */
typedef int dircallback(struct iso_directory_record *,void *);

/**
 * Returns the Unix from the ISO9660 9.1.5 (7 bytes) time format
 * This function is from the linux kernel.
 * Set 'hs' to non-zero if it's a HighSierra volume
 */
time_t isodate_915(char * p, int hs);

/**
 * Returns the Unix time from the ISO9660 8.4.26.1 (17 bytes) time format
 * BUG: hundredth of seconds are ignored, because time_t has one second
 * resolution (I think it's no problem at all)
 * Set 'hs' to non-zero if it's a HighSierra volume
 */
time_t isodate_84261(char * p, int hs);

/**
 * Creates the linked list of the volume descriptors
 * 'sector' is the starting sector number of where the filesystem start
 * (starting sector of a session on a CD-ROM)
 * If the function fails, returns NULL
 * Don't forget to call FreeISO9660 after using the volume descriptor list!
 */
iso_vol_desc *ReadISO9660(readfunc *read,sector_t sector,void *udata);

/**
 * Frees the linked list of volume descriptors.
 */
void FreeISO9660(iso_vol_desc *data);

/**
 * Iterates over the directory entries. The directory is in 'buf',
 * the size of the directory is 'size'. 'callback' is called for each
 * directory entry with the parameter 'udata'.
 */
int ProcessDir(readfunc *read,int extent,int size,dircallback *callback,void *udata);

/**
 * Parses the System Use area and fills rr_entry with values
 */
int ParseRR(struct iso_directory_record *idr, rr_entry *rrentry);

/**
 * Frees the strings in 'rrentry'
 */
void FreeRR(rr_entry *rrentry);

/**
 * returns the joliet level from the volume descriptor
 */
int JolietLevel(struct iso_volume_descriptor *ivd);

/**
 * Returns the size of the boot image (in 512 byte sectors)
 */
int BootImageSize(readfunc *read,int media,sector_t start,int len,void *udata);

/**
 * Frees the boot catalog entries in 'boot'. If you ever called ReadBootTable,
 * then don't forget to call FreeBootTable!
 */
void FreeBootTable(boot_head *boot);

/**
 * Reads the boot catalog into 'head'. Don't forget to call FreeBootTable!
 */
int ReadBootTable(readfunc *read,sector_t sector, boot_head *head, void *udata);

#endif
