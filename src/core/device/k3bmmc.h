/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_MMC_H_
#define _K3B_MMC_H_

// The symbol defined when compiling under linux varies greatly. 
// linux, Linux, and __linux__ are all known to be used. Perhaps
// the best approach is to #include <qglobal.h> and use Q_OS_LINUX
// instead.
#include <qglobal.h>

#ifdef Q_OS_LINUX
#include <inttypes.h>

/* Fix definitions for 2.5 kernels */
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,21) 
typedef unsigned long long __u64; 
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,50)
typedef unsigned char u8;
#endif

#include <linux/cdrom.h>
#include <endian.h>

#else
// Assume FreeBSD, 4.x
#include <inttypes.h>
#include <sys/endian.h>
#define __BYTE_ORDER BYTE_ORDER
#define __BIG_ENDIAN BIG_ENDIAN
#define __LITTLE_ENDIAN LITTLE_ENDIAN
#endif

namespace K3bCdDevice 
{
  /* 
   * struct disc_info taken from cdrwtool.h
   *
   *
   * disc status (status):
   *     00b - empty disc
   *     01b - incomplete disc (appendable)
   *     10b - Complete disc
   *     11b - Others
   *
   * State of last session (border)
   *     00b - Empty session
   *     01b - Incomplete session
   *     10b - Reseverd
   *     11b - Complete session (only possible when disc status is complete)
   */

  typedef struct disc_info {
    uint16_t length;
#if __BYTE_ORDER == __BIG_ENDIAN
    unsigned char reserved1 : 3;
    unsigned char erasable  : 1;
    unsigned char border    : 2;
    unsigned char status    : 2;
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned char status    : 2;
    unsigned char border    : 2;
    unsigned char erasable  : 1;
    unsigned char reserved1 : 3;
#else
#error "<bits/endian.h> is wack"
#endif
    uint8_t n_first_track;
    uint8_t n_sessions_l;
    uint8_t first_track_l;
    uint8_t last_track_l;
#if __BYTE_ORDER == __BIG_ENDIAN
    unsigned char did_v     : 1;
    unsigned char dbc_v     : 1;
    unsigned char uru       : 1;
    unsigned char reserved2 : 2;
    unsigned char dbit      : 1;
    unsigned char bg_f_status : 1;
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned char bg_f_status : 1;
    unsigned char dbit      : 1;
    unsigned char reserved2 : 2;
    unsigned char uru       : 1;
    unsigned char dbc_v     : 1;
    unsigned char did_v     : 1;
#endif

    /*
     * disc type
     *    00h - CD-DA of CDROM
     *    10h - CD-I
     *    20h - CD-ROM XA
     *    FFh - Undefined
     *    All other values are reserved
     */
    uint8_t disc_type;
    uint8_t n_sessions_m;
    uint8_t first_track_m;
    uint8_t last_track_m;
    uint32_t disc_id;

    /*
     * Last session lead-in start time
     * if the disc is complete this shall be FF/FF/FF
     */
    uint8_t lead_in_r;
    uint8_t lead_in_m;
    uint8_t lead_in_s;
    uint8_t lead_in_f;

    /*
     * Last possible start time for start of lead-in
     * if the disc is complete this shall be FF/FF/FF
     */
    uint8_t lead_out_r;
    uint8_t lead_out_m;
    uint8_t lead_out_s;
    uint8_t lead_out_f;

    uint8_t disc_bar_code[8];

    //
    // We need to make sure the structure has a proper size
    // I think it needs to be a power of 2.
    // With ide-scsi there is no problem. But without the
    // GPCMD_READ_DISC_INFO command failes if the size is 34
    //

/*     uint8_t reserved3; */
/*     uint8_t opc_entries; */
  } disc_info_t;



