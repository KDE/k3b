
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
				 int _maxReadSpeed,
				 int _maxBurnSpeed = 0)
		:id(_id), device(_device), description(_description), vendor(_vendor), version(_version), burner(_burner),
			maxReadSpeed(_maxReadSpeed), maxWriteSpeed(_maxBurnSpeed) {}
			
//	~K3bDevice();

	int id;
	QString device;
	QString description;
	QString vendor;
	QString version;
	bool burner;
	int maxReadSpeed;
	int maxWriteSpeed;
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
	 **/
	K3bDeviceManager( QObject* parent );
	~K3bDeviceManager();

	QList<K3bDevice>& burningDevices();
	/**
	 * Note that all burning devices can also be used as
	 * reading device and are not present in this list.
	 **/
	QList<K3bDevice>& readingDevices();

	/** writes to stdout **/
	void printDevices();

protected slots:
	void parseCdrecordOutput( KProcess* p, char* output, int len );
	
private:
	QList<K3bDevice> m_writer, m_reader;

	/**
	 * Returns number of found devices and constructs
	 * the lists m_burner and m_reader.
	 **/
	int scanbus();
};

#endif
