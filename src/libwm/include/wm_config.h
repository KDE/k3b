#ifndef WM_CONFIG_H
#define WM_CONFIG_H
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
 **********************************************************************
 *
 * This file consists of several parts. First, there's a generic,
 * platform independent part. Set needed options there.
 * The following parts are platform dependent. You may search for the
 * names listed below and then set your OS specific options there.
 * Don't be surprised, if there are no options for your OS. They aren't
 * needed in any case.
 * 
 * The default values should produce a functional WorkMan on every
 * platform.
 *
 *********************
 * Current platforms:
 *********************
 * BSD386
 * FreeBSD
 * HP-UX
 * Irix (SGI)
 * Linux
 * News  (Sony NewsOS)
 * OpenBSD
 * OSF1
 * Sun (SunOS/Solaris, Sparc or x86)
 * SVR4
 * Ultrix
 * AIX
 *
 * The above order corresponds with the order of the platform specific
 * options below.
 */

/******************************************************************
 * generic options
 ******************************************************************
 **   **  ***  ****    **  ****   **    *    **   **    **   *   **
 *  *  *  ***  ****  *  *  ***  *  **  **  ***  *  *  *  *  * *  **
 *     *  ***  ****    **  ***     **  **   **  *  *    **  * *  **
 *  *  *  ***  ****  ****  ***  *  **  **  ***  *  *  *  *  ***  **
 *  *  *    *    **  ****    *  *  **  **  ****   **  *  *  ***  **
 ******************************************************************/

/*
 * This option is obvious. But please do not forget the original
 * WorkMan version string if you need support.
 */

#define WORKMAN_NAME "LibWorkMan"
#define WORKMAN_VERSION "1.4.0"
 
/*
 * If your CD-ROM drive closes its tray if the device is opened, then
 * the next define can make WorkMans "Eject" button an "open/close"
 * button. If it disturbs you, just comment it out.
 *
 * ### this is preliminary. It may have no effect for you ###
 */
#define CAN_CLOSE 1

/*
 * Define the following if you want the balance slider to
 * decrease one channel's volume while increasing the other's
 */
/* #define SYMETRIC_BALANCE 1 */


/*
 * Define this if you want CDDA support. Supported systems are:
 *
 * - Solaris (2.4 or newer)
 * --> Linux is on the way. Don't define it now. It would lead to errors only.
 */
/*#define BUILD_CDDA 1*/



/******************************************************************
 * BSD386
 ******************************************************************
 ***     ****    ***    *******  **    ****    ****    ************
 ***  **  **  ******  *  *****  ******  **  **  **  ***************
 ***    ******  ****  **  ***  *****  *****    ***     ************
 ***  **  ******  **  *  ***  ********  **  **  **  **  ***********
 ***     ****    ***    ***  ******    ****    ****    ************
 ******************************************************************/
#if defined(__bsdi__) || defined(__bsdi)

/*
 * This lets you use the SoundBlaster Mixer on BSD/386
 */
#define SOUNDBLASTER 1

#define DEFAULT_CD_DEVICE "/dev/rsr2c" 

#endif /* __bsdi__ (BSD/386) */

/******************************************************************
 * FreeBSD
 ******************************************************************
 ***      **     ***      **      **     ****    ***    ***********
 ***  ******  **  **  ******  ******  **  **  ******  *  **********
 ***    ****     ***    ****    ****    ******  ****  **  *********
 ***  ******  **  **  ******  ******  **  ******  **  *  **********
 ***  ******  **  **      **      **     ****    ***    ***********
 ******************************************************************/
#if defined(__FreeBSD__) || defined(__FreeBSD) || defined(__NetBSD__) || defined(__NetBSD)

#define DEFAULT_CD_DEVICE	"/dev/rmatcd0c"

#endif /* freebsd */

/******************************************************************
 * HP-UX
 ******************************************************************
 ***  **  **     *********  **  **  **  ***************************
 ***  **  **  **  ********  **  ***    ****************************
 ***      **     ***    **  **  ****  *****************************
 ***  **  **  ************  **  ***    ****************************
 ***  **  **  *************    ***  **  ***************************
 ******************************************************************/
#if defined(hpux) || defined (__hpux)

#define	DEFAULT_CD_DEVICE	"/dev/rscsi"

#endif /* hpux */

/******************************************************************
 * Irix
 ******************************************************************
 ***      **     ***      **  **  *********************************
 *****  ****  **  ****  *****    **********************************
 *****  ****     *****  ******  ***********************************
 *****  ****  **  ****  *****    **********************************
 ***      **  **  **      **  **  *********************************
 ******************************************************************/
#if defined(sgi) || defined(__sgi)

#define DEFAULT_CD_DEVICE	"/dev/scsi/sc0d6l0"

#endif /* sgi IRIX */

/******************************************************************
 * Linux
 ******************************************************************
 ***  ******      **   ***  **  **  **  **  ***********************
 ***  ********  ****    **  **  **  ***    ************************
 ***  ********  ****  *  *  **  **  ****  *************************
 ***  ********  ****  **    **  **  ***    ************************
 ***      **      **  ***   ***    ***  **  ***********************
 ******************************************************************/
#if defined(__linux__)

/*
 * Uncomment the following line to have WorkMan send SCSI commands
 *  directly to the CD-ROM drive.  If you have a SCSI drive you 
 * probably want this, but it will cause WorkMan to not work on IDE
 * drives.
 */
/*#define LINUX_SCSI_PASSTHROUGH 1*/

/*
 * Which device should be opened by WorkMan at default?
 */
#define DEFAULT_CD_DEVICE	"/dev/cdrom"