  /* 
   * struct track_info taken from cdrwtool.h
   */
  typedef struct track_info {
    uint16_t info_length;
    uint8_t track_number_l;
    uint8_t session_number_l;
    uint8_t reserved1;
#if __BYTE_ORDER == __BIG_ENDIAN
    uint8_t reserved2               : 2;
    uint8_t damage          : 1;
    uint8_t copy            : 1;
    uint8_t track_mode              : 4;
    uint8_t rt                      : 1;
    uint8_t blank           : 1;
    uint8_t packet          : 1;
    uint8_t fp                      : 1;
    uint8_t data_mode               : 4;
    uint8_t reserved3               : 6;
    uint8_t lra_v           : 1;
    uint8_t nwa_v           : 1;
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    uint8_t track_mode              : 4;
    uint8_t copy            : 1;
    uint8_t damage          : 1;
    uint8_t reserved2               : 2;
    uint8_t data_mode               : 4;
    uint8_t fp                      : 1;
    uint8_t packet          : 1;
    uint8_t blank           : 1;
    uint8_t rt                      : 1;
    uint8_t nwa_v           : 1;
    uint8_t lra_v           : 1;
    uint8_t reserved3               : 6;
#endif
    uint32_t track_start;
    uint32_t next_writable;
    uint32_t free_blocks;
    uint32_t packet_size;
    uint32_t track_size;
    uint32_t last_recorded;
    uint8_t track_number_m;
    uint8_t session_number_m;
    uint8_t reserved4;
    uint8_t reserved5;
  } track_info_t;


  /*
   * Use this with the GPCMD_READ_TOC_PMA_ATIP command
   * where format is set to 2 (Full TOC)
   */
  typedef struct toc_track_descriptor {
    uint8_t session_number;
#if __BYTE_ORDER == __BIG_ENDIAN
    unsigned char adr     : 4;
    unsigned char control : 4;
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned char control : 4;
    unsigned char adr     : 4;
#endif
    uint8_t tno;
    uint8_t point;
    uint8_t min;
    uint8_t sec;
    uint8_t frame;
    uint8_t zero;
    uint8_t p_min;
    uint8_t p_sec;
    uint8_t p_frame;
  } toc_track_descriptor_t;


  /*
   * Use this with the GPCMD_READ_TOC_PMA_ATIP command
   */
  typedef struct toc_pma_atip {
    uint16_t length;
    uint8_t first;
    uint8_t last;
    toc_track_descriptor_t descriptor;
  } toc_pma_atip_t;


  struct cd_wr_speed_performance {
    unsigned char res0;                   /* Reserved                          */
    unsigned char rot_ctl_sel     : 2;    /* Rotational control selected       */
    unsigned char res_1_27        : 6;    /* Reserved                          */
    unsigned char wr_speed_supp[2];       /* Supported write speed             */
  };
  
