/***************************************************************************
                          k3bdirview.h  -  description
                             -------------------
    begin                : Mon Mar 26 2001
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

#ifndef K3BDIRVIEW_H
#define K3BDIRVIEW_H

#include <qvbox.h>
#include <qfile.h>
#include <qstring.h>

#include <klistview.h>
#include <kdiroperator.h>
#include <kfiledetailview.h>

class KioTree;
class KFileViewItem;
class QDragObject;
class QSplitter;

/**
  *@author Sebastian Trueg
  */


class K3bDirView : public QVBox  {
  Q_OBJECT

 public:
  K3bDirView(QWidget *parent=0, const char *name=0);
  ~K3bDirView();

 protected slots:
  void slotViewChanged( KFileView* newView );
  void slotDirActivated( const KURL& );
	
 private:     	
  class PrivateFileView;

  KDirOperator* m_fileView;
  QSplitter* m_mainSplitter;

  KioTree* m_kiotree;
};


class K3bDirView::PrivateFileView : public KFileDetailView
{
 public:
  PrivateFileView( QWidget* parent, const char* name );
      	
 protected:
  QDragObject* dragObject() const;
}; // class PrivateFileView

#endif
