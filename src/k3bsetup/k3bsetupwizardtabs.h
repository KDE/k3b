/* 
 *
 * $Id: $
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



#ifndef K3BSETUPWIZARDTABS_H
#define K3BSETUPWIZARDTABS_H

#include "k3bsetuptab.h"

#include <klistview.h>

#include "../device/k3bdevice.h"

class QPushButton;
class QLabel;
class QCheckBox;
class QListBox;
class QGroupBox;
class QLineEdit;
class QListViewItem;
class K3bDeviceWidget;
class K3bExternalBinWidget;


class K3bDeviceViewItem : public KListViewItem
{
 public:
  K3bDeviceViewItem( K3bDevice*, KListView*, const QString& = QString::null );
  K3bDeviceViewItem( K3bDevice*, KListViewItem*, const QString& = QString::null );
  K3bDeviceViewItem( K3bDevice*, KListViewItem*, KListViewItem* prev, const QString& = QString::null );

  K3bDevice* device;
};




class WelcomeTab : public K3bSetupTab
{
  Q_OBJECT

 public:
  WelcomeTab( int, int, K3bSetupWizard* );
};


class DeviceTab : public K3bSetupTab
{
  Q_OBJECT

 public:
  DeviceTab( int, int, K3bSetupWizard* );

  void readSettings();
  bool saveSettings();

 private slots:
  void slotRefreshButtonClicked();

 private:
  K3bDeviceWidget* m_deviceWidget;
};


class NoWriterTab : public K3bSetupTab
{
  Q_OBJECT

 public:
  NoWriterTab( int, int, K3bSetupWizard* );

  bool appropriate();
};


class FstabEntriesTab : public K3bSetupTab
{
  Q_OBJECT

 public:
  FstabEntriesTab( int, int, K3bSetupWizard* );

  void readSettings();
  bool saveSettings();

 private slots:
  void slotMountPointChanged( QListViewItem*, const QString&, int );
  void slotSelectMountPoint();

 private:
  QLabel*      m_labelFstab;
  KListView*   m_viewFstab;
  QCheckBox*   m_checkFstab;
  QPushButton* m_buttonSelectMountPoint;
};


class ExternalBinTab : public K3bSetupTab
{
  Q_OBJECT

 public:
  ExternalBinTab( int, int, K3bSetupWizard* );

  void readSettings();
  bool saveSettings();

  void aboutToShow();

 private:
  QLabel*      m_labelExternalPrograms;
  K3bExternalBinWidget* m_externalBinWidget;
};


class PermissionTab : public K3bSetupTab
{
  Q_OBJECT

 public:
  PermissionTab( int, int, K3bSetupWizard* );

  void readSettings();
  bool saveSettings();

 private slots:
  void slotAddUser();
  void slotRemoveUser();
  void slotPermissionsDetails();

 private:
  QLabel*      m_labelPermissions1;
  QGroupBox*   m_groupUsers;
  QListBox*    m_boxUsers;
  QPushButton* m_buttonRemoveUser;
  QPushButton* m_buttonAddUser;
  QCheckBox*   m_checkPermissionsDevices;
  QCheckBox*   m_checkPermissionsExternalPrograms;
  QPushButton* m_buttonPermissionsDetails;
  QGroupBox*   m_groupWriterGroup;
  QLineEdit*   m_editPermissionsGroup;
};


class FinishTab : public K3bSetupTab
{
  Q_OBJECT

 public:
  FinishTab( int, int, K3bSetupWizard* );

 private slots:
  void slotWritingSetting( const QString& s );
  void slotSettingWritten( bool success, const QString& comment );
  void slotError( const QString& error );

 private:
  KListView* m_viewChanges;
  KListViewItem* m_currentInfoViewItem;
};

#endif
