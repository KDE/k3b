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

#ifndef _K3B_AUDIO_METAINFO_RENAMER_PLUGIN_H_
#define _K3B_AUDIO_METAINFO_RENAMER_PLUGIN_H_


#include <kparts/plugin.h>
#include <klibloader.h>

#include <k3binteractiondialog.h>


class K3bDataDoc;
class K3bDirItem;
class K3bFileItem;
class QListViewItem;


class K3bAudioMetainfoRenamerPluginDialog : public K3bInteractionDialog
{
  Q_OBJECT

 public:
  K3bAudioMetainfoRenamerPluginDialog( K3bDataDoc* doc, QWidget* parent = 0, const char* name = 0 );
  ~K3bAudioMetainfoRenamerPluginDialog();

 protected slots:
  void slotLoadK3bDefaults();
  void slotLoadUserDefaults();
  void slotSaveUserDefaults();
  void slotStartClicked();
  void slotSaveClicked();

 private:
  void scanDir( K3bDirItem*, QListViewItem* parent );
  QString createNewName( K3bFileItem* );
  bool find( K3bDirItem*, const QString& );

  class Private;
  Private* d;
};


class K3bAudioMetainfoRenamerPlugin : public KParts::Plugin
{
  Q_OBJECT

 public:
  K3bAudioMetainfoRenamerPlugin( QObject* parent, const char* name, const QStringList& );
  virtual ~K3bAudioMetainfoRenamerPlugin();

 public slots:
  void slotDoRename();
};


#endif
