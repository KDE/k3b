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
  /** list to save changes to the devices before applying */
  class PrivateTempDevice {
  public:
    PrivateTempDevice() {
      burner = false;
      burnproof = false;
      maxReadSpeed = maxWriteSpeed = 0;
    }

    PrivateTempDevice( const QString & _vendor,
		       const QString & _description,
		       const QString & _version,
		       bool _burner,
		       bool _burnproof,
		       int _maxReadSpeed,
		       const QString & _devicename, int _maxBurnSpeed = 0 )
      : vendor( _vendor ),
      description( _description ), version( _version ), burner( _burner ),
      burnproof( _burnproof ), maxReadSpeed( _maxReadSpeed ),
      devicename( _devicename ), maxWriteSpeed( _maxBurnSpeed ) {}
    
    QString vendor;
    QString description;
    QString version;
    bool burner;
    bool burnproof;
    int maxReadSpeed;
    QString devicename;
    int maxWriteSpeed;
  };
  QList<PrivateTempDevice> m_tempReader;
  QList<PrivateTempDevice> m_tempWriter;

  class PrivateDeviceViewItem : public KListViewItem {
  public:
    PrivateDeviceViewItem( PrivateTempDevice* dev, KListView* view )
      : KListViewItem( view ) { device = dev; }
    PrivateDeviceViewItem( PrivateTempDevice* dev, QListViewItem* item )
      : KListViewItem( item ) { device = dev; }

    PrivateTempDevice* device;
  };

  // burning tab
  QCheckBox* m_checkUseID3Tag;
		
  void setupProgramsPage();
  void readPrograms();
  bool savePrograms();
	
  void setupDevicePage();
  void readDevices();
  void updateDeviceListViews();
  void updateDeviceInfoBox( PrivateTempDevice* dev = 0 );
  void saveDevices();

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
