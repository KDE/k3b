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

#ifndef _K3B_SOX_ENCODER_H_
#define _K3B_SOX_ENCODER_H_


#include <k3baudioencoder.h>
#include <k3bpluginconfigwidget.h>


class base_K3bSoxEncoderConfigWidget;
class KInstance;
class KProcess;



class K3bSoxEncoderFactory : public K3bAudioEncoderFactory
{
  Q_OBJECT

 public:
  K3bSoxEncoderFactory( QObject* parent = 0, const char* name = 0 );
  ~K3bSoxEncoderFactory();

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


class K3bSoxEncoder : public K3bAudioEncoder
{
  Q_OBJECT

 public:
  K3bSoxEncoder( QObject* parent = 0, const char* name = 0 );
  ~K3bSoxEncoder();

  /**
   * reimplemented since sox writes the file itself
   */
  bool openFile( const QString& ext, const QString& filename );
  void closeFile();

 private slots:
  void slotSoxFinished( KProcess* );
  void slotSoxOutputLine( const QString& );

 private:
  void finishEncoderInternal();
  bool initEncoderInternal( const QString& extension );
  long encodeInternal( const char* data, Q_ULONG len );

  class Private;
  Private* d;
};


class K3bSoxEncoderSettingsWidget : public K3bPluginConfigWidget
{
  Q_OBJECT

 public:
  K3bSoxEncoderSettingsWidget( QWidget* parent = 0, const char* name = 0 );
  ~K3bSoxEncoderSettingsWidget();

 public slots:
  void loadConfig();
  void saveConfig();

 private:
  base_K3bSoxEncoderConfigWidget* w;
};


K_EXPORT_COMPONENT_FACTORY( libk3bsoxencoder, K3bSoxEncoderFactory )

#endif
