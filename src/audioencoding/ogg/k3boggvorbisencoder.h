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

#ifndef _K3B_OGG_VORBIS_ENCODER_H_
#define _K3B_OGG_VORBIS_ENCODER_H_


#include <k3baudioencoder.h>
#include <k3bpluginconfigwidget.h>


class base_K3bOggVorbisEncoderSettingsWidget;
class KInstance;


class K3bOggVorbisEncoderFactory : public K3bAudioEncoderFactory
{
  Q_OBJECT

 public:
  K3bOggVorbisEncoderFactory( QObject* parent = 0, const char* name = 0 );
  ~K3bOggVorbisEncoderFactory();

  QStringList extensions() const { return QStringList("ogg"); }
  
  QString fileTypeComment( const QString& ) const;

  long long fileSize( const QString&, const K3b::Msf& msf ) const;

  int pluginSystemVersion() const { return 1; }

  K3bPlugin* createPluginObject( QObject* parent = 0, 
				 const char* name = 0,
				 const QStringList& = QStringList() );
  K3bPluginConfigWidget* createConfigWidgetObject( QWidget* parent = 0, 
						   const char* name = 0,
						   const QStringList &args = QStringList() );

 private:
  KInstance* s_instance;
};


class K3bOggVorbisEncoder : public K3bAudioEncoder
{
  Q_OBJECT

 public:
  K3bOggVorbisEncoder( QObject* parent = 0, const char* name = 0 );
  ~K3bOggVorbisEncoder();

 private:
  void loadConfig();
  void finishEncoderInternal();
  bool initEncoderInternal( const QString& extension );
  long encodeInternal( const char* data, Q_ULONG len );
  void setMetaDataInternal( MetaDataField, const QString& );

  bool writeOggHeaders();
  void cleanup();
  long flushVorbis();

  class Private;
  Private* d;
};


class K3bOggVorbisEncoderSettingsWidget : public K3bPluginConfigWidget
{
  Q_OBJECT

 public:
  K3bOggVorbisEncoderSettingsWidget( QWidget* parent = 0, const char* name = 0 );
  ~K3bOggVorbisEncoderSettingsWidget();

 public slots:
  void loadConfig();
  void saveConfig();

 private:
  base_K3bOggVorbisEncoderSettingsWidget* w;
};


K_EXPORT_COMPONENT_FACTORY( libk3boggvorbisencoder, K3bOggVorbisEncoderFactory )

#endif
