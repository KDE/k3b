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

#include <tools/k3baudiotitlemetainfo.h>

#include <qcstring.h>

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
   */
  virtual int analyseTrack( const QString& filename, 
			    unsigned long& size, 
			    K3bAudioTitleMetaInfo& info ) = 0;

  bool initDecoding( const QString& filename, unsigned long trackSize );

  /**
   * returnes -1 on error, 0 when finished, length of data otherwise
   * takes care of padding
   * calls decodeInternal() to actually decode data
   */
  int decode( const char** _data );

  /**
   * cleanup after decoding like closing files.
   */
  virtual void cleanup();

 protected:
  virtual bool initDecodingInternal( const QString& filename ) = 0;
  virtual int decodeInternal( const char** data ) = 0;

 private:
  unsigned long m_size;
  unsigned long m_alreadyDecoded;
  QByteArray m_data;
};


#endif
