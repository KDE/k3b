/* 

    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef _K3B_MMC_H_
#define _K3B_MMC_H_

#include <config-k3b.h>


namespace K3b {
    namespace Device 
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
         *     10b - Reserved
         *     11b - Complete session (only possible when disc status is complete)
         */
        typedef struct disc_info {
            quint16 length;
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
            unsigned char reserved1 : 3;
            unsigned char erasable  : 1;
            unsigned char border    : 2;
            unsigned char status    : 2;
#else
            unsigned char status    : 2;
            unsigned char border    : 2;
            unsigned char erasable  : 1;
            unsigned char reserved1 : 3;
#endif
            unsigned char n_first_track;
            unsigned char n_sessions_l;
            unsigned char first_track_l;
            unsigned char last_track_l;
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
            unsigned char did_v     : 1;
            unsigned char dbc_v     : 1;
            unsigned char uru       : 1;
            unsigned char reserved2 : 2;
            unsigned char dbit      : 1;
            unsigned char bg_f_status : 1;
#else
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
            unsigned char disc_type;
            unsigned char n_sessions_m;
            unsigned char first_track_m;
            unsigned char last_track_m;
            quint32 disc_id;

            /*
             * Last session lead-in start time
             * if the disc is complete this shall be FF/FF/FF
             */
            unsigned char lead_in_r;
            unsigned char lead_in_m;
            unsigned char lead_in_s;
            unsigned char lead_in_f;

            /*
             * Last possible start time for start of lead-in
             * if the disc is complete this shall be FF/FF/FF
             */
            unsigned char lead_out_r;
            unsigned char lead_out_m;
            unsigned char lead_out_s;
            unsigned char lead_out_f;

            unsigned char disc_bar_code[8];

            //
            // We need to make sure the structure has a proper size
            // I think it needs to be a power of 2.
            // With ide-scsi there is no problem. But without the
            // GPCMD_READ_DISC_INFO command failes if the size is 34
            //

/*     unsigned char reserved3; */
/*     unsigned char opc_entries; */
        } disc_info_t;



        /* 
         * struct track_info taken from cdrwtool.h
         */
        typedef struct track_info {
            unsigned char data_length[2];
            unsigned char track_number_l;
            unsigned char session_number_l;
            unsigned char reserved1;
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
            unsigned char reserved2       : 2;
            unsigned char damage          : 1;
            unsigned char copy            : 1;
            unsigned char track_mode      : 4;
            unsigned char rt              : 1;
            unsigned char blank           : 1;
            unsigned char packet          : 1;
            unsigned char fp              : 1;
            unsigned char data_mode       : 4;
            unsigned char reserved3       : 6;
            unsigned char lra_v           : 1;
            unsigned char nwa_v           : 1;
#else
            unsigned char track_mode      : 4;
            unsigned char copy            : 1;
            unsigned char damage          : 1;
            unsigned char reserved2       : 2;
            unsigned char data_mode       : 4;
            unsigned char fp              : 1;
            unsigned char packet          : 1;
            unsigned char blank           : 1;
            unsigned char rt              : 1;
            unsigned char nwa_v           : 1;
            unsigned char lra_v           : 1;
            unsigned char reserved3       : 6;
#endif
            unsigned char track_start[4];
            unsigned char next_writable[4];
            unsigned char free_blocks[4];
            unsigned char packet_size[4];
            unsigned char track_size[4];
            unsigned char last_recorded[4];
            unsigned char track_number_m;
            unsigned char session_number_m;
            unsigned char reserved4;
            unsigned char reserved5;
            unsigned char read_compatibility[4];
        } track_info_t;


        /*
         * Use this with the GPCMD_READ_TOC_PMA_ATIP command
         * where format is set to 2 (Full TOC)
         */
        struct toc_raw_track_descriptor {
            unsigned char session_number;
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
            unsigned char adr     : 4;
            unsigned char control : 4;
#else
            unsigned char control : 4;
            unsigned char adr     : 4;
#endif
            unsigned char tno;
            unsigned char point;
            unsigned char min;
            unsigned char sec;
            unsigned char frame;
            unsigned char zero;
            unsigned char p_min;
            unsigned char p_sec;
            unsigned char p_frame;
        };


#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
        struct cd_wr_speed_performance {
            unsigned char res0;                   /* Reserved                          */
            unsigned char res_1_27        : 6;    /* Reserved                          */
            unsigned char rot_ctl_sel     : 2;    /* Rotational control selected       */
            unsigned char wr_speed_supp[2];       /* Supported write speed             */
        };
