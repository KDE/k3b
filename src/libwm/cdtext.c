/***************************************************************************
                          cdtext.c  -  description
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

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef libunicode
  #include <unicode.h>
#endif
/* #include <sys/time.h> */

#include "include/wm_config.h"
#include "include/wm_struct.h"
/*#include "include/wm_cdrom.h"*/
#include "include/wm_database.h"
#include "include/wm_platform.h"
#include "include/wm_helpers.h"
#include "include/wm_cdinfo.h"
#include "include/wm_cdtext.h"

struct cdtext_info wm_cdtext_info;
static int first_initialise = 1;

int free_cdtext_info_block(struct cdtext_info_block* cdtextinfoblock)
{
  if(cdtextinfoblock)
  {
    if(cdtextinfoblock->name)
      free(cdtextinfoblock->name);
    if(cdtextinfoblock->performer)
      free(cdtextinfoblock->performer);
    if(cdtextinfoblock->songwriter)
      free(cdtextinfoblock->songwriter);
    if(cdtextinfoblock->composer)
      free(cdtextinfoblock->composer);
    if(cdtextinfoblock->arranger)
      free(cdtextinfoblock->arranger);
    if(cdtextinfoblock->message)
      free(cdtextinfoblock->message);
    if(cdtextinfoblock->UPC_EAN_ISRC_code)
      free(cdtextinfoblock->UPC_EAN_ISRC_code);

    if(cdtextinfoblock->block_encoding_text)
      free(cdtextinfoblock->block_encoding_text);
  }

  return 0;
}

int free_cdtext_info(struct cdtext_info* cdtextinfo)
{
  int i;
  printf("CDTEXT INFO: free_cdtext_info() called\n");
  if(cdtextinfo)
  {
    for(i = 0; i < 8; i++)
    {
      if(cdtextinfo->blocks[i])
      {
        free_cdtext_info_block(cdtextinfo->blocks[i]);
      }
    }
  }

  return 0;
}

struct cdtext_info_block* malloc_cdtext_info_block(int count_of_tracks)
{
  int memamount;
  struct cdtext_info_block* lp_block;

  lp_block = malloc(sizeof(struct cdtext_info_block));
  if(!lp_block)
  {
    return (struct cdtext_info_block*)0;
  }
  memset(lp_block, 0, sizeof(struct cdtext_info_block));

  memamount = count_of_tracks * sizeof(cdtext_string);

  lp_block->name = malloc(memamount);
  if(!lp_block->name)
    return (struct cdtext_info_block*)free_cdtext_info_block(lp_block);
  memset(lp_block->name, 0, memamount);

  lp_block->performer = malloc(memamount);
  if(!lp_block->performer)
    return (struct cdtext_info_block*)free_cdtext_info_block(lp_block);
  memset(lp_block->performer, 0, memamount);

  lp_block->songwriter = malloc(memamount);
  if(!lp_block->songwriter)
    return (struct cdtext_info_block*)free_cdtext_info_block(lp_block);
  memset(lp_block->songwriter, 0, memamount);

  lp_block->composer = malloc(memamount);
  if(!lp_block->composer)
    return (struct cdtext_info_block*)free_cdtext_info_block(lp_block);
  memset(lp_block->composer, 0, memamount);

  lp_block->arranger = malloc(memamount);
  if(!lp_block->arranger)
    return (struct cdtext_info_block*)free_cdtext_info_block(lp_block);
  memset(lp_block->arranger, 0, memamount);

  lp_block->message = malloc(memamount);
  if(!lp_block->message)
    return (struct cdtext_info_block*)free_cdtext_info_block(lp_block);
  memset(lp_block->message, 0, memamount);
		
  lp_block->UPC_EAN_ISRC_code = malloc(memamount);
  if(!lp_block->UPC_EAN_ISRC_code)
    return (struct cdtext_info_block*)free_cdtext_info_block(lp_block);
  memset(lp_block->UPC_EAN_ISRC_code, 0, memamount);

  return lp_block;
}

