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
 * Manage the CD database.  All these routines assume that the "cd" global
 * structure contains current information (as far as the outside world knows;
 * obviously it won't contain track titles just after a CD is inserted.)
 */

static char database_id[] = "$Id$";

#define RCFILE "/.workmanrc"
#define DBFILE "/.workmandb"
#define FUZZFRAMES 75

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "include/wm_config.h"
#include "include/wm_helpers.h"
#include "include/wm_struct.h"
#include "include/wm_cdinfo.h"
#include "include/wm_cddb.h"
#include "include/wm_index.h"
#include "include/wm_database.h"

#define WM_MSG_CLASS WM_MSG_CLASS_DB

#define SWALLOW_LINE(fp) { int c; while ((c = getc(fp)) != '\n' && c != EOF); }

int	suppress_locking = 0;	/* Turn off locking of datafile (dangerous) */

char	*rcfile = NULL;		/* Personal rcfile */
char	*dbfiles = NULL;	/* Colon-separated list of databases */
char	**databases = NULL;	/* NULL-terminated list of databases */

char	*otherrc = NULL;	/* Unrecognized cruft from start of rcfile */

long	rcpos, rclen;		/* XXX */

int	found_in_db, found_in_rc;
long	holepos, firstpos;

int fuzz_frames = FUZZFRAMES;

int wm_db_save_disabled = FALSE;

int cur_playnew = -1;

extern int cur_ntracks, cur_nsections;

int mark_a = 0;
int mark_b = 0;


/*
 *
 */
int wm_db_get_playnew( void )
{
	return 0;
}

/*
 * split_workmandb()
 *
 * Split the WORKMANDB environment variable, if any, into a list of database
 * files in the global "databases".  If WORKMANDB is not available, make
 * a single entry with $HOME/DBFILE.
 *
 * Also, fill the "rcfile" global with the personal preferences filename.
 *
 * The environment variables should have already been read and placed in the
 * "rcfile" and "dbfiles" globals, respectively.
 */
void
split_workmandb( void )
{
	int	ndbs, i;
	char	*home, *wmdb;
	int	no_rc = 0, no_db = 0;
	
	if (rcfile == NULL)
	{
		if ((home = getenv("HOME")) != NULL)
		{
			rcfile = malloc(strlen(home) + sizeof(RCFILE));
			if (rcfile == NULL)
			{
			
nomem:
				perror("split_workmandb()");
				exit(1);
			}

			strcpy(rcfile, home);
			strcat(rcfile, RCFILE);
		}
		else
			no_rc = 1;

	}

	if ((wmdb = dbfiles) == NULL)
	{
		if ((home = getenv("HOME")) != NULL)
		{
			wmdb = malloc(strlen(home) + sizeof(DBFILE));
			if (wmdb == NULL)
				goto nomem;

			databases = malloc(2 * sizeof (databases[0]));
			if (databases == NULL)
				goto nomem;

			strcpy(wmdb, home);
			strcat(wmdb, DBFILE);
			databases[0] = wmdb;
			databases[1] = NULL;
		}
		else
		{
			static char *emptydb = NULL;
		
			databases = &emptydb;
			no_db = 1;
		}
	}
	else
	{
		ndbs = 1;
		for (home = wmdb; *home; home++)
			if (*home == ':')
			{
				*home = '\0';
				ndbs++;
			}
		
		databases = malloc((ndbs + 1) * sizeof(databases[0]));
		if (databases == NULL)
			goto nomem;
		
		for (i = 0; i < ndbs; i++)
		{
			databases[i] = wmdb;
			wmdb += strlen(wmdb) + 1;
		}

		databases[i] = NULL;
	}

	if (no_db || no_rc)
	{
		fprintf(stderr,
"WorkMan was run without a home directory, probably by a system daemon.\n");
		fprintf(stderr, "It doesn't know where to find ");
		if (no_rc)
		{
			fprintf(stderr, "your personal preferences file ");
			if (no_db)
				fprintf(stderr, "or the\ndatabase of CD descriptions");
		}
		else
			fprintf(stderr, "the database of CD descriptions");

		fprintf(stderr,
".\nYou can use the X resources \"workman.db.shared\" and \"workman.db.personal\"\nto tell WorkMan where to look.\n");

		wm_db_save_disabled = TRUE;
	}
}

/*
 * print_cdinfo(cd, prefs)
 *
 * cd		A pointer to a cdinfo struct.
 * prefs	Flag: write personal preferences?
 *
 * Print a CD's information (in more or less readable form) to a buffer.
 * Returns a pointer to the buffer.
 *
 * XXX - could be more efficient about calling wm_strmcat() and strlen().
 */
char *
print_cdinfo(struct wm_cdinfo *cd, int prefs)
{
	int		i;
	char		tempbuf[2000];	/* XXX - is this always big enough? */
	static char	*cdibuf = NULL;
	struct wm_playlist	*l;

	sprintf(tempbuf, "\ntracks %d", cd->ntracks);
	for (i = 0; i < cur_ntracks; i++)
		if (cd->trk[i].section < 2)
			sprintf(tempbuf + strlen(tempbuf), " %d",
				cd->trk[i].start);
	sprintf(tempbuf + strlen(tempbuf), " %d\n", cd->length);

	wm_strmcpy(&cdibuf, tempbuf);

	if (cur_nsections)
	{
		sprintf(tempbuf, "sections %d", cur_nsections);
		/* fixed a bug here */
		for (i = 0; i < cur_ntracks; i++)
			if (cd->trk[i].section > 1)
				sprintf(tempbuf + strlen(tempbuf), " %d",
					cd->trk[i].start);
		sprintf(tempbuf + strlen(tempbuf), "\n");

		wm_strmcat(&cdibuf, tempbuf);
	}

	if (prefs)
	{
		if (cd->autoplay)
			wm_strmcat(&cdibuf, "autoplay\n");
		for (l = cd->lists; l != NULL && l->name != NULL; l++)
		{
			wm_strmcat(&cdibuf, "playlist ");

			i = strlen(cdibuf) - 1;
			wm_strmcat(&cdibuf, l->name);
			while (cdibuf[++i])
				if (cdibuf[i] == ' ' || cdibuf[i] == '\t')
					cdibuf[i] = '_';

			if (l->list != NULL)
			{
				for (i = 0; l->list[i]; i++)
					;
				sprintf(tempbuf, " %d", i);
				wm_strmcat(&cdibuf, tempbuf);
				for (i = 0; l->list[i]; i++)
				{
					sprintf(tempbuf, " %d", l->list[i]);
					wm_strmcat(&cdibuf, tempbuf);
				}
				wm_strmcat(&cdibuf, "\n");
			}
			else
				wm_strmcat(&cdibuf, " 0\n");
		}

		if (cd->volume)
		{
			/*
			 * Have to maintain compatibility with old versions,
			 * where volume was 0-32.
			 */
			sprintf(tempbuf, "cdvolume %d\n", (cd->volume * 32) / 100);
			wm_strmcat(&cdibuf, tempbuf);
		}

		if (cd->playmode)
		{
			sprintf(tempbuf, "playmode %d\n", cd->playmode);
			wm_strmcat(&cdibuf, tempbuf);
		}

		if (mark_a)
		{
			sprintf(tempbuf, "mark %d START\n", mark_a);
			wm_strmcat(&cdibuf, tempbuf);
		}
		if (mark_b)
		{
			sprintf(tempbuf, "mark %d END\n", mark_b);
			wm_strmcat(&cdibuf, tempbuf);
		}

		if (cd->otherrc)
			wm_strmcat(&cdibuf, cd->otherrc);

		for (i = 0; i < cur_ntracks; i++)
		{
			if (cd->trk[i].avoid)
			{
				sprintf(tempbuf, "dontplay %d\n", i + 1);
				wm_strmcat(&cdibuf, tempbuf);
			}
			if (cd->trk[i].volume)
			{
				sprintf(tempbuf, "volume %d %d\n", i + 1,
					(cd->trk[i].volume * 32) / 100);
				wm_strmcat(&cdibuf, tempbuf);
			}
			if (cd->trk[i].otherrc)
				wm_strmcat(&cdibuf, cd->trk[i].otherrc);
		}
	}
	else
	{
		if (cd->cdname[0])
		{
			wm_strmcat(&cdibuf, "cdname ");
			wm_strmcat(&cdibuf, cd->cdname);
			wm_strmcat(&cdibuf, "\n");
		}

		if (cd->artist[0])
		{
			wm_strmcat(&cdibuf, "artist ");
			wm_strmcat(&cdibuf, cd->artist);
			wm_strmcat(&cdibuf, "\n");
		}

		if (cd->otherdb)
			wm_strmcat(&cdibuf, cd->otherdb);

		for (i = 0; i < cur_ntracks; i++)
		{
			if (cd->trk[i].section > 1)
				wm_strmcat(&cdibuf, "s-");
			wm_strmcat(&cdibuf, "track ");
			if (cd->trk[i].songname != NULL)
				wm_strmcat(&cdibuf, cd->trk[i].songname);
			wm_strmcat(&cdibuf, "\n");
			if (cd->trk[i].contd)
			{
				if (cd->trk[i].section > 1)
					wm_strmcat(&cdibuf, "s-");
				wm_strmcat(&cdibuf, "continue\n");
			}
			if (cd->trk[i].otherdb)
				wm_strmcat(&cdibuf, cd->trk[i].otherdb);
		}
	}

	return (cdibuf);
} /* print_cdinfo() */

/*
 * Open the rcfile for reading or writing.
 *
 *	name		Filename
 *	mode		"r" or "w"
 */
FILE *
open_rcfile(char *name, char *mode)
{
	FILE		*fp;
	struct stat	st;

	fp = fopen(name, mode);
	if (fp == NULL)
	{
		if (errno != ENOENT || mode[0] == 'w')
			perror(name);
	}
	else
	{
		/* Don't let people open directories or devices */
		if (fstat(fileno(fp), &st) < 0)
		{
			perror(name);
			fclose(fp);
			return (NULL);
		}

#ifdef S_ISREG
		if (! S_ISREG(st.st_mode))
#else
		if ((st.st_mode & S_IFMT) != S_IFREG)
#endif
		{
			errno = EISDIR;
			perror(name);
			fclose(fp);
			return (NULL);
		}

		if (mode[0] == 'w') /* create -- put data in so locks work */
		{
			fputs("# WorkMan database file\n", fp);
			fclose(fp);
			fp = fopen(name, "r+");
			if (fp == NULL)
				if (errno != ENOENT)
					perror(name);
		}
	}

	return (fp);
}

/*
 *
 * allocate and clear "trackmap".
 *
 */
int *reset_tracks(void)
{
    int i, j;
    int *trackmap;
    /*
     * Since we access track numbers indirectly (to handle sections
     * with at least a little elegance), the track mapping needs to be
     * set up before we read anything.  Initially it must assume that
     * no sections will appear in this datafile.
     */
    trackmap = malloc(sizeof(int) * cur_ntracks);
    if (trackmap == NULL)
    {
    	perror("trackmap");
	exit(1);
    }
    j = 0;
    for (i = 0; i < cd->ntracks; i++)
    {
	trackmap[i] = j;
	while (cd->trk[++j].section > 1)
		;
    }
    return trackmap;
} /* reset_tracks() */

/*
 * Load a new-format database file, searching for a match with the currently
 * inserted CD.  Modify the in-core copy of the CD info based on what's found
 * in the database.
 *
 * Returns 1 if there was a match or 0 if not.
 *
 *	fp		FILE* of database or rcfile.
 *	prefs		1 if we're searching .workmanrc, 0 for .workmandb,
 *			2 if just reading settings
 *	scan		Scan for "tracks" location and entry size only
 *	holesize_wanted	How big a hole we're looking for, if any.
 *
 * If a hole was found along the way, update the global "holepos" with its
 * starting offset in the file.  A hole is defined as a bunch of blank lines
 * preceding a "tracks" line.  Holepos will contain the best match.
 *
 * In addition, "firstpos" will be filled with the position of the first
 * "tracks" keyword, so we know how much room is available for global
 * settings at the rcfile's start.
 */
int
search_db( FILE *fp, int prefs, int scan, int holesize_wanted )

{
	char	keyword[64], listname[64], *c;
	int	b, i, j, track = 0, listsize, ntracks, scratch, searching = 1;
	int	*trackmap = 0, gotsections = 0;
        int     fudge, maxfudge, sizediff, bestfudge = 0;
	long	pos = 0, thisholepos = -1, holesize = 99991239;
	struct	wm_playlist *l;

	rclen = 0;

	wm_lib_message(WM_MSG_CLASS_DB|WM_MSG_LEVEL_DEBUG , "db: Reached search_db()\n" );

	/* We may not find any holes at all! */
	if (holesize_wanted)
		holepos = -1;

	if( prefs != 2 )
		trackmap = reset_tracks();

	if (prefs)
		freeup(&otherrc);
	firstpos = -1;
	while (! feof(fp))
	{
		pos = ftell(fp);
		keyword[0] = '\0';
		do
			b = getc(fp);
		while (b != EOF && b != '\n' && isspace(b));

		if (b == EOF || feof(fp))
			break;

		if (b != '\n')
		{
			keyword[0] = b;
			fscanf(fp, "%s", &keyword[1]);
		}
		if (keyword[0] == '\0')		/* Blank line. */
		{
			if (thisholepos < 0)
				thisholepos = pos;
			continue;
		}

		/* Strip off "s-" if we've seen a "sections" keyword */
		if (gotsections && keyword[0] == 's' && keyword[1] == '-')
		{
			for (c = &keyword[2]; (c[-2] = *c) != '\0'; c++)
				;
			wm_lib_message(WM_MSG_CLASS_DB|WM_MSG_LEVEL_DEBUG , "db: stripped off the 's-'. Result is %s\n", keyword);
		}

		/* If this is the start of a CD entry, see if it matches. */
		if (! strcmp(keyword, "tracks"))
		{
			if (prefs == 2)
				break;

			/* Is this the end of a hole? */
			if (holesize_wanted && (thisholepos >= 0))
			{
				/* Yep.  Is it better than the last one? */
				if (pos - thisholepos < holesize && pos -
						thisholepos >= holesize_wanted)
				{
					holepos = thisholepos;
					holesize = pos - thisholepos;
				}
				thisholepos = -1;
			}

			/* Is it the start of the CD entries? */
			if (firstpos == -1)
				firstpos = pos;

			/* Is this the end of the entry we really wanted? */
			if (! searching)
			{
				rclen = pos - rcpos;
				break;
			}

                        /* If we have a near match, indicate that we
                            should stop reading tracks, etc now */
                        if (searching == 2)
                        {
                            searching = 3;
                            continue;
                        }

			fscanf(fp, "%d", &ntracks);

			if (ntracks != cd->ntracks)
			{
chomp:
				SWALLOW_LINE(fp);
				continue;
			}

                        fudge = 0;
                        maxfudge = (ntracks * fuzz_frames) >> 1;
			track = 0;
			for (i = 0; i < ntracks; i++)
			{
				fscanf(fp, "%d", &scratch);
				if (scratch != cd->trk[track].start)
                                {
                                    sizediff = abs(scratch - cd->trk[track].start);
                                    if (sizediff > fuzz_frames || 
                                        (sizediff && scan))
                                      break;
                                    fudge += sizediff;
                                }
				while (cd->trk[++track].section > 1)
					;
			}
			if (i != ntracks)
				goto chomp;

                        if (fudge > 0) /* best near match? */
                        {
                            if (fudge > maxfudge)
                                goto chomp;
                            if (bestfudge == 0 || fudge < bestfudge)
                                bestfudge = fudge;
                            else
                                goto chomp;
			    rcpos = pos;
			    track = 0;
                            searching = 2;
                        }
                        else /* probably exact match */
                        {
			    fscanf(fp, "%d", &scratch);

			    if (scratch != -1 && scratch != cd->length)
				    goto chomp;

			    /* Found it! */
			    rcpos = pos;
			    track = 0;
			    searching = 0;
                        }

			SWALLOW_LINE(fp);	/* Get rid of newline */
		}

		/* Global mode stuff goes here */
		else if (! strcmp(keyword, "cddbprotocol"))
		{
			getc(fp);
			i = getc(fp);	/* only first letter is used */
		        cddb.protocol = i == 'c' ? 1 : 
		                        i == 'h' ? 2 : 3 ;
			do
				i = getc(fp);
			while (i != '\n' && i != EOF);
		}

		else if (! strcmp(keyword, "cddbserver"))
		{
			getc(fp);	/* lose the space */
			if (cddb.cddb_server[0])
				do
					i = getc(fp);
				while (i != '\n' && i != EOF);
			else
			{
				fgets(cddb.cddb_server, 
				      sizeof(cddb.cddb_server), fp);
				if ((i = strlen(cddb.cddb_server)))
					cddb.cddb_server[i - 1] = '\0';
			}
		}

		else if (! strcmp(keyword, "cddbmailadress"))
		{
			getc(fp);	/* lose the space */
			if (cddb.mail_adress[0])
				do
					i = getc(fp);
				while (i != '\n' && i != EOF);
			else
			{
				fgets(cddb.mail_adress, 
				      sizeof(cddb.mail_adress), fp);
				if ((i = strlen(cddb.mail_adress)))
					cddb.mail_adress[i - 1] = '\0';
			}
		}

		else if (! strcmp(keyword, "cddbpathtocgi"))
		{
			getc(fp);	/* lose the space */
			if (cddb.path_to_cgi[0])
				do
					i = getc(fp);
				while (i != '\n' && i != EOF);
			else
			{
				fgets(cddb.path_to_cgi, 
				      sizeof(cddb.path_to_cgi), fp);
				if ((i = strlen(cddb.path_to_cgi)))
					cddb.path_to_cgi[i - 1] = '\0';
			}
		}

		else if (! strcmp(keyword, "cddbproxy"))
		{
			getc(fp);	/* lose the space */
			if (cddb.proxy_server[0])
				do
					i = getc(fp);
				while (i != '\n' && i != EOF);
			else
			{
				fgets(cddb.proxy_server, 
				      sizeof(cddb.proxy_server), fp);
				if ((i = strlen(cddb.proxy_server)))
					cddb.proxy_server[i - 1] = '\0';
			}
		}

		else if (! strcmp(keyword, "whendone"))
		{
			getc(fp);
			i = getc(fp);	/* only first letter is used */
			if (cur_stopmode == -1)	/* user's setting preferred */
				cur_stopmode = i == 's' ? 0 : i == 'r' ? 1 : 2;
			do
				i = getc(fp);
			while (i != '\n' && i != EOF);
		}

		else if (! strcmp(keyword, "playnew"))
		{
			if (cur_playnew == -1)
				cur_playnew = 1;
			SWALLOW_LINE(fp);
		}

		/* If we're searching, skip to the next "tracks" line. */
		else if (((searching & 1)|| scan) 
                         && !(prefs && firstpos == -1))
			SWALLOW_LINE(fp)

		else if (! strcmp(keyword, "sections"))
		{
			gotsections = 1;
			fscanf(fp, "%d", &ntracks);

			free(trackmap);
			trackmap = (int *) malloc(sizeof(int) *
						(cur_ntracks + ntracks));
			if (trackmap == NULL)
			{
				perror("section mapping");
				exit(1);
			}

			/*
			 * If sections are already defined, use these as a
			 * reference, mapping this CD entry's section numbers
			 * to the ones in core.
			 *
			 * Otherwise, split the CD up according to the sections
			 * listed here.
			 */
			if (cur_nsections)
			{
				track = 0;
				i = 0;
				while (ntracks)
				{
					ntracks--;
					fscanf(fp, "%d", &scratch);
					while (scratch > cd->trk[track].start)
					{
						if (cd->trk[track].section < 2)
							trackmap[i++] = track;
						++track;

						if (track == cur_ntracks)
							break;
					}

					/* rc has later sections than db... */
					if (track == cur_ntracks)
						break;

					/* Matches can be approximate */
					if (scratch+75 > cd->trk[track].start &&
					    scratch-75 < cd->trk[track].start)
						trackmap[i++] = track++;
					else
						trackmap[i++] = -1;
					
					if (track == cur_ntracks)
						break;
				}

				/* This only happens if track == cur_ntracks */
				while (ntracks--)
					trackmap[i++] = -1;

				while (track < cur_ntracks)
				{
					if (cd->trk[track].section < 2)
						trackmap[i++] = track;
					track++;
				}

				track = 0;
				SWALLOW_LINE(fp);
			}
			else
			{
				while (ntracks--)
				{
					fscanf(fp, "%d", &scratch);
					split_trackinfo(scratch);
				}

				for (i = 0; i < cur_ntracks; i++)
				{
					trackmap[i] = i;
					/* split_trackinfo() sets this */
					cd->trk[i].contd = 0;
				}

				SWALLOW_LINE(fp);
			}
		}

		else if (! strcmp(keyword, "track"))
		{
			char buf[502];

			getc(fp);	/* lose the space */

			/* don't overwrite existing track names. */
			/* However, overwrite them if there was a "bad" fuzzy match before */
			if ((trackmap[track] == -1 || track > (cd->ntracks + cur_nsections)) && (searching == 2))
				SWALLOW_LINE(fp)
			else if (cd->trk[trackmap[track]].songname &&
					cd->trk[trackmap[track]].songname[0])
				do
					i = getc(fp);
				while (i != '\n' && i != EOF);
			else
			{
				fgets(buf, sizeof(buf), fp);
				wm_lib_message(WM_MSG_CLASS_DB|WM_MSG_LEVEL_DEBUG, "found track %s\n", buf);
				if( (i = strlen(buf)) )
					buf[i - 1] = '\0';
				wm_strmcpy(&cd->trk[trackmap[track]].songname, buf);
			}
			track++;
		}

		else if (! strcmp(keyword, "playmode"))
			fscanf(fp, "%d", &cd->playmode);

		else if (! strcmp(keyword, "autoplay"))
			cd->autoplay = 1;

		else if (! strcmp(keyword, "cdname"))
		{
                        /* because of fuzzy matching that may change
                           the disk contents, we reset everything when
                           we find the name, in hopes that we will recover
                           most, if not all, of the information from the
                           file. */
/*
 * nasty bug was here. Was it? BUGBUGBUG
 *
 *			 wipe_cdinfo(); 
 *			 trackmap = reset_tracks();
 */

			getc(fp);	/* lose the space */
			/* don't overwrite existing cd name. */
			if (cd->cdname[0] && (searching == 2))
				do
					i = getc(fp);
				while (i != '\n' && i != EOF);
			else
			{
                                if (searching > 1)
                                {
                                    strcpy(cd->cdname, "Probably://");
				    fgets(cd->cdname + strlen(cd->cdname), sizeof(cd->cdname), fp);
                                } 
                                else 
                                {
				    fgets(cd->cdname, sizeof(cd->cdname), fp);
                                } 
				if ( (i = strlen(cd->cdname)) )
					cd->cdname[i - 1] = '\0';
			}
		}

		else if (! strcmp(keyword, "artist"))
		{
			getc(fp);	/* lose the space */
			/* don't overwrite existing artist names. */
			if (cd->artist[0])
				do
					i = getc(fp);
				while (i != '\n' && i != EOF);
			else
			{
				fgets(cd->artist, sizeof(cd->artist), fp);
				if( (i = strlen(cd->artist)) )
					cd->artist[i - 1] = '\0';
			}
		}

		else if (! strcmp(keyword, "cdvolume"))
		{
			fscanf(fp, "%d", &cd->volume);
			cd->volume = (cd->volume * 100) / 32;
		}

		else if (! strcmp(keyword, "dontplay"))
		{
			fscanf(fp, "%d", &i);
			if (trackmap[i - 1] != -1)
				cd->trk[trackmap[i - 1]].avoid = 1;
		}

		else if (! strcmp(keyword, "continue"))
		{
			if (trackmap[track - 1] != -1)
				cd->trk[trackmap[track - 1]].contd = 1;
		}

		else if (! strcmp(keyword, "volume"))
		{
			fscanf(fp, "%d", &i);
			if (trackmap[i - 1] == -1)
				SWALLOW_LINE(fp)
			else
			{
				i = trackmap[i - 1];
				fscanf(fp, "%d", &cd->trk[i].volume);
				cd->trk[i].volume = (cd->trk[i].volume*100)/32;
				if (cd->trk[i].volume > 32)
					cd->trk[i].volume = 0;
			}
		}

		else if (! strcmp(keyword, "playlist"))
		{
			getc(fp);
			fscanf(fp, "%s", listname);

/* XXX take this out at some point */
			if (! strcmp(listname, "Default"))
				strcpy(listname, "List A");

			for (i = 0; listname[i]; i++)
				if (listname[i] == '_')
					listname[i] = ' ';

			l = new_list(cd, listname);
			if (l == NULL)
			{
plnomem:
				perror("playlist read");
				exit(1);
			}

			fscanf(fp, "%d", &listsize);

			l->list = malloc(sizeof(int) * (listsize + 1));
			if (l->list == NULL)
				goto plnomem;

			/* Leave out tracks that weren't in .workmandb. */
			j = 0;
			for (i = 0; i < listsize; i++)
			{
				fscanf(fp, "%d", &scratch);
				scratch = trackmap[scratch - 1];
				if (scratch != -1)
					l->list[j++] = scratch + 1;
			}

			l->list[j] = 0;
		}

		else if (! strcmp(keyword, "mark"))
		{
			int mark_val = -1, mark_namelen;
			char mark_name[32];

			fscanf(fp, "%d", &mark_val);
			if (mark_val == -1)
				goto chomp;

			if (getc(fp) != ' ')
				continue;

			fgets(mark_name, sizeof(mark_name), fp);
			if( ( mark_namelen = strlen(mark_name)) )
				mark_name[mark_namelen - 1] = '\0';

/*
			if (! strcmp(mark_name, "START"))
				set_abtimer(0, mark_val);
			else if (! strcmp(mark_name, "END"))
				set_abtimer(1, mark_val);

*/
		}

		/* Unrecognized keyword.  Put it in the right place. */
		else
		{
			char	**buf, input[BUFSIZ];

			if (track && trackmap[track - 1] == -1)
			{
				SWALLOW_LINE(fp);
				continue;
			}

			i = track ? trackmap[track - 1] : 0;
			buf = prefs ? i ? &cd->trk[i].otherrc : &cd->otherrc :
				i ? &cd->trk[i].otherdb : &cd->otherdb;
			if (firstpos == -1) {
				if (prefs) {
					buf = &otherrc;
				} else {
					goto chomp;
				} /* if() else */
                        } /* if() */
			wm_strmcat(buf, keyword);
			do {
				input[sizeof(input) - 1] = 'x';
				fgets(input, sizeof(input), fp);
				wm_strmcat(buf, input);
			} while (input[sizeof(input) - 1] != 'x');
		}
	}

	if (rclen == 0 && !searching)
		rclen = pos - rcpos;

        if (searching > 1) /* A near match has been found. Good enough. */
            searching = 0;

        cddb_struct2cur();
	return (! searching);

} /* search_db() */

/*
 * Delay some amount of time without using interval timers.
 */
void
spinwheels(int secs) {
	struct timeval	tv;

	tv.tv_usec = 0;
	tv.tv_sec = secs;
	select(0, NULL, NULL, NULL, &tv);
} /* spinwheels() */

/*
 * lockit(fd, type)
 *
 * fd    file descriptor
 * type  lock type
 *
 * Lock a file.  Time out after a little while if we can't get a lock;
 * this usually means the locking system is broken.
 *
 * Unfortunately, if there are lots of people contending for a lock,
 * this can result in the file not getting locked when it probably should.
 */
int
lockit(int fd, int type)
{
	struct flock	fl;
	int		result, timer = 0;

	if (suppress_locking)
		return (0);

	fl.l_type = type;
	fl.l_whence = 0;
	fl.l_start = 0;
	fl.l_len = 0;

	while ((result = fcntl(fd, F_SETLK, &fl)) < 0)
	{
		if (errno != EACCES || errno != EAGAIN)
			break;
		if (timer++ == 30)
		{
			errno = ETIMEDOUT;
			break;
		}

		spinwheels(1);
	}

	return (result);
} /* lockit() */

/*
 * Search all the database files and our personal preference file for
 * more information about the current CD.
 */
void
load( void )
{
	FILE		*fp;
	char		**dbfile;
	int		locked = 0;
	int		dbfound = 0, *trklist, i;
	unsigned long	dbpos;

/* This is some kind of profiling code. I don't change it
   to wm_lib_message() for now... */
#ifdef DEBUG
	long		t1, t2;
	if( getenv( "WORKMAN_DEBUG" ) != NULL )
	  {
		time(&t1);
		printf("%s (%d): search start = %ld\n", __FILE__, __LINE__, t1);
		fflush(stdout);
	  }	
#endif

	dbfile = databases;

	found_in_db = 0;

	/* Turn the cd->trk array into a simple array of ints. */
	trklist = (int *)malloc(sizeof(int) * cd->ntracks);
	for (i = 0; i < cd->ntracks; i++)
		trklist[i] = cd->trk[i].start;

	do {
		if (*dbfile && idx_find_entry(*dbfile, cd->ntracks, trklist,
				cd->length * 75, 0, &dbpos) == 0)
			dbfound = 1;

		fp = *dbfile ? open_rcfile(*dbfile, "r") : NULL;
		if (fp != NULL)
		{
			if (lockit(fileno(fp), F_RDLCK))
				perror("Couldn't get read (db) lock");
			else
				locked = 1;

			if (dbfound)
				fseek(fp, dbpos, 0);

			if (search_db(fp, 0, 0, 0))
			{
				found_in_db = 1;
				cd->whichdb = *dbfile;
			}

			if (locked && lockit(fileno(fp), F_UNLCK))
				perror("Couldn't relinquish (db) lock");

			fclose(fp);
		}
	} while (*++dbfile != NULL && cd->whichdb == NULL);

#ifdef DEBUG
	if( getenv( "WORKMAN_DEBUG" ) != NULL )
	  {
		time(&t2);
		printf("%s (%d): db search end = %ld, elapsed = %ld\n", __FILE__, __LINE__, t2, t2 - t1);
		fflush(stdout);
	  }	
#endif

	fp = rcfile ? open_rcfile(rcfile, "r") : NULL;
	if (fp != NULL)
	{
		locked = 0;
		if (lockit(fileno(fp), F_RDLCK))
			perror("Couldn't get read (rc) lock");
		else
			locked = 1;

		rcpos = 0;
		found_in_rc = search_db(fp, 1, 0, 0);
		if (! found_in_rc)
			cd->autoplay = wm_db_get_playnew();

		if (locked && lockit(fileno(fp), F_UNLCK))
			perror("Couldn't relinquish (rc) lock");

		fclose(fp);
	}

	free(trklist);

	if (cur_playnew == -1)
		cur_playnew = 0;

#ifdef DEBUG
	if( getenv( "WORKMAN_DEBUG" ) != NULL )
	  {
		time(&t2);
		printf("%s (%d): search end = %ld, elapsed = %ld\n", __FILE__, __LINE__, t2, t2 - t1);
		fflush(stdout);
	  }	
#endif
} /* load() */

/*
 * Load program settings from the rcfile.
 */
void
load_settings( void )
{
	FILE    *fp;
	int     locked;

	fp = rcfile ? open_rcfile(rcfile, "r") : NULL;
	if (fp != NULL)
	{
		locked = 0;
		if (lockit(fileno(fp), F_RDLCK))
			perror("Couldn't get read (rc) lock");
		else
			locked = 1;

		rcpos = 0;
		found_in_rc = search_db(fp, 2, 0, 0);
		if (! found_in_rc)
			cd->autoplay = wm_db_get_playnew();

		if (locked && lockit(fileno(fp), F_UNLCK))
			perror("Couldn't relinquish (rc) lock");

		fclose(fp);
	}
} /* load_settings() */

/*
 * save_globals()
 *
 * Save the global preferences, scooting CD entries to the end if needed.
 * The assumption here is that the rcfile is locked, and that firstpos has
 * been set by a previous scan.
 */
void
save_globals(FILE *fp)
{
	char	*globes = NULL, *cdentry = NULL, temp[100];
	long	curpos;
	int	globesize, hit_cdent = 0, c = 0;

	if (otherrc)
		wm_strmcpy(&globes, otherrc);

	if (cddb.protocol)
	{
		sprintf(temp, "cddbprotocol ");
		switch(cddb.protocol)
		{
		 case 1: /* cddbp */
		    sprintf(temp + strlen(temp), "cddbp\n");
		    break;
		 case 2: /* http */
		    sprintf(temp + strlen(temp), "http\n");
		    break;
		 case 3: /* proxy */
		    sprintf(temp + strlen(temp), "proxy\n");
		    break;
		 default:
		    break;
		}
		wm_strmcat(&globes, temp);
	    
		if(cddb.mail_adress[0])
	 	{
			sprintf(temp,"cddbmailadress %s\n",
				cddb.mail_adress);
			wm_strmcat(&globes, temp);
		}

		if(cddb.cddb_server[0])
	 	{
			sprintf(temp,"cddbserver %s\n",
				cddb.cddb_server);
			wm_strmcat(&globes, temp);
		}

		if(cddb.path_to_cgi[0])
	 	{
			sprintf(temp,"cddbpathtocgi %s\n",
				cddb.mail_adress);
			wm_strmcat(&globes, temp);
		}

		if(cddb.proxy_server[0])
	 	{
			sprintf(temp,"cddbproxy %s\n",
				cddb.mail_adress);
			wm_strmcat(&globes, temp);
		}
	}

	if (cur_stopmode == 1 || cur_stopmode == 2)
	{
		sprintf(temp, "whendone %s\n", cur_stopmode == 1 ? "repeat" :
			"eject");
		wm_strmcat(&globes, temp);
	}

	if (cur_playnew == 1)
		wm_strmcat(&globes, "playnew\n");

	curpos = firstpos;
	if (curpos < 0)
		curpos = 0;

	fseek(fp, curpos, SEEK_SET);

	if (firstpos < (globesize = globes != NULL ? strlen(globes) : 0))
	{
		while (1)
		{
			temp[sizeof(temp)-1] = 'x';

			if (fgets(temp, sizeof(temp), fp) == NULL)
			{
				fseek(fp, 0, SEEK_SET);
				if (globes != NULL)
				{
					fwrite(globes, globesize, 1, fp);
					free(globes);
				}
				if (cdentry != NULL)
				{
					fwrite(cdentry, strlen(cdentry), 1, fp);
					free(cdentry);
				}
				return;
			}

			if (! strncmp(temp, "tracks ", 7))
			{
				hit_cdent = 1;
				if (curpos >= globesize)
					break;
			}

			if (! hit_cdent)
			{
				curpos += strlen(temp);
				if (temp[sizeof(temp)-1] == '\0')
					while ((c = getc(fp)) != '\n' &&
								c != EOF)
						curpos++;
				if (c == '\n')
					curpos++;

				continue;
			}

			wm_strmcat(&cdentry, temp);
			curpos += strlen(temp);
			while (temp[sizeof(temp)-1] == '\0')
			{
				temp[sizeof(temp)-1] = 'x';
				if (fgets(temp, sizeof(temp), fp) == NULL)
					break;
				wm_strmcat(&cdentry, temp);
				curpos += strlen(temp);
			}
		} 

		if (cdentry != NULL)
		{
			fseek(fp, 0, SEEK_END);
			fwrite(cdentry, strlen(cdentry), 1, fp);
			free(cdentry);
		}
	}

	if (globes != NULL)
	{
		fseek(fp, 0, SEEK_SET);
		fwrite(globes, globesize, 1, fp);
		free(globes);
	}

	while (globesize++ < curpos)
		putc('\n', fp);
} /* save_globals() */

/*
 * save_entry()
 *
 * Save the CD information to one database.
 *
 *	filename	Database to save to.
 *	pref		0 for hard data, 1 for preferences.
 *
 * If an entry for this CD exists already, overwrite it with the new entry
 * if the new entry is the same size or smaller, or with newlines if the new
 * entry is larger (in which case the new entry is appended to the file.)
 *
 * Also, if the preference information is being updated, save it to the
 * file while we've got it locked.  Scoot stuff from the beginning of
 * the file to the end as needed to facilitate this.
 *
 * XXX Preference-saving should probably be done elsewhere, like in an
 * Apply button on the Goodies popup, and in any case needs to go to a
 * different file (.Xdefaults?)
 *
 * Returns 0 on success.
 */
int
save_entry(char *filename, int pref)
{
	FILE		*fp;
	char		*buf;
	int		len, i, locked = 0;


	if( filename == NULL )
		return (-1);

	fp = open_rcfile(filename, "r+");
	if (fp == NULL)
	{
		if (errno == ENOENT)	/* doesn't exist already */
			fp = open_rcfile(filename, "w");
		if (fp == NULL)
			return (-1);
	}

	if (lockit(fileno(fp), F_WRLCK))
		perror("Warning: Couldn't get write lock");
	else
		locked = 1;

	buf = print_cdinfo(cd, pref);
	len = strlen(buf);	/* doesn't return if there's an error */

	rcpos = -1;
	search_db(fp, pref, 1, len);
	if (rcpos != -1)		/* XXX */
	{
		/*
		 * Jump to the entry's position in the database file, if
		 * it was found.
		 */
		fseek(fp, rcpos, SEEK_SET);

		if (rclen >= len && holepos == -1)
		{
			/*
			 * If the new entry will fit in the space occupied by
			 * the old one, overwrite the old one and make a hole
			 * of the appropriate size at its end.
			 *
			 * No need to update the index file in this case, as
			 * the entry's position hasn't changed.
			 */
			fputs(buf, fp);
			for (i = len; i < rclen; i++)
				fputc('\n', fp);
		}
		else
		{
			/*
			 * Overwrite the old entry with a hole and delete
			 * its pointer in the index file.
			 */
			for (i = 0; i < rclen; i++)
				fputc('\n', fp);
			idx_delete_entry(filename, cd->trk[cd->ntracks-1].start,
					0, rcpos);

			rcpos = -1;
		}
	}

	/*
	 * Old entry wasn't found, or its new version wouldn't fit where
	 * the old one was.
	 */
	if (rcpos == -1)
	{
		/*
		 * Write the new entry in a hole, if there is one,
		 * or at the end of the file.
		 */
		if (holepos >= 0)
		{
			fseek(fp, holepos, SEEK_SET);
			if (holepos < firstpos)
				firstpos = holepos;
		}
		else
		{
			fseek(fp, 0, SEEK_END);
			holepos = ftell(fp);
		}
		fputs(buf, fp);

		/*
		 * Write a new index entry for this CD.
		 */
		idx_write_entry(filename, cd->trk[cd->ntracks - 1].start,
			holepos);
	}

	if (pref)
		save_globals(fp);

	fflush(fp);

	if (locked && lockit(fileno(fp), F_UNLCK))
		perror("Warning: Couldn't relinquish write lock");

	fclose(fp);

	return (0);
} /* save_entry() */

/*
 * save()
 *
 * Save CD information to the appropriate datafile (the first file in the
 * list, unless the entry came from another database file) and to the
 * personal prefs file.
 */
int
save( void )
{

	if( wm_db_save_disabled == FALSE )
	{
		if (save_entry(rcfile, 1))
			return (0);

		if (cd->whichdb == NULL || access(cd->whichdb, W_OK))
			cd->whichdb = databases[0];

		if (save_entry(cd->whichdb, 0))
			return (0);

		return( WM_DB_SAVE_ERROR );
	} else {
		return( WM_DB_SAVE_DISABLED );
	}
} /* save() */
