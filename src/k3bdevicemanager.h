
#ifndef K3BDEVICEMANAGER_H
#define K3BDEVICEMANAGER_H

#include <qobject.h>
#include <qstring.h>
#include <qarray.h>
#include <qlist.h>

class KProcess;

class K3bDevice
{
public:
	K3bDevice( int _id,
				 const QString& _device,
				 const QString& _description,
				 const QString& _vendor,
				 const QString& _version,
				 bool _burner,
				 QArray<int> _readSpeed,
				 QArray<int> _burnSpeed = QArray<int>())
		:id(_id), device(_device), description(_description), vendor(_vendor), version(_version), burner(_burner),
			readSpeed(_readSpeed), burnSpeed(_burnSpeed) {}
			
//	~K3bDevice();

	const int id;
	const QString device;
	const QString description;
	const QString vendor;
	const QString version;
	const bool burner;
	const QArray<int> readSpeed;
	const QArray<int> burnSpeed;
};


class K3bDeviceManager : public QObject
{
	Q_OBJECT

public:
	/**
	 * Constructs a device-manager and scans the scsi-bus
	 * for devices. Every instance of K3bDeviceManager on
	 * a machine is equal, so having multible instances
	 * does not make sense.
	 * @param cdrecord While K3b uses cdrecord for all
	 *                 low level shit we need this.
	 **/
	K3bDeviceManager( const QString& cdrecord, QObject* parent );
	~K3bDeviceManager();

	const QList<K3bDevice>& burningDevices();
	/**
	 * Note that all burning devices can also be used as
	 * reading device and are not present in this list.
	 **/
	const QList<K3bDevice>& readingDevices();

	/** writes to stdout **/
	void printDevices();

protected slots:
	void parseCdrecordOutput( KProcess* p, char* output, int len );
	
private:
	QList<K3bDevice> m_writer, m_reader;
	const QString m_cdrecord;

	/**
	 * Returns number of found devices and constructs
	 * the lists m_burner and m_reader.
	 **/
	int scanbus();
};

#endif
