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

#ifndef _K3B_EXTERNAL_ENCODER_H_
#define _K3B_EXTERNAL_ENCODER_H_


#include <k3baudioencoder.h>
#include <k3bpluginconfigwidget.h>


class base_K3bExternalEncoderConfigWidget;
class KInstance;
class KProcess;



class K3bExternalEncoderFactory : public K3bAudioEncoderFactory
{
  Q_OBJECT

 public:
  K3bExternalEncoderFactory( QObject* parent = 0, const char* name = 0 );
  ~K3bExternalEncoderFactory();

  QStringList extensions() const;
  
  QString fileTypeComment( const QString& ) const;

  //  long long fileSize( const QString&, const K3b::Msf& msf ) const;

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


class K3bExternalEncoder : public K3bAudioEncoder
{
  Q_OBJECT

 public:
  K3bExternalEncoder( QObject* parent = 0, const char* name = 0 );
  ~K3bExternalEncoder();

  /**
   * reimplemented since the external program is intended to write the file
   * TODO: allow writing to stdout.
   */
  bool openFile( const QString& ext, const QString& filename );
  void closeFile();

  class Command;

 private slots:
  void slotExternalProgramFinished( KProcess* );
  void slotExternalProgramOutputLine( const QString& );

 private:
  void finishEncoderInternal();
  bool initEncoderInternal( const QString& extension );
  long encodeInternal( const char* data, Q_ULONG len );
  void setMetaDataInternal( MetaDataField, const QString& );

  class Private;
  Private* d;
};


class K3bExternalEncoderSettingsWidget : public K3bPluginConfigWidget
{
  Q_OBJECT

 public:
  K3bExternalEncoderSettingsWidget( QWidget* parent = 0, const char* name = 0 );
  ~K3bExternalEncoderSettingsWidget();

 public slots:
  void loadConfig();
  void saveConfig();

 private slots:
  void slotHighlighted( int );
  void slotNewCommand();
  void slotDeleteCommand();
  void updateCurrentCommand();

 private:
  bool checkCurrentCommand();
  void loadCommand( int );

  base_K3bExternalEncoderConfigWidget* w;

  class Private;
  Private* d;
};


K_EXPORT_COMPONENT_FACTORY( libk3bexternalencoder, K3bExternalEncoderFactory )

#endif
