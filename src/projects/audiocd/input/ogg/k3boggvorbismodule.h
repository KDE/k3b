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


#ifndef K3BOGGVORBISMODULE_H
#define K3BOGGVORBISMODULE_H

#include <config.h>

#ifdef OGG_VORBIS

#include "../k3baudiomodule.h"

class OggVorbis_File;
class KURL;


/**
  *@author Sebastian Trueg
  */
class K3bOggVorbisModule : public K3bAudioModule
{
  Q_OBJECT

 public: 
  K3bOggVorbisModule( QObject* parent = 0, const char* name = 0 );
  ~K3bOggVorbisModule();

  bool canDecode( const KURL& url );
  int analyseTrack( const QString& filename, unsigned long& size );
  bool metaInfo( const QString& filename, K3bAudioTitleMetaInfo& );

  void cleanup();

  bool seek( const K3b::Msf& );

 protected:
  bool initDecodingInternal( const QString& filename );
  int decodeInternal( char* _data, int maxLen );

 private:
  OggVorbis_File* m_oggVorbisFile;
};

#endif

#endif
