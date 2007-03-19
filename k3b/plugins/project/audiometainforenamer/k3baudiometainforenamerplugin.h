/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_AUDIO_METAINFO_RENAMER_PLUGIN_H_
#define _K3B_AUDIO_METAINFO_RENAMER_PLUGIN_H_


#include <k3bprojectplugin.h>
#include <qwidget.h>


class K3bDataDoc;
class K3bDirItem;
class K3bFileItem;
class QListViewItem;


class K3bAudioMetainfoRenamerPluginWidget : public QWidget, public K3bProjectPluginGUIBase
{
  Q_OBJECT

 public:
  K3bAudioMetainfoRenamerPluginWidget( K3bDoc* doc, QWidget* parent = 0, const char* name = 0 );
  ~K3bAudioMetainfoRenamerPluginWidget();

  QWidget* qWidget() { return this; }

  QString title() const;
  QString subTitle() const;

  void loadDefaults();
  void readSettings( KConfigBase* );
  void saveSettings( KConfigBase* );

  void activate();

 private slots:
  void slotScanClicked();

 private:
  void scanDir( K3bDirItem*, QListViewItem* parent );
  QString createNewName( K3bFileItem* );
  bool existsOtherItemWithSameName( K3bFileItem*, const QString& );

  class Private;
  Private* d;
};


class K3bAudioMetainfoRenamerPlugin : public K3bProjectPlugin
{
  Q_OBJECT

 public:
  K3bAudioMetainfoRenamerPlugin( QObject* parent, const char* name );
  ~K3bAudioMetainfoRenamerPlugin();

  int pluginSystemVersion() const { return 3; }

  K3bProjectPluginGUIBase* createGUI( K3bDoc*, QWidget* = 0, const char* = 0 );
};


#endif