  /**
   * Based on the cdrecord struct cd_mode_page_2A
   * MM Capabilities and Mechanical Status Page
   */
  struct mm_cap_page_2A {
    unsigned char PS               : 1;
    unsigned char res_1            : 1;
    unsigned char page_code        : 6;
    unsigned char page_len;             /* 0x14 = 20 Bytes (MMC) */
                                        /* 0x18 = 24 Bytes (MMC-2) */
                                        /* 0x1C >= 28 Bytes (MMC-3) */
    unsigned char res_2_67         : 2; /* Reserved        */
    unsigned char dvd_ram_read     : 1; /* Reads DVD-RAM media       */
    unsigned char dvd_r_read       : 1; /* Reads DVD-R media       */
    unsigned char dvd_rom_read     : 1; /* Reads DVD ROM media       */
    unsigned char method2          : 1; /* Reads fixed packet method2 media  */
    unsigned char cd_rw_read       : 1; /* Reads CD-RW media       */
    unsigned char cd_r_read        : 1; /* Reads CD-R  media       */
    unsigned char res_3_67         : 2; /* Reserved        */
    unsigned char dvd_ram_write    : 1; /* Supports writing DVD-RAM media    */
    unsigned char dvd_r_write      : 1; /* Supports writing DVD-R media      */
    unsigned char res_3_3          : 1; /* Reserved        */
    unsigned char test_write       : 1; /* Supports emulation write      */
    unsigned char cd_rw_write      : 1; /* Supports writing CD-RW media      */
    unsigned char cd_r_write       : 1; /* Supports writing CD-R  media      */
    unsigned char BUF              : 1; /* Supports Buffer under. free rec.  */
    unsigned char multi_session    : 1; /* Reads multi-session media      */
    unsigned char mode_2_form_2    : 1; /* Reads Mode-2 form 2 media      */
    unsigned char mode_2_form_1    : 1; /* Reads Mode-2 form 1 media (XA)    */
    unsigned char digital_port_1   : 1; /* Supports digital output on port 1 */
    unsigned char digital_port_2   : 1; /* Supports digital output on port 2 */
    unsigned char composite        : 1; /* Deliveres composite A/V stream    */
    unsigned char audio_play       : 1; /* Supports Audio play operation     */
    unsigned char read_bar_code    : 1; /* Supports reading bar codes      */
    unsigned char UPC              : 1; /* Reads media catalog number (UPC)  */
    unsigned char ISRC             : 1; /* Reads ISRC information      */
    unsigned char c2_pointers      : 1; /* Supports C2 error pointers      */
    unsigned char rw_deint_corr    : 1; /* Reads de-interleved R-W sub chan  */
    unsigned char rw_supported     : 1; /* Reads R-W sub channel information */
    unsigned char cd_da_accurate   : 1; /* READ CD data stream is accurate   */
    unsigned char cd_da_supported  : 1; /* Reads audio data with READ CD cmd */
    unsigned char loading_type     : 3; /* Loading mechanism type      */
    unsigned char res_6_4          : 1; /* Reserved        */
    unsigned char eject            : 1; /* Ejects disc/cartr with STOP LoEj  */
    unsigned char prevent_jumper   : 1; /* State of prev/allow jumper 0=pres */
    unsigned char lock_state       : 1; /* Lock state 0=unlocked 1=locked    */
    unsigned char lock             : 1; /* PREVENT/ALLOW may lock media      */
    unsigned char res_7            : 2; /* Reserved        */
    unsigned char rw_in_lead_in    : 1; /* Reads raw R-W subcode from lead in */
    unsigned char side_change      : 1; /* Side change capable       */
    unsigned char sw_slot_sel      : 1; /* Load empty slot in changer      */
    unsigned char disk_present_rep : 1; /* Changer supports disk present rep */
    unsigned char sep_chan_mute    : 1; /* Mute controls each channel separat*/
    unsigned char sep_chan_vol     : 1; /* Vol controls each channel separat */
    unsigned char max_read_speed[2];    /* Max. read speed in KB/s      */
                                        /* obsolete in MMC-4 */
    unsigned char num_vol_levels[2];    /* # of supported volume levels      */
    unsigned char buffer_size[2];       /* Buffer size for the data in KB    */
    unsigned char cur_read_speed[2];    /* Current read speed in KB/s      */
                                        /* obsolete in MMC-4 */
    unsigned char res_16;               /* Reserved        */
    unsigned char res_17           : 2; /* Reserved        */
    unsigned char length           : 2; /* 0=32BCKs 1=16BCKs 2=24BCKs 3=24I2c*/
    unsigned char LSBF             : 1; /* Set: LSB first Clear: MSB first   */
    unsigned char RCK              : 1; /* Set: HIGH high LRCK=left channel  */
    unsigned char BCK              : 1; /* Data valid on falling edge of BCK */
    unsigned char res_17_0         : 1; /* Reserved        */
    unsigned char max_write_speed[2];   /* Max. write speed supported in KB/s*/
                                        /* obsolete in MMC-4 */
    unsigned char cur_write_speed[2];   /* Current write speed in KB/s      */
                                        /* obsolete in MMC-4 */

    /* Byte 22 ... Only in MMC-2      */
    unsigned char copy_man_rev[2];      /* Copy management revision supported*/
    unsigned char res_24;               /* Reserved        */
    unsigned char res_25;               /* Reserved        */

    /* Byte 26 ... Only in MMC-3      */
    unsigned char res_26;               /* Reserved        */
    unsigned char res_27_27        : 6; /* Reserved        */
    unsigned char rot_ctl_sel      : 2; /* Rotational control selected      */
    unsigned char v3_cur_write_speed[2]; /* Current write speed in KB/s      */
    unsigned char num_wr_speed_des[2];  /* # of wr speed perf descr. tables  */
    struct cd_wr_speed_performance
    wr_speed_des[1];                    /* wr speed performance descriptor   */
                                        /* Actually more (num_wr_speed_des)  */
  };
}


#endif
