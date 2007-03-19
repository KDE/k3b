/* 
 * FLAC decoder module for K3b.
 * Based on the Ogg Vorbis module for same.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2003 John Steele Scott <toojays@toojays.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_FLAC_DECODER_H_
#define _K3B_FLAC_DECODER_H_


#include <k3baudiodecoder.h>

class KURL;


class K3bFLACDecoderFactory : public K3bAudioDecoderFactory
{
  Q_OBJECT

 public:
  K3bFLACDecoderFactory( QObject* parent = 0, const char* name = 0 );
  ~K3bFLACDecoderFactory();

  bool canDecode( const KURL& filename );

  int pluginSystemVersion() const { return 3; }

  K3bAudioDecoder* createDecoder( QObject* parent = 0, 
				  const char* name = 0 ) const;
};


class K3bFLACDecoder : public K3bAudioDecoder
{
  Q_OBJECT

 public: 
  K3bFLACDecoder( QObject* parent = 0, const char* name = 0 );
  ~K3bFLACDecoder();

  void cleanup();

  bool seekInternal( const K3b::Msf& );

  QString fileType() const;
  QStringList supportedTechnicalInfos() const;
  QString technicalInfo( const QString& ) const;

 protected:
  bool analyseFileInternal( K3b::Msf& frames, int& samplerate, int& ch );
  bool initDecoderInternal();

  int decodeInternal( char* _data, int maxLen );

 private:
  class Private;
  Private* d;
};

#endif
