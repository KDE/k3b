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

class QSplitter;
class KFileView;
class KURL;
class KioTree;
class K3bCdView;
class K3bFileView;
class K3bDeviceManager;
class KProcess;
class KComboBox;


/**
  *@author Sebastian Trueg
  */
class K3bDirView : public QVBox  
{
  Q_OBJECT

 public:
  K3bDirView(QWidget *parent=0, const char *name=0);
  ~K3bDirView();
  void setupFinalize( K3bDeviceManager *dm);

 protected slots:
  void slotDirActivated( const KURL& );
  void slotDirActivated( const QString& );
  void slotCDDirActivated( const QString&  );
  void slotUpdateURLCombo( const KURL& url );

 private:
  K3bCdView* m_cdView;
  K3bFileView* m_fileView;
  KComboBox* m_urlCombo;
  QSplitter* m_mainSplitter;
  KioTree* m_kiotree;
  bool m_initialized;
};

#endif
