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

#ifndef K3BWAVEMODULE_H
#define K3BWAVEMODULE_H


#include "../k3baudiomodule.h"

#include <kurl.h>
#include <qcstring.h>


class K3bAudioTrack;
class QFile;



class K3bWaveModule : public K3bAudioModule
{
  Q_OBJECT

 public:
  K3bWaveModule( QObject* parent = 0, const char* name = 0 );
  ~K3bWaveModule();

  bool canDecode( const KURL& url );

  /**
   * retrieve information about the track like the length
   */
  int analyseTrack( const QString& filename, unsigned long& size );

  void cleanup();

 protected:
  bool initDecodingInternal( const QString& filename );
  int decodeInternal( const char** _data );

 private:
  long wavSize( QFile* f );
  unsigned long identifyWaveFile( QFile* );

  static unsigned short le_a_to_u_short( unsigned char* a );
  static unsigned long le_a_to_u_long( unsigned char* a );

  QFile* m_file;
  QByteArray* m_data;
};


#endif
