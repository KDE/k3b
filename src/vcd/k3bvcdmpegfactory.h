/***************************************************************************
                          k3bvcdmpegfactory.h  -  description
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

#ifndef K3BVCDMPEGFACTROY_H
#define K3BVCDMPEGFACTROY_H

#include <qstring.h>
#include <qfile.h>
#include <qobject.h>
#include <math.h>

#define PACK_START_CODE          0x000001BA
#define TRANSPORT_SYNC_BYTE      0x47
#define BUFFER_SIZE 1024*1024

class KURL;

class K3bVcdMpegFactory : public QObject
{
  Q_OBJECT

 public:
  ~K3bVcdMpegFactory();
    
  public:
  unsigned int getMpegFileType(const KURL&);
  static K3bVcdMpegFactory* self();
   
  private:
  K3bVcdMpegFactory();
  
  FILE* bitfile;
  // QFile* bitfile;

  void init_getbits(const QString);
  void finish_getbits();
  unsigned int get1bit();
  unsigned int getbits(int);
  double bitcount();
  int end_bs();
  int seek_sync(unsigned int, int);
  unsigned int look_ahead(int);
  bool refill_buffer();

};

#endif