void get_data_from_cdtext_pack(
  const struct cdtext_pack_data_header *pack,
  const struct cdtext_pack_data_header *pack_previous,
  cdtext_string *p_componente)
{

  int arr = pack->header_field_id2_tracknumber;
  int i;
  int language_block;
  int unicode;

  language_block = (pack->header_field_id4_block_no >> 4) & 0x07;
  unicode = pack->header_field_id4_block_no & 0x80;

  if(!unicode)
  {
    for(i = 0; i < DATAFIELD_LENGHT_IN_PACK; i++)
    {
      if(pack->text_data_field[i] == 0x00) /* end marker */
      {
        arr++;
      }
      else if(pack->text_data_field[i] == 0x09) /* repeat last marker */
      {
        /* ASSERT(arr > 0) */
        strcat((char*)(p_componente[arr]), (char*)(p_componente[arr-1]));
        arr++;
      }
      else
      {
      	strncat((char*)(p_componente[arr]), (char*)(&(pack->text_data_field[i])), 1);
      }
    }
  }
#ifdef libunicode
  else /* doublebytes ;-) */
  {
    for(i = 0; i < DATAFIELD_LENGHT_IN_PACK; i += 2)
    {
      if((Uchar)(pack->text_data_field[i]) == 0x0000) /* end marker */
      {
        arr++;
      }
      else if((Uchar)(pack->text_data_field[i]) == 0x0909) /* repeat last marker */
      {
        /* ASSERT(arr > 0) */
        strcat((char*)(p_componente[arr]), (char*)(p_componente[arr-1]));
        arr++;
      }
      else
      {
        strncat((char*)(p_componente[arr]), u_uc_to_utf8((Uchar*)(&(pack->text_data_field[i]))), 1);
      }
    }
  }
#else
  else {
     fprintf( stderr, "can't handle unicode"); 
  }
#endif
}