/*
 * Uncomment the following if you use the sbpcd or mcdx device driver.
 * It shouldn't hurt if you use it on other devices. It'll be nice to
 * hear from non-sbpcd (or mcdx) users if this is right.
 */
/*#define SBPCD_HACK 1*/

/*
 * Linux Soundcard support
 * Disabled by default, because some people rely on it
 */
/* #define OSS_SUPPORT 1 */

/*
 * This has nothing to do with the above.
 */

/* #define CURVED_VOLUME */
 
/*
 * Uncomment the following if you want to try out a better responding
 * WorkMan, especially with IDE drives. This may work with non-IDE
 * drives as well. But it may be possible, that it doesn't work at all.
 * If your drive/driver combination cannot handle the faster access,
 * the driver will usually hang and you have to reboot your machine.
 */
/* #define FAST_IDE 1 */

/*
 * There are two alternative ways of checking a device containing a
 * mounted filesystem. Define BSD_MOUNTTEST for the test using
 * getmntent(). Undefine it for using the SVR4 ustat().
 * I built in the choice, because it's not clear which method should
 * be used in Linux. The ustat manpage tells us since 1995, that
 * fstat() should be used, but I'm too dumb to do so.
 */

#define BSD_MOUNTTEST 

#endif /* __linux */

/******************************************************************
 * Sony NewsOS
 ******************************************************************
 ***   ***  **      **  *****  ***    *****************************
 ***    **  **  ******  *****  **  ********************************
 ***  *  *  **    ****  ** **  ****  ******************************
 ***  **    **  ******  *   *  ******  ****************************
 ***  ***   **      **    *    ***    *****************************
 ******************************************************************/
#if defined(__sony_news) || defined(sony_news)

#define	DEFAULT_CD_DEVICE	"/dev/rsd/b0i6u0p2\0"

#endif

/******************************************************************
 * OpenBSD
 ******************************************************************
 ****    ***     ***      **   ***  **     ****    ***    *********
 ***  **  **  **  **  ******    **  **  **  **  ******  *  ********
 ***  **  **     ***    ****  *  *  **    ******  ****  **  *******
 ***  **  **  ******  ******  **    **  **  ******  **  *  ********
 ****    ***  ******      **  ***   **     ****    ***    *********
 ******************************************************************/
#if defined(__OpenBSD__) || defined(__OpenBSD)

#define	DEFAULT_CD_DEVICE	"/dev/rcdrom"

#endif

/******************************************************************
 * OSF1
 ******************************************************************
 ****    ****    ***      ***  ***  *******************************
 ***  **  **  ******  ******  **    *******************************
 ***  **  ****  ****    ***  *****  *******************************
 ***  **  ******  **  ****  ******  *******************************
 ****    ****    ***  ***  *****      *****************************
 ******************************************************************/
#if defined(__osf__) || defined(__osf)

/* not needed here, it's figured out by plat_osf1.c */
/* #define DEFAULT_CD_DEVICE "/dev/rcdrom/cd0" */

#endif

/******************************************************************
 * SunOS/Solaris
 ******************************************************************
 ****    ***  **  **   ***  ***************************************
 ***  ******  **  **    **  ***************************************
 *****  ****  **  **  *  *  ***************************************
 *******  **  **  **  **    ***************************************
 ****    ****    ***  ***   ***************************************
 ******************************************************************/
#if defined(sun) || defined(__sun)

/*
 * Define the following for Solaris 2.x
 * If you don't want WorkMan to try to activate the SPARCstation 5
 * internal audio input so you get sound from the workstation, comment
 * out the CODEC define.
 */

#define SYSV 1
#define CODEC 1

/*
 * set the following to "SUNW,CS4231" for Sun and to "SUNW,sb16"
 * for PC (with SoundBlaster 16) running Solaris x86
 * (only important if you define CODEC above)
 */
#define SUN_AUD_DEV "SUNW,CS4231"
/*#define SUN_AUD_DEV "SUNW,sbpro"*/


#endif

/******************************************************************
 * SVR4
 ******************************************************************
 ****    ***  ****  **     ***  *  ********************************
 ***  ******  ****  **  **  **  *  ********************************
 *****  *****  **  ***     ***      *******************************
 *******  ***  **  ***  **  *****  ********************************
 ****    *****    ****  **  *****  ********************************
 ******************************************************************/
#if (defined(SVR4) || defined(__SVR4)) && !defined(sun) && !defined(__sun) && !defined(sony_news) && !defined(__sony_news) 

#define DEFAULT_CD_DEVICE       "/dev/rcdrom/cd0"

#endif

/******************************************************************
 * Ultrix
 ******************************************************************
 ***  **  **  *****      **     ***      **  **  ******************
 ***  **  **  *******  ****  **  ****  *****    *******************
 ***  **  **  *******  ****     *****  ******  ********************
 ***  **  **  *******  ****  **  ****  *****    *******************
 ****    ***      ***  ****  **  **      **  **  ******************
 ******************************************************************/
#if defined(ultrix) || defined(__ultrix)

#endif

/******************************************************************
 * IBM AIX
 ******************************************************************
 ****    ***      **  **  *****************************************
 ***  **  ****  *****    ******************************************
 ***      ****  ******  *******************************************
 ***  **  ****  *****    ******************************************
 ***  **  **      **  **  *****************************************
 ******************************************************************/
#if defined(AIXV3) || defined(__AIXV3)

#define DEFAULT_CD_DEVICE	"/dev/cd0"

#endif /* IBM AIX */

/******************************************************************/ 
#endif /* WM_CONFIG_H */








