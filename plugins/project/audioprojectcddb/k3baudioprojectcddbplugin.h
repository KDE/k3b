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

#ifndef _K3B_AUDIO_PROJECT_CDDB_PLUGIN_H_
#define _K3B_AUDIO_PROJECT_CDDB_PLUGIN_H_


#include <k3bprojectplugin.h>

class K3bCddb;
class K3bAudioDoc;
class K3bProgressDialog;
class QWidget;

class K3bAudioProjectCddbPlugin : public K3bProjectPlugin
{
  Q_OBJECT

 public:
  K3bAudioProjectCddbPlugin( QObject* parent, const char* name );
  ~K3bAudioProjectCddbPlugin();

  int pluginSystemVersion() const { return 3; }

  void activate( K3bDoc* doc, QWidget* parent );

 private slots:
  void slotCddbQueryFinished( int result );
  void slotCancelClicked();

 private:
  K3bCddb* m_cddb;
  K3bAudioDoc* m_doc;
  K3bProgressDialog* m_progress;
  QWidget* m_parentWidget;

  bool m_canceled;
};


#endif