#else
        struct cd_wr_speed_performance {
            unsigned char res0;                   /* Reserved                          */
            unsigned char rot_ctl_sel     : 2;    /* Rotational control selected       */
            unsigned char res_1_27        : 6;    /* Reserved                          */
            unsigned char wr_speed_supp[2];       /* Supported write speed             */
        };
#endif


        /**
         * Based on the cdrecord struct cd_mode_page_2A
         * MM Capabilities and Mechanical Status Page
         */
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN

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
            unsigned char sep_chan_mute    : 1; /* Mute controls each channel separately */
            unsigned char sep_chan_vol     : 1; /* Vol controls each channel separately */
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

#else  // LITTLE_ENDIAN

        struct mm_cap_page_2A {
            unsigned char page_code        : 6;
            unsigned char res_1            : 1;
            unsigned char PS               : 1;
            unsigned char page_len;             /* 0x14 = 20 Bytes (MMC) */
            /* 0x18 = 24 Bytes (MMC-2) */
            /* 0x1C >= 28 Bytes (MMC-3) */
            unsigned char cd_r_read        : 1; /* Reads CD-R  media       */
            unsigned char cd_rw_read       : 1; /* Reads CD-RW media       */
            unsigned char method2          : 1; /* Reads fixed packet method2 media  */
            unsigned char dvd_rom_read     : 1; /* Reads DVD ROM media       */
            unsigned char dvd_r_read       : 1; /* Reads DVD-R media       */
            unsigned char dvd_ram_read     : 1; /* Reads DVD-RAM media       */
            unsigned char res_2_67         : 2; /* Reserved        */
            unsigned char cd_r_write       : 1; /* Supports writing CD-R  media      */
            unsigned char cd_rw_write      : 1; /* Supports writing CD-RW media      */
            unsigned char test_write       : 1; /* Supports emulation write      */
            unsigned char res_3_3          : 1; /* Reserved        */
            unsigned char dvd_r_write      : 1; /* Supports writing DVD-R media      */
            unsigned char dvd_ram_write    : 1; /* Supports writing DVD-RAM media    */
            unsigned char res_3_67         : 2; /* Reserved        */
            unsigned char audio_play       : 1; /* Supports Audio play operation     */
            unsigned char composite        : 1; /* Deliveres composite A/V stream    */
            unsigned char digital_port_2   : 1; /* Supports digital output on port 2 */
            unsigned char digital_port_1   : 1; /* Supports digital output on port 1 */
            unsigned char mode_2_form_1    : 1; /* Reads Mode-2 form 1 media (XA)    */
            unsigned char mode_2_form_2    : 1; /* Reads Mode-2 form 2 media      */
            unsigned char multi_session    : 1; /* Reads multi-session media      */
            unsigned char BUF              : 1; /* Supports Buffer under. free rec.  */
            unsigned char cd_da_supported  : 1; /* Reads audio data with READ CD cmd */
            unsigned char cd_da_accurate   : 1; /* READ CD data stream is accurate   */
            unsigned char rw_supported     : 1; /* Reads R-W sub channel information */
            unsigned char rw_deint_corr    : 1; /* Reads de-interleved R-W sub chan  */
            unsigned char c2_pointers      : 1; /* Supports C2 error pointers      */
            unsigned char ISRC             : 1; /* Reads ISRC information      */
            unsigned char UPC              : 1; /* Reads media catalog number (UPC)  */
            unsigned char read_bar_code    : 1; /* Supports reading bar codes      */
            unsigned char lock             : 1; /* PREVENT/ALLOW may lock media      */
            unsigned char lock_state       : 1; /* Lock state 0=unlocked 1=locked    */
            unsigned char prevent_jumper   : 1; /* State of prev/allow jumper 0=pres */
            unsigned char eject            : 1; /* Ejects disc/cartr with STOP LoEj  */
            unsigned char res_6_4          : 1; /* Reserved        */
            unsigned char loading_type     : 3; /* Loading mechanism type      */
            unsigned char sep_chan_vol     : 1; /* Vol controls each channel separately */
            unsigned char sep_chan_mute    : 1; /* Mute controls each channel separately */
            unsigned char disk_present_rep : 1; /* Changer supports disk present rep */
            unsigned char sw_slot_sel      : 1; /* Load empty slot in changer      */
            unsigned char side_change      : 1; /* Side change capable       */
            unsigned char rw_in_lead_in    : 1; /* Reads raw R-W subcode from lead in */
            unsigned char res_7            : 2; /* Reserved        */
            unsigned char max_read_speed[2];    /* Max. read speed in KB/s      */
            /* obsolete in MMC-4 */
            unsigned char num_vol_levels[2];    /* # of supported volume levels      */
            unsigned char buffer_size[2];       /* Buffer size for the data in KB    */
            unsigned char cur_read_speed[2];    /* Current read speed in KB/s      */
            /* obsolete in MMC-4 */
            unsigned char res_16;               /* Reserved        */
            unsigned char res_17_0         : 1; /* Reserved        */
            unsigned char BCK              : 1; /* Data valid on falling edge of BCK */
            unsigned char RCK              : 1; /* Set: HIGH high LRCK=left channel  */
            unsigned char LSBF             : 1; /* Set: LSB first Clear: MSB first   */
            unsigned char length           : 2; /* 0=32BCKs 1=16BCKs 2=24BCKs 3=24I2c*/
            unsigned char res_17           : 2; /* Reserved        */
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
            unsigned char rot_ctl_sel      : 2; /* Rotational control selected      */
            unsigned char res_27_27        : 6; /* Reserved        */
            unsigned char v3_cur_write_speed[2]; /* Current write speed in KB/s      */
            unsigned char num_wr_speed_des[2];  /* # of wr speed perf descr. tables  */
            struct cd_wr_speed_performance
            wr_speed_des[1];                    /* wr speed performance descriptor   */
            /* Actually more (num_wr_speed_des)  */
        };
