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

#ifndef K3BMP3MODULE_H
#define K3BMP3MODULE_H


#include <k3baudiodecoder.h>

extern "C" {
#include <mad.h>
}


class K3bMadDecoderFactory : public K3bAudioDecoderFactory
{
  Q_OBJECT

 public:
  K3bMadDecoderFactory( QObject* parent = 0, const char* name = 0 );
  ~K3bMadDecoderFactory();

  bool canDecode( const KURL& filename );

  int pluginSystemVersion() const { return 1; }

  K3bPlugin* createPluginObject( QObject* parent = 0, 
				 const char* name = 0,
				 const QStringList& = QStringList() );

 private:
  KInstance* s_instance;
};


class K3bMadDecoder : public K3bAudioDecoder
{
  Q_OBJECT

 public:
  K3bMadDecoder( QObject* parent = 0, const char* name = 0 );
  ~K3bMadDecoder();

  QString metaInfo( MetaDataField );

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
  void initMadStructures();
  unsigned long countFrames();
  inline unsigned short linearRound( mad_fixed_t fixed );
  void madStreamBuffer();
  bool madDecodeNextFrame();
  bool decodeNextHeader();
  bool createPcmSamples( mad_synth* );

  static const int INPUT_BUFFER_SIZE = 5*8192;

  static int MaxAllowedRecoverableErrors;

  class Private;
  Private* d;
};


K_EXPORT_COMPONENT_FACTORY( libk3bmaddecoder, K3bMadDecoderFactory )

#endif
