/* 
 *
 * $Id: $
 * Copyright (C) 2004 Matthieu Bedouet <mbedouet@no-log.org>
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

#ifndef _K3B_AIFF_DECODER_H_
#define _K3B_AIFF_DECODER_H_

#include <k3baudiodecoder.h>

class KURL;


class K3bLibsndfileDecoderFactory : public K3bAudioDecoderFactory
{
  Q_OBJECT

 public:
  K3bLibsndfileDecoderFactory( QObject* parent = 0, const char* name = 0 );
  ~K3bLibsndfileDecoderFactory();

  bool canDecode( const KURL& filename );

  int pluginSystemVersion() const { return 2; }

  K3bPlugin* createPluginObject( QObject* parent = 0, 
				 const char* name = 0,
				 const QStringList& = QStringList() );

 private:
  KInstance* s_instance;
};


class K3bLibsndfileDecoder : public K3bAudioDecoder
{
  Q_OBJECT

 public:
  K3bLibsndfileDecoder( QObject* parent = 0, const char* name = 0 );
  ~K3bLibsndfileDecoder();
  void cleanup();
  QString fileType() const;

 protected:
  bool analyseFileInternal( K3b::Msf& frames, int& samplerate, int& ch );
  bool initDecoderInternal();
  bool seekInternal( const K3b::Msf& );

  int decodeInternal( char* _data, int maxLen );
 
 private:
  bool openFile();

  class Private;
  Private* d;
  
};

K_EXPORT_COMPONENT_FACTORY( libk3blibsndfiledecoder, K3bLibsndfileDecoderFactory )

#endif