#endif

        /**
         * Based on the cdrecord struct cd_mode_page_05
         * Write Parameters Mode Page
         */
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
        struct wr_param_page_05 {                /* write parameters */
            unsigned char PS               : 1;
            unsigned char res_1            : 1;
            unsigned char page_code        : 6;
            unsigned char page_len;                /* 0x32 = 50 Bytes */
            unsigned char res_2_7          : 1;
            unsigned char BUFE             : 1;    /* Enable Bufunderrun free rec.      */
            unsigned char LS_V             : 1;    /* Link size valid                   */
            unsigned char test_write       : 1;    /* Do not actually write data        */
            unsigned char write_type       : 4;    /* Session write type (PACKET/TAO...)*/
            unsigned char multi_session    : 2;    /* Multi session write type          */
            unsigned char fp               : 1;    /* Fixed packed (if in packet mode)  */
            unsigned char copy             : 1;    /* 1st higher gen of copy prot track */
            unsigned char track_mode       : 4;    /* Track mode (Q-sub control nibble) */
            unsigned char res_4            : 4;    /* Reserved                          */
            unsigned char dbtype           : 4;    /* Data block type                   */
            unsigned char link_size;               /* Link Size (default is 7)          */
            unsigned char res_6;                   /* Reserved                          */
            unsigned char res_7            : 2;    /* Reserved                          */
            unsigned char host_appl_code   : 6;    /* Host application code of disk     */
            unsigned char session_format;          /* Session format (DA/CDI/XA)        */
            unsigned char res_9;                   /* Reserved                          */
            unsigned char packet_size[4];          /* # of user datablocks/fixed packet */
            unsigned char audio_pause_len[2];      /* # of blocks where index is zero   */
            unsigned char media_cat_number[16];    /* Media catalog Number (MCN)        */
            unsigned char ISRC[14];                /* ISRC for this track               */
            unsigned char sub_header[4];
            unsigned char vendor_uniq[4];
        };

