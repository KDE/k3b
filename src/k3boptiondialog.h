/***************************************************************************
                          k3boptiondialog.h  -  description
                             -------------------
    begin                : Tue Apr 17 2001
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

#ifndef K3BOPTIONDIALOG_H
#define K3BOPTIONDIALOG_H

#include <kdialogbase.h>
#include <qlist.h>
#include <klistview.h>

class KListView;
class KActionMenu;
class KAction;
class QLabel;
class QListViewItem;
class QPushButton;
class QGroupBox;
class QXEmbed;
class QWidgetStack;
class QCheckBox;

class K3bDevice;

/**
  *@author Sebastian Trueg
  */

class K3bOptionDialog : public KDialogBase
{
  Q_OBJECT

 public:
  K3bOptionDialog(QWidget *parent=0, const char *name=0, bool modal = true);
  ~K3bOptionDialog();
	
  enum m_configPageIndex { Devices = 0, Programs = 1 };
  
 protected slots:
  void slotOk();
  void slotApply();
  void slotDefault();
  void slotStartPS();
	
 private:
  // programs Tab
  KListView* m_viewPrograms;
  QPushButton* m_buttonSearch;
  QLabel* m_labelInfo;

  // devices Tab
  QLabel* m_labelDevicesInfo;
  KListView* m_viewDevicesReader;
  KListView* m_viewDevicesWriter;
  KListView* m_viewDeviceInfo;
  QGroupBox* m_groupReader;
  QGroupBox* m_groupWriter;
  QGroupBox* m_groupDeviceInfo;
  QPushButton* m_buttonRefreshDevices;
  KActionMenu* m_menuDevices;
  KAction* m_actionNewDevice;
  KAction* m_actionRemoveDevice;
  /** list to save changes to the devices before appying */
  QList<K3bDevice> m_tempReader;
  QList<K3bDevice> m_tempWriter;

  class PrivateDeviceViewItem : public KListViewItem {
  public:
    PrivateDeviceViewItem( K3bDevice* dev, KListView* view )
      : KListViewItem( view ) { device = dev; }
    PrivateDeviceViewItem( K3bDevice* dev, QListViewItem* item )
      : KListViewItem( item ) { device = dev; }

    K3bDevice* device;
  };

  // permission tab
  QWidgetStack* m_stackPermission;
  QXEmbed* m_embedPermission;
  QPushButton* m_buttonStartPS;
  QWidget* m_containerInfo;

  // burning tab
  QCheckBox* m_checkUseID3Tag;
		
  void setupProgramsPage();
  void readPrograms();
  bool savePrograms();
	
  void setupDevicePage();
  void readDevices();
  void updateDeviceListViews();
  void updateDeviceInfoBox( K3bDevice* dev = 0 );
  void saveDevices();

  void setupPermissionPage();

  void setupBurningPage();
  void readBurningSettings();
  void saveBurningSettings();
	
  bool devicesChanged;
			
 private slots:
  void slotDeviceSelected(QListViewItem*);
  void slotDeviceInfoRenamed( QListViewItem* );
  void slotRefreshDevices();
  void slotNewDevice();
  void slotRemoveDevice();
  void slotDevicesPopup( QListViewItem*, const QPoint& );
};

#endif
