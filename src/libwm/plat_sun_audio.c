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
 * Sun (really Solaris) digital audio functions.
 */

#include "include/wm_config.h"

static char plat_sun_audio_id[] = "$Id$";

#if defined(sun) || defined(__sun) && defined(SYSV) && defined(BUILD_CDDA) && defined(WMCDDA_DONE)


#include "include/wm_cdda.h"

/* types.h included by wmcdda.h */

#include <stdio.h>
#include <malloc.h>
#include <sys/ioctl.h>
#include <sys/audioio.h>
#include <sys/stropts.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#define WM_MSG_CLASS WM_MSG_CLASS_PLATFORM

/*
 * Since there's a lag time between writing audio to the audio device and
 * hearing it, we need to make sure the status indicators correlate to what's
 * playing out the speaker.  Luckily, Solaris gives us some audio
 * synchronization facilities that make this pretty easy.
 *
 * We maintain a circular queue of status information.  When we write some
 * sound to the audio device, we put its status info into the queue.  We write
 * a marker into the audio stream; when the audio device driver encounters the
 * marker, it increments a field in a status structure.  When we see that
 * field go up, we grab the next status structure from the queue and send it
 * to the parent process.
 *
 * The minimum size of the queue depends on the latency of the audio stream.
 */
#define QSIZE 500

struct cdda_block	queue[QSIZE];
int			qtail;
int			qstart;

/*
 * We only send WMCDDA_PLAYED status messages upstream when the CD is supposed
 * to be playing; this is used to keep track.
 */
extern int playing;

static int	aufd, aucfd;
static int	raw_audio = 1;	/* Can /dev/audio take 44.1KHz stereo? */

/* 
 * For fast linear-to-ulaw mapping, we use a lookup table that's generated
 * at startup.
 */
unsigned char *ulawmap, linear_to_ulaw();

char *getenv();

/*
 * Dummy signal handler so writes to /dev/audio will interrupt.
 */
static void
dummy( void )
{
	signal(SIGALRM, dummy);
}

/*
 * Initialize the audio device.
 */
void
wmaudio_init( void )
{
	audio_info_t		info;
	char			*audiodev, *acdev;
	int			linval;

	audiodev = getenv("AUDIODEV");
	if (audiodev == NULL || 
	    strncmp("/dev/", audiodev, 5) || 
	    strstr(audiodev, "/../") )
		audiodev = "/dev/audio";

	acdev = malloc(strlen(audiodev) + 4);
	if (acdev == NULL)
	{
		perror("Can't allocate audio control filename");
		exit(1);
	}
	strcpy(acdev, audiodev);
	strcat(acdev, "ctl");

	aucfd = open(acdev, O_WRONLY, 0);
	if (aucfd < 0)
	{
		perror(acdev);
		exit(1);
	}
	free(acdev);

	aufd = open(audiodev, O_WRONLY, 0);
	if (aufd < 0)
	{
		perror(audiodev);
		exit(1);
	}

	signal(SIGALRM, dummy);

	/*
	 * Try to set the device to CD-style audio; we can process it
	 * with the least CPU overhead.
	 */
	AUDIO_INITINFO(&info);
	info.play.sample_rate = 44100;
	info.play.channels = 2;
	info.play.precision = 16;
	info.play.encoding = AUDIO_ENCODING_LINEAR;
	info.play.pause = 0;
	info.record.pause = 0;
	info.monitor_gain = 0;

	if (ioctl(aufd, AUDIO_SETINFO, &info) < 0)
		if (errno == EINVAL)
		{
			/*
			 * Oh well, so much for that idea.
			 */
			AUDIO_INITINFO(&info);
			info.play.sample_rate = 8000;
			info.play.channels = 1;
			info.play.precision = 8;
			info.play.encoding = AUDIO_ENCODING_ULAW;
			info.play.pause = 0;
			info.record.pause = 0;
			info.monitor_gain = 0;
			if (ioctl(aufd, AUDIO_SETINFO, &info) < 0)
			{
				perror("Can't set up audio device");
				exit(1);
			}

			/*
			 * Initialize the linear-to-ulaw mapping table.
			 */
			if (ulawmap == NULL)
				ulawmap = malloc(65536);
			if (ulawmap == NULL)
			{
				perror("malloc");
				exit(1);
			}
			for (linval = 0; linval < 65536; linval++)
				ulawmap[linval] = linear_to_ulaw(linval-32768);
			ulawmap += 32768;
			raw_audio = 0;
		}
		else
		{
			perror(audiodev);
			exit(1);
		}
}

