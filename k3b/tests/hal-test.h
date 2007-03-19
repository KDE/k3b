
#ifndef _TESTHAL_H_
#define _TESTHAL_H_

#include "../libk3bdevice/k3bhalconnection.h"
#include <qlabel.h>


class Main : public QLabel
{
  Q_OBJECT

public:
  Main()
    : QLabel( "Close this window to end the HAL test", 0, 0 ) {
    connect( &hal, SIGNAL(deviceAdded(const QString&)),
	     this, SLOT(slotDeviceAdded(const QString&)) );
    connect( &hal, SIGNAL(deviceRemoved(const QString&)),
	     this, SLOT(slotDeviceRemoved(const QString&)) );
    qDebug( "Opening connection to HAL..." );
    hal.open();
  }

private slots:
  void slotDeviceAdded( const QString& dev ) {
    qDebug( "Device added: %s", dev.latin1() );
  }
  void slotDeviceRemoved( const QString& dev ) {
    qDebug( "Device removed: %s", dev.latin1() );
  }

private:
  K3bDevice::HalConnection hal;
};
#endif

