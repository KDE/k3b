

#ifndef K3BSETUPWIZARDTABS_H
#define K3BSETUPWIZARDTABS_H

#include "k3bsetuptab.h"

#include <klistview.h>

class QPushButton;
class QLabel;
class QCheckBox;
class QListBox;
class QGroupBox;
class QLineEdit;
class QListViewItem;
class K3bDevice;




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
  void slotAddDevice();
  void slotDeviceItemRenamed( QListViewItem*, const QString&, int );

 private:
  QPushButton* m_buttonAddDevice;
  QLabel*      m_labelSetupDrives;
  KListView*   m_viewSetupReader;
  KListView*   m_viewSetupWriter;
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

 private slots:
  void slotExternalProgramItemRenamed( QListViewItem*, const QString&, int );
  void slotSelectExternalBin();

 private:
  QLabel*      m_labelExternalPrograms;
  KListView*   m_viewExternalPrograms;
  QPushButton* m_buttonSelectExternalBin;
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
