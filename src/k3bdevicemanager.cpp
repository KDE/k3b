#include "k3bdevicemanager.h"

#include <qstring.h>
#include <qlist.h>


K3bDeviceManager::K3bDeviceManager( const QString& cdrecord )
: m_cdrecord(cdrecord)
{
	if( scanbus() < 1 )
		qDebug( "(K3bDeviceManager) No SCSI-devices found.");
	else if( m_burner.count() == 0 )
		qDebug( "(K3bDeviceManager) No burning-devices found.");
}

const QList<K3bDevice>& K3bDeviceManager::burningDevices()
{
	return m_burner;
}

const QList<K3bDevice>& K3bDeviceManager::readingDevices()
{
	return m_reader;
}

int K3bDeviceManager::scanbus()
{
	// Use KProcess in block-mode while this should take less
	// than a second!
	return 1;
}
