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

#ifndef _K3B_LAME_ENCODER_H_
#define _K3B_LAME_ENCODER_H_


#include <k3baudioencoder.h>
#include <k3bpluginconfigwidget.h>

#include <base_k3blameencodersettingswidget.h>

class KInstance;


class K3bLameEncoderFactory : public K3bAudioEncoderFactory
{
  Q_OBJECT

 public:
  K3bLameEncoderFactory( QObject* parent = 0, const char* name = 0 );
  ~K3bLameEncoderFactory();

  QStringList extensions() const;
  
  QString fileTypeComment( const QString& ) const;

  long long fileSize( const QString&, const K3b::Msf& msf ) const;

  int pluginSystemVersion() const { return 2; }

  K3bPlugin* createPluginObject( QObject* parent = 0, 
				 const char* name = 0,
				 const QStringList& = QStringList() );
  K3bPluginConfigWidget* createConfigWidgetObject( QWidget* parent = 0, 
						   const char* name = 0,
						   const QStringList &args = QStringList() );

 private:
  KInstance* s_instance;
};


class K3bLameEncoder : public K3bAudioEncoder
{
  Q_OBJECT

 public:
  K3bLameEncoder( QObject* parent = 0, const char* name = 0 );
  ~K3bLameEncoder();

 private:
  void finishEncoderInternal();
  bool initEncoderInternal( const QString& extension, const K3b::Msf& length );
  long encodeInternal( const char* data, Q_ULONG len );
  void setMetaDataInternal( MetaDataField, const QString& );

  class Private;
  Private* d;
};


class K3bLameEncoderSettingsWidget : public K3bPluginConfigWidget
{
  Q_OBJECT

 public:
  K3bLameEncoderSettingsWidget( QWidget* parent = 0, const char* name = 0 );
  ~K3bLameEncoderSettingsWidget();

 public slots:
  void loadConfig();
  void saveConfig();

 private slots:
  void slotToggleCbrVbr();

 private:
  base_K3bLameEncoderSettingsWidget* m_w;
};


K_EXPORT_COMPONENT_FACTORY( libk3blameencoder, K3bLameEncoderFactory )

#endif
