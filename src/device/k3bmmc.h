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


#include <inttypes.h>
#include <linux/cdrom.h>
#include <endian.h>


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
    unsigned char reserved2 : 5;
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned char reserved2 : 5;
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
    uint8_t reserved3;
    uint8_t opc_entries;
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
};


#endif
