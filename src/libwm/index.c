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
 * Maintain an external index file for a WorkMan CD database.
 * Uses the Berkeley libdb library, available from ftp.cs.berkeley.edu.
 */

static char index_id[] = "$Id$";

#ifdef LIBDB

#include <stdio.h>
#include <stdlib.h>
#include <db.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>	/* for htonl() */
#include "include/wm_config.h"
#include "include/wm_index.h"

extern int suppress_locking;

/*
 * idx_find_entry()
 *
 * Find an entry in the index file.
 *
 * Input:
 *	file	Name of database file (text version).
 *	ntracks	Number of tracks on the CD we're looking for.
 *	tracks	Array of track start times.
 *	len	CD length in frames.
 *	fuzz	Fuzz factor (tolerance value).
 *	pos	Pointer to return value.
 *
 * Output:
 *	1	No matching record found.
 *	0	Record found; *pos contains offset into text file.
 *	-1	Index file out of date or inaccessible, or another error.
 */
int
idx_find_entry( char *file, int ntracks, int *tracks,
		int len, int fuzz, unsigned long *pos )
{
	unsigned long	dbpos;
	char		*indexname = NULL, keyval[8];
	int		c;
	FILE		*text;
	DB		*index;
	DBT		key, data;
	BTREEINFO	bti;

	/*
	 * First, see if the text file is accessible.  Lock it if so.
	 */
	text = fopen(file, "r");
	if (text == NULL)
		return (-1);
	if ((c = getc(text)) == EOF)
	{
		fclose(text);
		return (-1);
	}
	if (! suppress_locking)
		if (lockit(fileno(text), F_RDLCK))
		{
			fclose(text);
			return (-1);
		}

	/*
	 * Open the index file.
	 */
	indexname = malloc(strlen(file) + sizeof(".ind"));
	if (indexname == NULL)
	{
		fclose(text);
		return (-1);
	}
	strcpy(indexname, file);
	strcat(indexname, ".ind");
	bti.flags = 0;
	bti.cachesize = 0;
	bti.minkeypage = bti.maxkeypage = 0;
	bti.psize = bti.lorder = 0;
	bti.compare = NULL;
	bti.prefix = NULL;
	index = dbopen(indexname, O_RDONLY, 0666, DB_BTREE, &bti);
	free(indexname);
	if (index == NULL)
	{
		fclose(text);
		return (-1);
	}

	/*
	 * Search for the first matching entry.
	 */
	sprintf(keyval, "%07d", tracks[ntracks - 1] - fuzz);
	key.data = keyval;
	key.size = 7;
	if (c = (index->seq)(index, &key, &data, R_CURSOR))
	{
		(index->close)(index);
		fclose(text);
		return (c);
	}

	/*
	 * Now loop through all the possible matches, collecting them into
	 * memory.
	 */
	do {
		char	tracksline[750], *s;
		int	i, val;

		/* Hit the end of the valid entries? */
		sscanf(key.data, "%d", &val);
		if (val > tracks[ntracks - 1] + fuzz)
			break;

		dbpos = ntohl(*((unsigned long *) data.data));
		if (fseek(text, dbpos, 0))
			break;
		
		fgets(tracksline, sizeof(tracksline), text);
		if (strncmp(tracksline, "tracks ", 7))
			break;
		(void) strtok(tracksline, " \t");

		/* Got a valid tracks line.  See if it matches the CD. */
		s = strtok(NULL, " \t");
		if (s == NULL)
			break;
		if (atoi(s) != ntracks)
			continue;
		
		for (i = 0; i < ntracks; i++)
		{
			s = strtok(NULL, " \t");
			if (s == NULL)
				break;
			val = atoi(s);
			if (val + fuzz < tracks[i] || val - fuzz > tracks[i])
				break;
		}
		if (i != ntracks)
			continue;

		s = strtok(NULL, " \t");
		if (s == NULL)
			continue;
		val = atoi(s);
		if (val + fuzz / 75 < len / 75 || val + fuzz / 75 > len / 75)
			continue;
		
		/* XXX - add to sorted list! */
		*pos = dbpos;
		(index->close)(index);
		fclose(text);
		return (0);
	} while ((c = (index->seq)(index, &key, &data, R_NEXT)) == 0);

	if (c == 0)
	{
		/* An error. */
		(index->close)(index);
		fclose(text);
		return (-1);
	}

	(index->close)(index);
	fclose(text);
	return (1);
}

