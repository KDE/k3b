/***************************************************************************
                          k3bfileview.h  -  description
                             -------------------
    begin                : Sun Oct 28 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BFILEVIEW_H
#define K3BFILEVIEW_H


#include "k3bcdcontentsview.h"


class KDirOperator;
class QDragObject;
class KURL;
class KFileFilterCombo;
class KFileItem;
class KActionCollection;
class KConfig;


/**
  *@author Sebastian Trueg
  */
class K3bFileView : public K3bCdContentsView
{
  Q_OBJECT

 public: 
  K3bFileView(QWidget *parent=0, const char *name=0);
  ~K3bFileView();
  void setUrl(const KURL &url, bool forward);

  KActionCollection* actionCollection() const;

  void reload();

 signals:
  void urlEntered( const KURL& url );

 public slots:
  void slotAudioFilePlay();
  void slotAudioFileEnqueue();
  void slotAddFilesToProject();

 private:
  class PrivateFileView;
  KDirOperator *m_dirOp;
  KFileFilterCombo* m_filterWidget;

  void setupGUI();

 private slots:
  void slotFilterChanged();
  void slotFileHighlighted( const KFileItem* item );
  void slotCheckActions();
  void saveConfig( KConfig* c );
};


#endif
