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
 * Build a WorkMan database index file from a flat text file.  Requires
 * 4.4BSD libdb library.
 */

static char buildindex_id[] = "$Id$";
 
#include <stdio.h>
//#include <db.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>		/* for htonl() */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

char *strrchr();

main(argc, argv)
	int	argc;
	char	**argv;
{
	DB	*db;
	DBT	key, data;
	FILE	*fp;
	int	lock = 1, i = 0, locked, frame;
	char	buf[1000], indname[MAXPATHLEN + 100], framebuf[8], *c;
	unsigned long	pos;
	BTREEINFO	bt;
	struct stat	st;

	if (argc > 2 && !strcmp(argv[1], "-n"))
	{
		lock = 0;
		i++;
	}

	if (argc < i + 2)
	{
		fprintf(stderr, "Usage: %s [-n] dbfile [dbfile ...]\n",
			argv[0]);
		exit(1);
	}

	data.data = &pos;
	data.size = sizeof(pos);
	key.data = framebuf;
	key.size = 7;		/* %07d */

	while (++i < argc)
	{
		fprintf(stderr, "Building index for %s\n", argv[i]);

		if ((fp = fopen(argv[i], "r")) == NULL)
		{
			perror(argv[i]);
			continue;
		}

		/*
		 * Figure out the file's mode, uid, gid, so we can set the
		 * permissions on the index file to the same thing.
		 */
		if (fstat(fileno(fp), &st))
		{
			sprintf(indname, "%s: fstat", argv[i]);
			perror(indname);
			fclose(fp);
			continue;
		}

		if (lock && lockit(fileno(fp), F_WRLCK))
		{
			sprintf(indname, "%s: Warning: Couldn't lock", argv[i]);
			perror(indname);
			locked = 0;
		}
		else
			locked = 1;

		/*
		 * Create a database file.
		 */
		bt.flags = R_DUP;	/* allow duplicate keys */
		bt.cachesize = 0;
		bt.psize = 0;
		bt.lorder = 4321;
		bt.minkeypage = 0;
		bt.compare = NULL;	/* use lexical comparison */
		bt.prefix = NULL;	/* no prefix comparisons */

		/* Index files have ".ind" extensions */
		sprintf(indname, "%s.ind", argv[i]);
		if ((db = dbopen(indname, O_CREAT | O_RDWR | O_TRUNC,
			st.st_mode, DB_BTREE, &bt)) == NULL)
		{
			perror(indname);
			if (locked)
				lockit(fileno(fp), F_UNLCK);
			fclose(fp);
			continue;
		}

		/*
		 * Now loop through the text file, inserting a record into
		 * the index file for each "tracks" line.
		 */
		while (! feof(fp))
		{
			pos = ftell(fp);
			buf[0] = '\0';
			if (fgets(buf, sizeof(buf), fp) == NULL || ! buf[0])
			{
				/* End of file? */
				if (feof(fp))
					break;
				
				/* Nope.  A read error.  Unlink the database. */
				perror(argv[i]);
				(void) unlink(indname);
				break;
			}

			if (strncmp(buf, "tracks ", 7))
				continue;
			
			/*
			 * Found the start of a record.  Figure out the start
			 * time of the last track and put an entry in the
			 * index file with that as the key.
			 */
			c = strrchr(buf, ' ');	/* this will always succeed */
			*c = '\0';
			c = strrchr(buf, ' ');	/* this should too, but... */
			if (c == NULL)
			{
				fprintf(stderr,
					"%s: Malformed tracks line at %lu\n",
					argv[i], pos);
				continue;
			}
			sscanf(c+1, "%d", &frame);
			sprintf(framebuf, "%07d", frame);
			pos = htonl(pos);

			if ((db->put)(db, &key, &data, 0))
			{
				perror(indname);
				unlink(indname);
				break;
			}
		}

		/*
		 * Clean up.
		 */
		(void) (db->close)(db);
		if (locked)
			lockit(fileno(fp), F_UNLCK);
	}
}

/*
 * Lock a file.  Time out after a little while if we can't get a lock;
 * this usually means the locking system is broken.
 *
 * Unfortunately, if there are lots of people contending for a lock,
 * this can result in the file not getting locked when it probably should.
 */
int
lockit(fd, type)
	int	fd;
	int	type;
{
	struct flock	fl;
	int		result, timer = 0;

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

		sleep(1);
	}

	return (result);
}