#else // __LITTLE_ENDIAN
        struct wr_param_page_05 {		/* write parameters */
            unsigned char page_code        : 6;
            unsigned char res_1            : 1;
            unsigned char PS               : 1;
            unsigned char p_len;		/* 0x32 = 50 Bytes */
            unsigned char write_type	   : 4;	/* Session write type (PACKET/TAO...)*/
            unsigned char test_write	   : 1;	/* Do not actually write data	     */
            unsigned char LS_V		   : 1;	/* Link size valid		     */
            unsigned char BUFE		   : 1;	/* Enable Bufunderrun free rec.	     */
            unsigned char res_2_7	   : 1;
            unsigned char track_mode	   : 4;	/* Track mode (Q-sub control nibble) */
            unsigned char copy		   : 1;	/* 1st higher gen of copy prot track ~*/
            unsigned char fp		   : 1;	/* Fixed packed (if in packet mode)  */
            unsigned char multi_session	   : 2;	/* Multi session write type	     */
            unsigned char dbtype	   : 4;	/* Data block type		     */
            unsigned char res_4		   : 4;	/* Reserved			     */
            unsigned char link_size;		/* Link Size (default is 7)	     */
            unsigned char res_6;	       	/* Reserved			     */
            unsigned char host_appl_code   : 6;	/* Host application code of disk     */
            unsigned char res_7		   : 2;	/* Reserved			     */
            unsigned char session_format;	/* Session format (DA/CDI/XA)	     */
            unsigned char res_9;		/* Reserved			     */
            unsigned char packet_size[4];	/* # of user datablocks/fixed packet */
            unsigned char audio_pause_len[2];	/* # of blocks where index is zero   */
            unsigned char media_cat_number[16];	/* Media catalog Number (MCN)	     */
            unsigned char ISRC[14];		/* ISRC for this track		     */
            unsigned char sub_header[4];
            unsigned char vendor_uniq[4];
        };
