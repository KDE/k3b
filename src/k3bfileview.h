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


#ifndef K3BFILEVIEW_H
#define K3BFILEVIEW_H


#include "k3bcontentsview.h"


class K3bDirOperator;
class QDragObject;
class KURL;
class KFileFilterCombo;
class KFileItem;
class KActionCollection;
class KConfig;
class K3bToolBox;


/**
  *@author Sebastian Trueg
  */
class K3bFileView : public K3bContentsView
{
  Q_OBJECT

 public:
  K3bFileView(QWidget *parent=0, const char *name=0);
  ~K3bFileView();

  void setUrl( const KURL &url, bool forward = true );
  KURL url();

  KActionCollection* actionCollection() const;

  void reload();

 signals:
  void urlEntered( const KURL& url );

 public slots:
  void setDir( const QString& );
  void saveConfig( KConfig* c );
  void readConfig( KConfig* c );
  void setAutoUpdate( bool );

 private:
  K3bToolBox* m_toolBox;
  K3bDirOperator* m_dirOp;
  KFileFilterCombo* m_filterWidget;

  void setupGUI();

 private slots:
  void slotFilterChanged();
  void slotFileHighlighted( const KFileItem* item );
};


#endif
