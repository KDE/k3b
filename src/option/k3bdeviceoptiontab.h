
#ifndef K3B_DEVICE_OPTIONTAB_H
#define K3B_DEVICE_OPTIONTAB_H

#include <qwidget.h>

class QLabel;
class K3bDeviceWidget;


class K3bDeviceOptionTab : public QWidget
{
Q_OBJECT

 public:
  K3bDeviceOptionTab( QWidget*, const char* name = 0 );
  ~K3bDeviceOptionTab();

  void readDevices();
  void saveDevices();

 private:
  QLabel*          m_labelDevicesInfo;
  K3bDeviceWidget* m_deviceWidget;
};



#endif
