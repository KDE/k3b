
#ifndef K3B_DEVICE_OPTIONTAB_H
#define K3B_DEVICE_OPTIONTAB_H

#include <qwidget.h>
#include <qlist.h>
#include <klistview.h>

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


class K3bDeviceOptionTab : public QWidget
{
Q_OBJECT

 public:
  K3bDeviceOptionTab( QWidget*, const char* name = 0 );
  ~K3bDeviceOptionTab();

  void readDevices();
  void saveDevices();

 private slots:
  void slotDeviceSelected(QListViewItem*);
  void slotRefreshDevices();
  void slotNewDevice();
  void slotCdrdaoDriverChanged(const QString&);
  void slotCdTextCapabilityChanged( const QString& );
  void slotWriteSpeedChanged( int );
  void slotReadSpeedChanged( int );

 private:
  /** list to save changes to the devices before applying */
  class PrivateTempDevice {
  public:
    PrivateTempDevice( K3bDevice* d );

    K3bDevice* device;
    int maxReadSpeed;
    int maxWriteSpeed;
    QString cdrdaoDriver;
    bool cdTextCapable;
  };

  QList<K3bDeviceOptionTab::PrivateTempDevice> m_tempReader;
  QList<K3bDeviceOptionTab::PrivateTempDevice> m_tempWriter;
  K3bDeviceOptionTab::PrivateTempDevice* m_currentTempDevice;

  class PrivateDeviceViewItem : public KListViewItem {
  public:
    PrivateDeviceViewItem( K3bDeviceOptionTab::PrivateTempDevice* dev, KListView* view )
      : KListViewItem( view ) { device = dev; }
    PrivateDeviceViewItem( K3bDeviceOptionTab::PrivateTempDevice* dev, QListViewItem* item )
      : KListViewItem( item ) { device = dev; }

    K3bDeviceOptionTab::PrivateTempDevice* device;
  };


  void updateDeviceListViews();
  void updateDeviceInfoBox( K3bDeviceOptionTab::PrivateTempDevice* dev = 0 );
  void showWriterSpecificProps( bool );

  QGroupBox*    m_groupDeviceInfo;
  QLabel*       m_labelDevicesInfo;
  KListView*    m_viewDevicesReader;
  KListView*    m_viewDevicesWriter;
  QGroupBox*    m_groupReader;
  QGroupBox*    m_groupWriter;
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
  QLabel*       m_labelWriteSpeed;
  QFrame*       m_line3;
  KIntNumInput* m_spinReadSpeed;
  KIntNumInput* m_spinWriteSpeed;
  QComboBox*    m_comboDriver;
  QComboBox*    m_comboCdText;
  QLabel*    m_checkBurnProof;

  bool devicesChanged;
};



#endif
