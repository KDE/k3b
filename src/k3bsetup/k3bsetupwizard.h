

#ifndef K3BSETUPWIZARD_H
#define K3BSETUPWIZARD_H

#include <qvariant.h>
#include <kwizard.h>


class QCheckBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QListBox;
class QPushButton;
class QWidget;
class QCloseEvent;
class QKeyEvent;
class QListViewItem;

class K3bSetup;
class K3bDeviceManager;
class KSimpleConfig;
class KListView;

class K3bSetupWizard : public KWizard
{ 
    Q_OBJECT

 public:
  K3bSetupWizard( QWidget* = 0, const char* = 0, bool = FALSE, WFlags = WType_TopLevel | WDestructiveClose );
  ~K3bSetupWizard();

  void init();
  void apply();

  /**
   * reimplemented from QWizard
   */
  bool appropriate( QWidget* ) const;

 protected:
  void closeEvent( QCloseEvent* );
  void keyPressEvent( QKeyEvent* );

 protected slots:
  void accept();
  void slotAddDevice();
  void slotAddUser();
  void slotRemoveUser();
  void slotPermissionsDetails();
  void slotDeviceItemRenamed( QListViewItem*, const QString&, int );
  void slotSelectMountPoint();

 private:
  void updateDevices();
  void updateFstabEntries();
  void createNewFstab();

  class PrivateDeviceViewItem;

  K3bSetup* m_setup;
  K3bDeviceManager* m_deviceManager;
  KSimpleConfig* m_config;
  QString m_configPath;
  
  QWidget* m_page1;
  QLabel* m_labelWelcome;
  
  QWidget* m_page2;
  QPushButton* m_buttonAddDevice;
  QLabel* m_labelSetupDrives;
  KListView* m_viewSetupReader;
  KListView* m_viewSetupWriter;
  
  QWidget* m_page3;
  QLabel* m_labelNoWriter;
  
  QWidget* m_page4;
  QLabel* m_labelFstab;
  KListView* m_viewFstab;
  QCheckBox* m_checkFstab;
  QPushButton* m_buttonSelectMountPoint;
  
  QWidget* m_page5;
  QLabel* m_labelExternalPrograms;
  KListView* m_viewExternalPrograms;
  
  QWidget* m_page6;
  QLabel* m_labelPermissions1;
  QGroupBox* m_groupUsers;
  QListBox* m_boxUsers;
  QPushButton* m_buttonRemoveUser;
  QPushButton* m_buttonAddUser;
  QCheckBox* m_checkPermissionsDevices;
  QCheckBox* m_checkPermissionsExternalPrograms;
  QLabel* m_labelPermissions2;
  QPushButton* m_buttonPermissionsDetails;
  QGroupBox* m_groupWriterGroup;
  QLineEdit* m_editPermissionsGroup;
  
  QWidget* m_page7;
};

#endif // K3BSETUPWIZARD_H