/*
 * Get ready to play some sound.
 */
void
wmaudio_ready( void )
{
	audio_info_t		info;

	/*
	 * Start at the correct queue position.
	 */
	if (ioctl(aucfd, AUDIO_GETINFO, &info) < 0) perror("AUDIO_GETINFO");
	qtail = info.play.eof % QSIZE;
	qstart = qtail;

	queue[qtail].status = WMCDDA_OK;
}

/*
 * Stop the audio immediately.
 */
void
wmaudio_stop( void )
{
	if (ioctl(aufd, I_FLUSH, FLUSHRW) < 0)
		perror("flush");
}

/*
 * Close the audio device.
 */
void
wmaudio_close( void )
{
	wmaudio_stop();
	close(aufd);
	close(aucfd);
}

/*
 * Set the volume level.
 */
void
wmaudio_volume(int level)
{
	audio_info_t		info;

	AUDIO_INITINFO(&info);
	if (ioctl(aucfd, AUDIO_GETINFO, &info) < 0) perror("AUDIO_GETINFO");
	info.play.gain = level;
	if (ioctl(aucfd, AUDIO_SETINFO, &info) < 0) perror("AUDIO_SETINFO");
}

/*
 * Set the balance level.
 */
void
wmaudio_balance(int level)
{
	audio_info_t		info;

	AUDIO_INITINFO(&info);
	if (ioctl(aucfd, AUDIO_GETINFO, &info) < 0) perror("AUDIO_GETINFO");
	level *= AUDIO_RIGHT_BALANCE;
	info.play.balance = level / 255;
	if (ioctl(aucfd, AUDIO_SETINFO, &info) < 0) perror("AUDIO_SETINFO");
}

/*
 * Mark the most recent audio block on the queue as the last one.
 */
void
wmaudio_mark_last( void )
{
	queue[qtail].status = WMCDDA_DONE;
}

/*
 * Figure out the most recent status information and send it upstream.
 */
int
wmaudio_send_status( void )
{
	audio_info_t		info;
	int			qhead;

	/*
	 * Now send the most current status information to our parent.
	 */
	if (ioctl(aucfd, AUDIO_GETINFO, &info) < 0)
		perror("AUDIO_GETINFO");
	qhead = info.play.eof % QSIZE;

	if (qhead != qstart && playing)
	{
		int	balance;

		if (queue[qhead].status != WMCDDA_DONE)
			queue[qhead].status = WMCDDA_PLAYED;
		queue[qhead].volume = info.play.gain;
		queue[qhead].balance = (info.play.balance * 255) /
					AUDIO_RIGHT_BALANCE;

		send_status(queue + qhead);
		qstart = -1;
	}

	return (queue[qhead].status == WMCDDA_DONE);
}

/*
 * Play some audio and pass a status message upstream, if applicable.
 * Returns 0 on success.
 */
int
wmaudio_play(unsigned char *rawbuf, long buflen, struct cdda_block *blk)
{
	int			i;
	short			*buf16;
	int			alarmcount = 0;
	struct itimerval	it;

	alarm(1);

	while (write(aufd, rawbuf, buflen) <= 0)
		if (errno == EINTR)
		{
			if (! raw_audio && alarmcount++ < 5)
			{
				/*
				 * 8KHz /dev/audio blocks for several seconds
				 * waiting for its queue to drop below a low
				 * water mark.
				 */
				wmaudio_send_status();
				timerclear(&it.it_interval);
				timerclear(&it.it_value);
				it.it_value.tv_usec = 500000;
				setitimer(ITIMER_REAL, &it, NULL);
				continue;
			}

/*			close(aufd);
			close(aucfd);
			wmaudio_init();
*/ wmaudio_stop();
			alarm(2);
			continue;
		}
		else
		{
			blk->status = WMCDDA_ERROR;
			return (-1);
		}
	alarm(0);

	/*
	 * Mark this spot in the audio stream.
	 *
	 * Marks don't always succeed (if the audio buffer is empty
	 * this call will block forever) so do it asynchronously.
	 */
	fcntl(aufd, F_SETFL, O_NONBLOCK);
	if (write(aufd, rawbuf, 0) < 0)
	{
		if (errno != EAGAIN)
			perror("audio mark");
	}
	else
		qtail = (qtail + 1) % QSIZE;

	fcntl(aufd, F_SETFL, 0);

	queue[qtail] = *blk;

	if (wmaudio_send_status() < 0)
		return (-1);
	else
		return (0);
}