#endif


        struct toc_track_descriptor {
            unsigned char res1;
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
            unsigned char adr     : 4;
            unsigned char control : 4;
#else
            unsigned char control : 4;
            unsigned char adr     : 4;
#endif
            unsigned char track_no;
            unsigned char res2;
            unsigned char start_adr[4];
        };


        struct atip_descriptor {
            unsigned char dataLength[2];
            unsigned char res1;
            unsigned char res2;
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
            unsigned char ind_wr_power : 4;  // indicated writing power
            unsigned char ddcd         : 1;  // DDCD
            unsigned char ref_speed    : 3;  // reference Speed
            unsigned char zero         : 1;  // 0
            unsigned char uru          : 1;  // Uru
            unsigned char res3         : 6;
            unsigned char one          : 1;  // 1
            unsigned char disc_type    : 1;  // Disc Type
            unsigned char disc_subtype : 3;  // Disc Sub-Type
            unsigned char a1_valid     : 1;
            unsigned char a2_valid     : 1;
            unsigned char a3_valid     : 1;
#else
            unsigned char ref_speed    : 3;  // reference Speed
            unsigned char ddcd         : 1;  // DDCD
            unsigned char ind_wr_power : 4;  // indicated writing power
            unsigned char res3         : 6;
            unsigned char uru          : 1;  // Uru
            unsigned char zero         : 1;  // 0
            unsigned char a3_valid     : 1;
            unsigned char a2_valid     : 1;
            unsigned char a1_valid     : 1;
            unsigned char disc_subtype : 3;  // Disc Sub-Type
            unsigned char disc_type    : 1;  // Disc Type
            unsigned char one          : 1;  // 1
#endif
            unsigned char res4;
            unsigned char lead_in_m;
            unsigned char lead_in_s;
            unsigned char lead_in_f;
            unsigned char res5;
            unsigned char lead_out_m;
            unsigned char lead_out_s;
            unsigned char lead_out_f;
            unsigned char res6;
            unsigned char a1[3];
            unsigned char res7;
            unsigned char a2[3];
            unsigned char res8;
            unsigned char a3[3];
            unsigned char res9;
            unsigned char s4[3];
            unsigned char res10;
        };


        struct mechanism_status_header {
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
            unsigned char fault         : 1;
            unsigned char changer_state : 2;
            unsigned char slot_low      : 5;
#else
            unsigned char slot_low      : 5;
            unsigned char changer_state : 2;
            unsigned char fault         : 1;
#endif
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
            unsigned char mech_state    : 3;
            unsigned char door_open     : 1;
            unsigned char res1          : 1;
            unsigned char slot_high     : 3;
#else
            unsigned char slot_high     : 3;
            unsigned char res1          : 1;
            unsigned char door_open     : 1;
            unsigned char mech_state    : 3;
#endif
            unsigned char current_lba[3];
            unsigned char num_slots;
            unsigned char slot_len[2];
        };

        struct mechanism_status_slot {
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
            unsigned char disc_present  : 1;
            unsigned char res1          : 6;
            unsigned char change        : 1;
#else
            unsigned char change        : 1;
            unsigned char res1          : 6;
            unsigned char disc_present  : 1;
#endif
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
            unsigned char res2          : 6;
            unsigned char cwp_v         : 1;
            unsigned char cwp           : 1;
#else
            unsigned char cwp           : 1;
            unsigned char cwp_v         : 1;
            unsigned char res2          : 6;
#endif
            unsigned char res3;
            unsigned char res4;
        };


        struct inquiry {
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
            unsigned char p_qualifier   : 3;
            unsigned char p_device_type : 5;
            unsigned char rmb           : 1;
            unsigned char reserved1     : 7;
#else
            unsigned char p_device_type : 5;
            unsigned char p_qualifier   : 3;
            unsigned char reserved1     : 7;
            unsigned char rmb           : 1;
#endif
            unsigned char version;
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
            unsigned char interface_dep : 4;
            unsigned char data_format   : 4;
#else
            unsigned char data_format   : 4;
            unsigned char interface_dep : 4;
#endif
            unsigned char add_length;
            unsigned char reserved2;
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
            unsigned char bque          : 1;
            unsigned char enc_serv      : 1;
            unsigned char vs1           : 1;
            unsigned char multi_p       : 1;
            unsigned char m_chngr       : 1;
            unsigned char reserved3     : 1;
            unsigned char reserved4     : 1;
            unsigned char addr_16       : 1;
            unsigned char rel_adr       : 1;
            unsigned char reserved5     : 1;
            unsigned char w_bus_16      : 1;
            unsigned char sync          : 1;
            unsigned char linked        : 1;
            unsigned char reserved6     : 1;
            unsigned char cmd_que       : 1;
            unsigned char vs2           : 1;
#else
            unsigned char addr_16       : 1;
            unsigned char reserved4     : 1;
            unsigned char reserved3     : 1;
            unsigned char m_chngr       : 1;
            unsigned char multi_p       : 1;
            unsigned char vs1           : 1;
            unsigned char enc_serv      : 1;
            unsigned char bque          : 1;
            unsigned char vs2           : 1;
            unsigned char cmd_que       : 1;
            unsigned char reserved6     : 1;
            unsigned char linked        : 1;
            unsigned char sync          : 1;
            unsigned char w_bus_16      : 1;
            unsigned char reserved5     : 1;
            unsigned char rel_adr       : 1;
#endif
            unsigned char vendor[8];
            unsigned char product[16];
            unsigned char revision[4];
            unsigned char vendor_specific[20];
            unsigned char reserved7[2];
            unsigned char version1[2];
            unsigned char version2[2];
            unsigned char version3[2];
            unsigned char version4[2];
            unsigned char version5[2];
            unsigned char version6[2];
            unsigned char version7[2];
            unsigned char version8[2];

            // bytes 74-95: reserved
            // bytes 96-n: vendor specific
        };


        struct ricoh_mode_page_30 {
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
            unsigned char PS               : 1;
            unsigned char res_1            : 1;
            unsigned char page_code        : 6;
#else
            unsigned char page_code        : 6;
            unsigned char res_1            : 1;
            unsigned char PS               : 1;
#endif
            unsigned char page_len;                  /* 0xE = 14 Bytes */
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
            unsigned char res_2_67        :2;
            unsigned char AWSCS           :1;     /* Auto write speed control supp. */
            unsigned char ARSCS           :1;     /* Auto read speed control supp. */
            unsigned char res_2_23        :2;
            unsigned char TWBFS           :1;     /* Test Burn-Free sup.  */
            unsigned char BUEFS           :1;     /* Burn-Free supported  */
#else
            unsigned char BUEFS           :1;     /* Burn-Free supported  */
            unsigned char TWBFS           :1;     /* Test Burn-Free sup.  */
            unsigned char res_2_23        :2;
            unsigned char ARSCS           :1;     /* Auto read speed control supp. */
            unsigned char AWSCS           :1;     /* Auto write speed control supp. */
            unsigned char res_2_67        :2;
#endif
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
            unsigned char res_3_67        :2;
            unsigned char AWSCD           :1;     /* Auto write speed control disabled */
            unsigned char ARSCE           :1;     /* Auto read speed control enabled */
            unsigned char res_2_13        :3;
            unsigned char BUEFE           :1;     /* Burn-Free enabled    */
#else
            unsigned char BUEFE           :1;     /* Burn-Free enabled    */
            unsigned char res_2_13        :3;
            unsigned char ARSCE           :1;     /* Auto read speed control enabled */
            unsigned char AWSCD           :1;     /* Auto write speed control disabled */
            unsigned char res_3_67        :2;
#endif
            unsigned char link_counter[2];        /* Burn-Free link counter */
            unsigned char res[10];                /* Padding up to 16 bytes */
        };
    }
}

#endif
