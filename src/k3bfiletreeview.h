/***************************************************************************
                          k3bfiletreeview.h  -  description
                             -------------------
    begin                : Sun Mar 17 2002
    copyright            : (C) 2002 by Sebastian Trueg
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

#ifndef K3BFILETREEVIEW_H
#define K3BFILETREEVIEW_H


#include <kfiletreeview.h>

#include <qmap.h>

class K3bDeviceManager;
class K3bDevice;
class KFileTreeBranch;



class K3bDeviceBranch : public KFileTreeBranch
{
  Q_OBJECT

 public:
  K3bDeviceBranch( KFileTreeView*, K3bDevice* dev, KFileTreeViewItem* item = 0 );

  K3bDevice* device() const { return m_device; }

 public slots:
  bool populate( const KURL& url, KFileTreeViewItem* v );

 private:
  K3bDevice* m_device;
};



/**
  *@author Sebastian Trueg
  */
class K3bFileTreeView : public KFileTreeView
{
  Q_OBJECT

 public: 
  K3bFileTreeView( QWidget *parent = 0, const char *name = 0 );
  ~K3bFileTreeView();


  virtual KFileTreeBranch* addBranch( KFileTreeBranch* );
  virtual KFileTreeBranch* addBranch( const KURL& url, const QString& name, const QPixmap& , bool showHidden = false );

  /**
   * adds home and root dir branch
   */
  void addDefaultBranches();

  void addCdDeviceBranches( K3bDeviceManager* );

 public slots:
  void followUrl( const KURL& url );
  void setTreeDirOnlyMode( bool b );

 signals:
  void urlExecuted( const KURL& url );
  void deviceExecuted( K3bDevice* dev );

 private slots:
  void slotItemExecuted( QListViewItem* item );

 private:
  bool m_dirOnlyMode;
  QMap<KFileTreeBranch*, K3bDevice*> m_deviceBranchesMap;
};

#endif