/*
 * Get the current audio state.
 */
void
wmaudio_state(struct cdda_block *blk)
{
	audio_info_t		info;
	int			balance;

	if (ioctl(aucfd, AUDIO_GETINFO, &info) < 0)
		perror("AUDIO_GETINFO");
	blk->volume = info.play.gain;
	blk->balance = (info.play.balance * 255) / AUDIO_RIGHT_BALANCE;
}

/*
** This routine converts from linear to ulaw.
**
** Craig Reese: IDA/Supercomputing Research Center
** Joe Campbell: Department of Defense
** 29 September 1989
**
** References:
** 1) CCITT Recommendation G.711  (very difficult to follow)
** 2) "A New Digital Technique for Implementation of Any
**     Continuous PCM Companding Law," Villeret, Michel,
**     et al. 1973 IEEE Int. Conf. on Communications, Vol 1,
**     1973, pg. 11.12-11.17
** 3) MIL-STD-188-113,"Interoperability and Performance Standards
**     for Analog-to_Digital Conversion Techniques,"
**     17 February 1987
**
** Input: Signed 16 bit linear sample
** Output: 8 bit ulaw sample
*/
#define ZEROTRAP    /* turn on the trap as per the MIL-STD */
#define BIAS 0x84               /* define the add-in bias for 16 bit samples */
#define CLIP 32635
 
unsigned char
linear_to_ulaw( sample )
int sample;
{
	static int exp_lut[256] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
				   4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
				   5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
				   5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
				   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
				   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
				   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
				   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
				   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
				   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
				   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
				   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
				   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
				   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
				   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
				   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};
	int sign, exponent, mantissa;
	unsigned char ulawbyte;
 
	/* Get the sample into sign-magnitude. */
	sign = (sample >> 8) & 0x80;            /* set aside the sign */
	if ( sign != 0 ) sample = -sample;              /* get magnitude */
	if ( sample > CLIP ) sample = CLIP;             /* clip the magnitude */
 
	/* Convert from 16 bit linear to ulaw. */
	sample = sample + BIAS;
	exponent = exp_lut[( sample >> 7 ) & 0xFF];
	mantissa = ( sample >> ( exponent + 3 ) ) & 0x0F;
	ulawbyte = ~ ( sign | ( exponent << 4 ) | mantissa );
#ifdef ZEROTRAP
	if ( ulawbyte == 0 ) ulawbyte = 0x02;   /* optional CCITT trap */
#endif
 
	return ulawbyte;
}

/*
 * Downsample a block of CDDA data, if necessary, for playing out an old-style
 * audio device.
 */
long
wmaudio_convert(unsigned char *rawbuf, long buflen, struct cdda_block *blk)
{
	short		*buf16 = (short *)rawbuf;
	int		i, j, samples;
	int		mono_value;
	unsigned char	*rbend = rawbuf + buflen;

	/* Don't do anything if the audio device can take the raw values. */
	if (raw_audio)
		return (buflen);

	for (i = 0; buf16 < (short *)(rbend); i++)
	{
		/* Downsampling to 8KHz is a little irregular. */
		samples = (i & 1) ? ((i % 20) ? 10 : 12) : 12;

		/* And unfortunately, we don't always end on a nice boundary. */
		if (buf16 + samples > (short *)(rbend))
			samples = ((short *)rbend) - buf16;

		/*
		 * No need to average all the values; taking the first one
		 * is sufficient and less CPU-intensive.  But we do need to
		 * do both channels.
		 */
		mono_value = (buf16[0] + buf16[1]) / 2;
		buf16 += samples;
		rawbuf[i] = ulawmap[mono_value];
	}

	return (i);
}

#endif /* ... && BUILD_CDDA */
