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

#ifndef _K3B_AUDIO_CONVERTER_PLUGIN_H_
#define _K3B_AUDIO_CONVERTER_PLUGIN_H_


#include <kparts/plugin.h>
#include <klibloader.h>
#include <kurl.h>

#include <k3binteractiondialog.h>

class base_K3bAudioConverterWidget;


class K3bAudioConverterPluginDialog : public K3bInteractionDialog
{
  Q_OBJECT

 public:
  K3bAudioConverterPluginDialog( QWidget* parent = 0, const char* name = 0 );
  ~K3bAudioConverterPluginDialog();

 public slots:
  void addFile( const KURL& url );

 protected slots:
  void slotLoadK3bDefaults();
  void slotLoadUserDefaults();
  void slotSaveUserDefaults();
  void slotStartClicked();

  void slotAddFiles();
  void slotRemove();
  void slotClear();
  void slotConfigureEncoder();
  void slotToggleAll();

 private:
  void loadAudioEncoder();

  base_K3bAudioConverterWidget* m_w;

  class Private;
  Private* d;
};


class K3bAudioConverterPlugin : public KParts::Plugin
{
  Q_OBJECT

 public:
  K3bAudioConverterPlugin( QObject* parent, const char* name, const QStringList& );
  virtual ~K3bAudioConverterPlugin();

 public slots:
  void slotConvert();
};


#endif
