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

#ifndef K3BAUDIOMODULE
#define K3BAUDIOMODULE


#include <qobject.h>

#include "../k3baudiotitlemetainfo.h"
#include <k3bmsf.h>

#include <kurl.h>


/**
 * Abstract streaming class for all the audio input.
 * Has to output data in the following format:
 * MSBLeft LSBLeft MSBRight LSBRight (big endian byte order)
 **/
class K3bAudioModule : public QObject
{
  Q_OBJECT

 public:
  K3bAudioModule( QObject* parent = 0, const char* name = 0 );
  virtual ~K3bAudioModule();

  virtual bool canDecode( const KURL& url ) = 0;

  /**
   * returnes K3bAudioTitleMetaInfo::FileStatus
   * at least size has to be set in all derived classes
   * TODO: remove this status stuff and let the method return the size.
   *      ev. ists doch besser, wieder ein modul pro track zu machen. So viel speicher ist
   *      das ja nun auch nicht. Dann ist die verwaltung nur wieder einfacher. Eine Factory könnte dann
   *      für jedes modul das canDecode übernehmen (so wie im plugin-system vorgesehen)
   *      Das hier sollte dann eher eine init methode sein. Danzu sollte es eine "K3b::Msf size()" methode
   *      geben. Metainfo benötigt dann auch keinen Dateinamen mehr. Der sollte auch über eine methode
   *      zugänglich sein. die init methode könnte dann bei mp3 den seek-index bauen.
   *      Auch wäre endlich mal eine generelle MAD-Wrapper klasse sinnvoll.
   */
  virtual int analyseTrack( const QString& filename, 
			    unsigned long& size ) = 0;

  /**
   * The default implementation uses KFileMetaInfo
   */
  virtual bool metaInfo( const QString&,
			 K3bAudioTitleMetaInfo& );

  bool initDecoding( const QString& filename, unsigned long trackSize );

  /**
   * returnes -1 on error, 0 when finished, length of data otherwise
   * takes care of padding
   * calls decodeInternal() to actually decode data
   */
  int decode( char* _data, int maxLen );

  /**
   * cleanup after decoding like closing files.
   */
  virtual void cleanup();

  /**
   * Seek to the position pos.
   * returnes true on success;
   */
  virtual bool seek( const K3b::Msf& pos ) = 0;

 protected:
  virtual bool initDecodingInternal( const QString& filename ) = 0;

  /**
   * fill the already allocated data with maximal maxLen bytes of decoded samples.
   */
  virtual int decodeInternal( char* data, int maxLen ) = 0;

 private:
  unsigned long m_size;
  unsigned long m_alreadyDecoded;
};


#endif
