
#ifndef K3BDEVICEMANAGER_H
#define K3BDEVICEMANAGER_H

#include <qstring.h>
#include <qarray.h>
#include <qlist.h>


class K3bDevice
{
public:
	K3bDevice( int id,
				 QString device,
				 QString description,
				 bool burner,
				 QArray<int> readSpeed,
				 QArray<int> burnSpeed = QArray<int>());
//	~K3bDevice();

	const int id;
	const QString device;
	const QString description;
	const bool burner;
	const QArray<int> readSpeed;
	const QArray<int> burnSpeed;
};


class K3bDeviceManager
{
public:
	/**
	 * Constructs a device-manager and scans the scsi-bus
	 * for devices. Every instance of K3bDeviceManager on
	 * a machine is equal, so having multible instances
	 * does not make sense.
	 * @param cdrecord While K3b uses cdrecord for all
	 *                 low level shit we need this.
	 **/
	K3bDeviceManager( const QString& cdrecord );
//	~K3bDeviceManager();

	const QList<K3bDevice>& burningDevices();
	/**
	 * Note that all burning devices can also be used as
	 * reading device and are not present in this list.
	 **/
	const QList<K3bDevice>& readingDevices();

private:
	QList<K3bDevice> m_burner, m_reader;
	const QString m_cdrecord;

	/**
	 * Returns number of found devices and constructs
	 * the lists m_burner and m_reader.
	 **/
	int scanbus();
};

#endif