int wm_get_cdtext(struct wm_drive *d)
{
  /* alloc cdtext_info */

  unsigned char *buffer;
  int buffer_lenght;
  int ret;
  int i;
  struct cdtext_pack_data_header *pack, *pack_previous;
  cdtext_string *p_componente;
  struct cdtext_info_block *lp_block;

  if(1 == first_initialise)
  {
    memset(&wm_cdtext_info, 0, sizeof(struct cdtext_info));
    first_initialise = 0;
  }

  lp_block = 0;
  p_componente = 0;
  buffer = 0;
  buffer_lenght = 0;

  ret = (d->get_cdtext)(d, &buffer, &buffer_lenght);

  free_cdtext_info(&wm_cdtext_info);
  memset(&wm_cdtext_info, 0, sizeof(struct cdtext_info));
  if(!ret)
  {
    (d->get_trackcount)(d, &(wm_cdtext_info.count_of_entries));
    if( wm_cdtext_info.count_of_entries < 0 )
      wm_cdtext_info.count_of_entries = 1;
    else
      wm_cdtext_info.count_of_entries++;

    i = 0;

    pack = 0; /* NULL pointer*/
    while(i < buffer_lenght)
    {
      pack_previous = pack;
      pack = (struct cdtext_pack_data_header*)(buffer+i);
      /* to implement: check_crc(pack); */

      /* only for valid packs */
      if(pack->header_field_id1_typ_of_pack >= 0x80 && pack->header_field_id1_typ_of_pack < 0x90)
      {
        int code, j;
          printf("CDTEXT DEBUG: valid packet at 0x%08X: 0x %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
            i,
            pack->header_field_id1_typ_of_pack,
            pack->header_field_id2_tracknumber,
            pack->header_field_id3_sequence,
            pack->header_field_id4_block_no,
            pack->text_data_field[0],
            pack->text_data_field[1],
            pack->text_data_field[2],
            pack->text_data_field[3],
            pack->text_data_field[4],
            pack->text_data_field[5],
            pack->text_data_field[6],
            pack->text_data_field[7],
            pack->text_data_field[8],
            pack->text_data_field[9],
            pack->text_data_field[10],
            pack->text_data_field[11],
            pack->crc_byte1,
            pack->crc_byte2);
        wm_cdtext_info.count_of_valid_packs++;

        code = (pack->header_field_id4_block_no >> 4) & 0x07;
        if(0 == lp_block || lp_block->block_code != code) /* find or create one new block */
        {
          lp_block = 0;
          for(j = 0; j < MAX_LANGUAGE_BLOCKS && wm_cdtext_info.blocks[j] != 0 && 0 == lp_block; i++)
          {
            if(wm_cdtext_info.blocks[j]->block_code == code)
            {
              lp_block = wm_cdtext_info.blocks[j];
            }
          }

          if(MAX_LANGUAGE_BLOCKS <= j)
          {
            free_cdtext_info(&wm_cdtext_info);
            printf("CDTEXT ERROR: more as 8 languageblocks defined\n");
            return -1;
          }

          if(0 == lp_block)
          {
            /* make next new block */
            lp_block = malloc_cdtext_info_block(wm_cdtext_info.count_of_entries);
            if(0 == lp_block)
            {
              printf("CDTEXT ERROR: out of memory, can't create a new language block\n");
              free_cdtext_info(&wm_cdtext_info);
              return ENOMEM;
            }
            else
            {
              wm_cdtext_info.blocks[j] = lp_block;
              wm_cdtext_info.blocks[j]->block_code = code;
              wm_cdtext_info.blocks[j]->block_unicode = pack->header_field_id4_block_no & 0x80;
              printf("CDTEXT INFO: created a new language block; code %i, %s characters\n", code, lp_block->block_unicode?"doublebyte":"singlebyte");
/*
  unsigned char block_encoding; not jet!
  cdtext_string* block_encoding_text;
*/
            }
          }
        }
      }

      switch(pack->header_field_id1_typ_of_pack)
      {
        case 0x80:
          p_componente = (lp_block->name);
          get_data_from_cdtext_pack(pack, pack_previous, p_componente);
          break;
        case 0x81:
          p_componente = (lp_block->performer);
          get_data_from_cdtext_pack(pack, pack_previous, p_componente);
          break;
        case 0x82:
          p_componente = (lp_block->songwriter);
          get_data_from_cdtext_pack(pack, pack_previous, p_componente);
          break;
        case 0x83:
          p_componente = (lp_block->composer);
          get_data_from_cdtext_pack(pack, pack_previous, p_componente);
          break;
        case 0x84:
          p_componente = (lp_block->arranger);
          get_data_from_cdtext_pack(pack, pack_previous, p_componente);
          break;
        case 0x85:
          p_componente = (lp_block->message);
          get_data_from_cdtext_pack(pack, pack_previous, p_componente);
          break;
        case 0x86:
          memcpy((char*)(lp_block->binary_disc_identification_info),
          (char*)(pack->text_data_field),  DATAFIELD_LENGHT_IN_PACK);
          break;
        case 0x87:
          memcpy((char*)(lp_block->binary_genreidentification_info),
          (char*)(pack->text_data_field),  DATAFIELD_LENGHT_IN_PACK);
          break;
        case 0x88:
         printf("CDTEXT INFO: PACK with code 0x88 (TOC)\n");
          break;
        case 0x89:
         printf("CDTEXT INFO: PACK with code 0x89 (second TOC)\n");
          break;
        case 0x8A:
        case 0x8B:
        case 0x8C:
          printf("CDTEXT INFO: PACK with code 0x%02X (reserved)\n", pack->header_field_id1_typ_of_pack);
          break;
        case 0x8D:
          printf("CDTEXT INFO: PACK with code 0x8D (for content provider only)\n");
          break;
        case 0x8E:
          p_componente = (lp_block->UPC_EAN_ISRC_code);
          get_data_from_cdtext_pack(pack, pack_previous, p_componente);
          break;
        case 0x8F:
          memcpy((char*)(lp_block->binary_size_information),
          (char*)(pack->text_data_field), DATAFIELD_LENGHT_IN_PACK);
          break;
        default:
          printf("CDTEXT ERROR: invalid packet at 0x%08X: 0x %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
            i,
            pack->header_field_id1_typ_of_pack,
            pack->header_field_id2_tracknumber,
            pack->header_field_id3_sequence,
            pack->header_field_id4_block_no,
            pack->text_data_field[0],
            pack->text_data_field[1],
            pack->text_data_field[2],
            pack->text_data_field[3],
            pack->text_data_field[4],
            pack->text_data_field[5],
            pack->text_data_field[6],
            pack->text_data_field[7],
            pack->text_data_field[8],
            pack->text_data_field[9],
            pack->text_data_field[10],
            pack->text_data_field[11],
            pack->crc_byte1,
            pack->crc_byte2);
          wm_cdtext_info.count_of_invalid_packs++;
      }
      i += sizeof(struct cdtext_pack_data_header);
    } /* while */
  }

  if(0 == ret && wm_cdtext_info.count_of_valid_packs > 0)
    wm_cdtext_info.valid = 1;

  return ret;
}

void wm_free_cdtext(void)
{
  printf("CDTEXT INFO: wm_free_cdtext() called, this function will be called at the end from USERSPACE(KSCD etc.)\n");
  if (0 == first_initialise)
    free_cdtext_info(&wm_cdtext_info);

  first_initialise = 1;	
}
