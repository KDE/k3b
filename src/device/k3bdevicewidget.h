/***************************************************************************
                          k3bdevicewidget.h  -  description
                             -------------------
    begin                : Wed Apr 17 2002
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

#ifndef K3BDEVICEWIDGET_H
#define K3BDEVICEWIDGET_H

#include <qwidget.h>
#include <qlist.h>

class QComboBox;
class QLabel;
class QGroupBox;
class QPushButton;
class QCheckBox;
class KListView;
class QString;
class KIntNumInput;
class QFrame;
class QListViewItem;
class QString;
class K3bDevice;
class K3bDeviceManager;


/**
  *@author Sebastian Trueg
  */
class K3bDeviceWidget : public QWidget
{
  Q_OBJECT

 public:
  K3bDeviceWidget( K3bDeviceManager*, QWidget *parent = 0, const char *name = 0 );
  ~K3bDeviceWidget();

 public slots:
  void init();
  void apply();

 signals:
  void refreshButtonClicked();

 private slots:
  void slotDeviceSelected(QListViewItem*);
 //  void slotRefreshDevices();
  void slotNewDevice();
  void slotCdrdaoDriverChanged(const QString&);
  void slotCdTextCapabilityChanged( const QString& );
  void slotWriteSpeedChanged( int );
  void slotReadSpeedChanged( int );
  void slotCdrwChanged(bool);
  void slotBurnproofChanged(bool);
  void slotDaoChanged(bool);

 private:
  /** list to save changes to the devices before applying */
  class PrivateTempDevice;
  class PrivateDeviceViewItem;

  QList<PrivateTempDevice> m_tempDevices;
  PrivateTempDevice* m_currentTempDevice;

  void updateDeviceListViews();
  void updateDeviceInfoBox( PrivateTempDevice* dev = 0 );
  void showWriterSpecificProps( bool );

  QListViewItem* m_writerParentViewItem;
  QListViewItem* m_readerParentViewItem;

  K3bDeviceManager* m_deviceManager;

  QGroupBox*    m_groupDeviceInfo;
  QLabel*       m_labelDevicesInfo;
  KListView*    m_viewDevices;
  QPushButton*  m_buttonRefreshDevices;
  QPushButton*  m_buttonAddDevice;
  QLabel*       m_labelDevicename;
  QLabel*       m_labelDeviceInterface;
  QLabel*       m_labelVendor;
  QLabel*       m_labelDescription;
  QLabel*       m_labelVersion;
  QLabel*       m_labelDriver;
  QLabel*       m_labelCdText;
  QLabel*       m_labelBurnProof;
  QLabel*       m_labelCdrw;
  QLabel*       m_labelDao;
  QLabel*       m_labelWriteSpeed;
  QFrame*       m_line3;
  KIntNumInput* m_spinReadSpeed;
  KIntNumInput* m_spinWriteSpeed;
  QComboBox*    m_comboDriver;
  QComboBox*    m_comboCdText;
  QCheckBox*    m_checkBurnProof;
  QCheckBox*    m_checkCdrw;
  QCheckBox*    m_checkDao;
};

#endif
