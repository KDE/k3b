/***************************************************************************
                          k3bvcdmpegfactory.cpp  -  description
                             -------------------
    begin                : Son Nov 24 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// TODO: more mpeg functions (Size, Frames ...), cleanup, FILE -> QFile

#include "k3bvcdmpegfactory.h"
#include "k3bvcdmpegfactory.moc"

// QT-includes
#include <qstring.h>
#include <qfile.h>

// KDE-includes
#include <kurl.h>
#include <kdebug.h>

int masks[8]={0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
static unsigned char bfr[BUFFER_SIZE];
static int byteidx;
static int bitidx;
static int bufcount;
static double totbits;
bool eobs;

K3bVcdMpegFactory::K3bVcdMpegFactory()
  : QObject()
{
}

K3bVcdMpegFactory::~K3bVcdMpegFactory()
{
}

unsigned int K3bVcdMpegFactory::getMpegFileType(const KURL& url)
{
  unsigned long i;
  bool mpeg2;
  int mpeg_type; // 0 = none, 1 = program stream, 2 = transport stream
  init_getbits(url.path());

  mpeg_type = 0;
  while (!mpeg_type) {
    if (look_ahead(32) == PACK_START_CODE) {
      mpeg_type = 1;
      i = getbits(32);
      if (look_ahead(2) == 1) {
        kdDebug() << QString("File %1 is an MPEG-2 Program Stream").arg(url.path()) << endl;
        return 2;
      }
      else {
        kdDebug() << QString("File %1 is an MPEG-1 Program Stream").arg(url.path()) << endl;
        return 1;
      }
    }
    else
      if (look_ahead(8) == TRANSPORT_SYNC_BYTE) {
        kdDebug() << QString("File %1 is an MPEG-2 Transport Stream").arg(url.path()) << endl;
        mpeg_type = 2;
      }
      else
        getbits(8);
  }
  if (!mpeg_type) {
    kdDebug() << "Unknown file type.";
    return 0;
  }
  return 0;
}

/* return total number of generated bits */
double K3bVcdMpegFactory::bitcount()
{
  return totbits;
}

bool K3bVcdMpegFactory::refill_buffer()
{
  int i;

  i = fread(bfr, sizeof(unsigned char), BUFFER_SIZE, bitfile);
  if (i <= 0)
  {
    eobs = true;
    return false;
  }
  bufcount = i;
  return true;
}

/* open the device to read the bit stream from it */
void K3bVcdMpegFactory::init_getbits(const QString bs_filename)
{
  if ((bitfile = fopen(QFile::encodeName(bs_filename), "rb")) == NULL) {
    kdDebug() << QString("Unable to open file %1 for reading.").arg(bs_filename) << endl;
    return;
  }
  byteidx = 0;
  bitidx = 8;
  totbits = 0.0;
  bufcount = 0;
  eobs = false;
  if (!refill_buffer()) {
    if (eobs) {
      kdDebug() << QString("Unable to read from file %1.").arg(bs_filename) << endl;
      return;
    }
  }
}

/*close the device containing the bit stream after a read process*/
void K3bVcdMpegFactory::finish_getbits()
{
  if (bitfile)
    fclose(bitfile);
  bitfile = NULL;
}

/*read 1 bit from the bit stream */
unsigned int K3bVcdMpegFactory::get1bit()
{
  unsigned int bit;

  if (eobs)
    return 0;

  bit = (bfr[byteidx] & masks[bitidx - 1]) >> (bitidx - 1);
  totbits++;
  bitidx--;
  if (!bitidx) {
    bitidx = 8;
    byteidx++;
    if (byteidx == bufcount) {
      if (bufcount == BUFFER_SIZE)
        refill_buffer();
      else
        eobs = true;
      byteidx = 0;
    }
  }
  return bit;
}

/*read N bit from the bit stream */
unsigned int K3bVcdMpegFactory::getbits(int N)
{
 unsigned int val = 0;
 int i = N;
 unsigned int j;

 // Optimize: we are on byte boundary and want to read multiple of bytes!
 if ((bitidx == 8) && ((N & 7) == 0)) {
   i = N >> 3;
   while (i > 0) {
     val = (val << 8) | bfr[byteidx];
     byteidx++;
     totbits += 8;
     if (byteidx == bufcount) {
       if (bufcount == BUFFER_SIZE)
         refill_buffer();
       else
         eobs = true;
       byteidx = 0;
     }
     i--;
   }
   return val;
 }

 while (i > 0) {
   j = get1bit();
   val = (val << 1) | j;
   i--;
 }
 return val;
}

/*return the status of the bit stream*/
/* returns 1 if end of bit stream was reached */
/* returns 0 if end of bit stream was not reached */

int K3bVcdMpegFactory::end_bs()
{
  return eobs;
}

/*this function seeks for a byte aligned sync word (max 32 bits) in the bit stream and
  places the bit stream pointer right after the sync.
  This function returns 1 if the sync was found otherwise it returns 0  */

int K3bVcdMpegFactory::seek_sync(unsigned int sync, int N)
{
  unsigned int val;
  unsigned int maxi = (int)pow(2.0, (double)N) - 1;

  while (bitidx != 8)
    get1bit();

  val = getbits(N);

  while ((val & maxi) != sync) {
    if (eobs)
      return 0;
    val <<= 8;
    val |= getbits(8);
  }
  return 1;
}

/*look ahead for the next N bits from the bit stream */
unsigned int K3bVcdMpegFactory::look_ahead(int N)
{
  unsigned long val = 0;
  unsigned char *tmp_bfr = bfr;
  unsigned char tmp_bfr1[4];
  int j = N;
  int eo_bs = eobs;
  int buf_count = bufcount;
  int bit_idx = bitidx;
  int byte_idx = byteidx;

  while (j > 0) {
    if (eo_bs)
      return 0;
    val <<= 1;
    val |= (tmp_bfr[byte_idx] & masks[bit_idx - 1]) >> (bit_idx - 1);
    bit_idx--;
    j--;
    if (!bit_idx) {
      bit_idx = 8;
      byte_idx++;
      if (byte_idx == buf_count) {
        if (buf_count == BUFFER_SIZE) {
          if (fread(tmp_bfr1, sizeof(unsigned char), 4, bitfile) != 4)
            eo_bs = true;
          else
            tmp_bfr = &tmp_bfr1[0];
          if (fseek(bitfile, -4, SEEK_CUR)) {
            kdDebug() << "Unable to set file position." << endl;
            return 0;
          }
        }
        else
          eo_bs = true;
        byte_idx = 0;
      }
    }
  }
  return val;
}
