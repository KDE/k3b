/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_WAVE_DECODER_H_
#define _K3B_WAVE_DECODER_H_

#include <k3baudiodecoder.h>
#include <k3b_export.h>

#include <kurl.h>
#include <qcstring.h>


class QFile;


class LIBK3B_EXPORT K3bWaveDecoderFactory : public K3bAudioDecoderFactory
{
  Q_OBJECT

 public:
  K3bWaveDecoderFactory( QObject* parent = 0, const char* name = 0 );
  ~K3bWaveDecoderFactory();

  bool canDecode( const KURL& filename );

  int pluginSystemVersion() const { return 3; }

  K3bAudioDecoder* createDecoder( QObject* parent = 0, 
				  const char* name = 0 ) const;
};


class LIBK3B_EXPORT K3bWaveDecoder : public K3bAudioDecoder
{
  Q_OBJECT

 public:
  K3bWaveDecoder( QObject* parent = 0, const char* name = 0 );
  ~K3bWaveDecoder();

  void cleanup();

  bool seekInternal( const K3b::Msf& );

  QString fileType() const;

  QStringList supportedTechnicalInfos() const;

  QString technicalInfo( const QString& ) const;

 protected:
  bool analyseFileInternal( K3b::Msf& frames, int& samplerate, int& channels );
  bool initDecoderInternal();
  int decodeInternal( char* data, int maxLen );

 private:
  class Private;
  Private* d;
};

#endif