/*
 * idx_delete_entry()
 *
 * Delete an entry from the index file.
 *
 * Input:
 *	file	Name of database file (text version).
 *	track	Last track's start time (database key).
 *	fuzz	Fuzz factor (tolerance value).
 *	pos	Position of CD in database file.
 *
 * Output:
 *	1	No matching record found.
 *	0	Record deleted.
 *	-1	Index file out of date or inaccessible, or another error.
 *
 * Note: it is the caller's responsibility to do locking, as it's assumed
 * that this operation will accompany a modification of the main database
 * file and will need to be atomic with that modification.
 */
int
idx_delete_entry(char *file, int track, int fuzz, unsigned long pos )
{
	unsigned long	dbpos;
	char		*indexname = NULL, keyval[8];
	int		c, status;
	DB		*index;
	DBT		key, data;
	BTREEINFO	bti;

	/*
	 * Open the index file.
	 */
	indexname = malloc(strlen(file) + sizeof(".ind"));
	if (indexname == NULL)
		return (-1);

	strcpy(indexname, file);
	strcat(indexname, ".ind");

	bti.flags = 0;
	bti.cachesize = 0;
	bti.minkeypage = bti.maxkeypage = 0;
	bti.psize = bti.lorder = 0;
	bti.compare = NULL;
	bti.prefix = NULL;
	index = dbopen(indexname, O_RDWR, 0666, DB_BTREE, &bti);
	free(indexname);
	if (index == NULL)
		return (-1);

	/*
	 * Search for the first matching entry.
	 */
	sprintf(keyval, "%07d", track - fuzz);
	key.data = keyval;
	key.size = 7;
	if (c = (index->seq)(index, &key, &data, R_CURSOR))
	{
		/*
		 * Nothing matched!
		 */
		(index->close)(index);
		return (c);
	}

	/*
	 * Look for the entry the user wants to delete.
	 */
	do {
		int	val;

		/* Hit the end of the valid entries? */
		sscanf(key.data, "%d", &val);
		if (val > track + fuzz)
			break;

		/* Is this the entry we want? */
		if (pos == ntohl(*((unsigned long *) data.data)))
		{
			/*
			 * Yep!  Delete it.
			 */
			status = (index->del)(index, &key, R_CURSOR);
			(index->close)(index);
			return (status);
		}
	} while ((c = (index->seq)(index, &key, &data, R_NEXT)) == 0);

	if (c == 0)
	{
		/* An error. */
		(index->close)(index);
		return (-1);
	}

	(index->close)(index);
	return (1);
}

/*
 * idx_write_entry()
 *
 * Write out an index file entry.
 *
 * Input:
 *	file	Name of database file (text version).
 *	track	Start time of last track (database key).
 *	pos	Position of entry in text file.
 *
 * Output:
 *	0	Record written.
 *	-1	Index file inaccessible, or another error.
 *
 * Note: it is the caller's responsibility to do locking, as it's assumed
 * that this operation will accompany a modification of the main database
 * file and will need to be atomic with that modification.
 */
int
idx_write_entry( char *file, int track, unsigned long pos )
{
	char		*indexname, keyval[8];
	int		status;
	DB		*index;
	DBT		key, data;
	BTREEINFO	bti;

	/*
	 * Open the index file.
	 */
	indexname = malloc(strlen(file) + sizeof(".ind"));
	if (indexname == NULL)
		return (-1);

	strcpy(indexname, file);
	strcat(indexname, ".ind");

	bti.flags = R_DUP;
	bti.cachesize = 0;
	bti.minkeypage = 0;
	bti.maxkeypage = 0;
	bti.psize = 0;
	bti.lorder = 4321;	/* network byte order */
	bti.compare = NULL;
	bti.prefix = NULL;
	index = dbopen(indexname, O_RDWR, 0666, DB_BTREE, &bti);
	free(indexname);
	if (index == NULL)
		return (-1);

	/*
	 * Create a new key and value.
	 */
	pos = htonl(pos);
	data.data = &pos;
	data.size = sizeof(pos);
	key.data = keyval;
	key.size = 7;

	sprintf(keyval, "%07d", track);

	status = (index->put)(index, &key, &data, 0);

	(index->close)(index);
	return (status);
}

#else /* LIBDB */

int
idx_find_entry()
{
	return (1);	/* no record found; text file will be searched. */
}

int
idx_delete_entry()
{
	return (0);
}

int
idx_write_entry()
{
	return (0);
}

#endif /* LIBDB */
