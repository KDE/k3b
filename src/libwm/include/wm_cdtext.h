/***************************************************************************
                          wm_cdtext.h  -  description
                             -------------------
    begin                : Mon Feb 12 2001
    copyright            : (C) 2001 by Alex Kern
    email                : alex.kern@gmx.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*
 * cdtext base structure and defines
 */
#ifndef WM_CDTEXT_H
#define WM_CDTEXT_H

extern struct cdtext_info wm_cdtext_info;

#define MAX_LENGHT_OF_CDTEXT_STRING 162 /* max 160 bytes + 2 * 0x00 by UNICODES */
#define DATAFIELD_LENGHT_IN_PACK 12
#define MAX_LANGUAGE_BLOCKS 8

struct cdtext_pack_data_header {
  unsigned char header_field_id1_typ_of_pack;
  unsigned char header_field_id2_tracknumber;
  unsigned char header_field_id3_sequence;
  unsigned char header_field_id4_block_no;
  unsigned char text_data_field[DATAFIELD_LENGHT_IN_PACK];
  unsigned char crc_byte1;
  unsigned char crc_byte2;
};

typedef unsigned char cdtext_string[MAX_LENGHT_OF_CDTEXT_STRING];

/* meke it more generic
   it can be up to 8 blocks with different encoding */

struct cdtext_info_block {
  /* management */
  unsigned char block_code;
  unsigned char block_unicode; /* 0 - single chars, 1 - doublebytes */
  unsigned char block_encoding; /* orange book -? */
  cdtext_string* block_encoding_text;

  /* variable part of cdtext */
  cdtext_string* name;
  cdtext_string* performer;
  cdtext_string* songwriter;
  cdtext_string* composer;
  cdtext_string* arranger;
  cdtext_string* message;
  cdtext_string* UPC_EAN_ISRC_code;

  /* fix part of cdtext */
  unsigned char binary_disc_identification_info[DATAFIELD_LENGHT_IN_PACK];
  unsigned char binary_genreidentification_info[DATAFIELD_LENGHT_IN_PACK];
  unsigned char binary_size_information[DATAFIELD_LENGHT_IN_PACK];
};

struct cdtext_info {
  /* somethimes i get hunderts of bytes, without anyone valid pack
     my CDU-561 for example */
  int count_of_entries; /* one more becose album need one too */
  int count_of_valid_packs;
  int count_of_invalid_packs;
  int valid;

  struct cdtext_info_block *blocks[MAX_LANGUAGE_BLOCKS];
};

#ifndef IGNORE_FEATURE_LIST

struct feature_list_header {
  unsigned char lenght_msb;
  unsigned char lenght_1sb;
  unsigned char lenght_2sb;
  unsigned char lenght_lsb;
  unsigned char reserved1;
  unsigned char reserved2;
  unsigned char profile_msb;
  unsigned char profile_lsb;
};

struct feature_descriptor_cdread {
  unsigned char feature_code_msb;
  unsigned char feature_code_lsb;
  unsigned char settings;
  unsigned char add_lenght;
  unsigned char add_settings;
  unsigned char reserved1;
  unsigned char reserved2;
  unsigned char reserved3;
};

#endif /* IGNORE_FEATURE_LIST */

int wm_get_cdtext(struct wm_drive*);
void wm_free_cdtext(void);

#endif /* WM_CDTEXT_H */
