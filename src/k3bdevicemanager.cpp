#include "k3bdevicemanager.h"

#include <qstring.h>
#include <qstringlist.h>
#include <qlist.h>

#include <kprocess.h>
#include <kapp.h>
#include <kconfig.h>

#include <iostream>


K3bDeviceManager::K3bDeviceManager( QObject* parent )
: QObject(parent), m_reader(), m_writer()
{
	m_reader.setAutoDelete( true );
	m_writer.setAutoDelete( true );
}


K3bDeviceManager::~K3bDeviceManager()
{
}

QList<K3bDevice>& K3bDeviceManager::burningDevices()
{
	return m_writer;
}

QList<K3bDevice>& K3bDeviceManager::readingDevices()
{
	return m_reader;
}

int K3bDeviceManager::scanbus()
{
	kapp->config()->setGroup("External Programs");
	
	// Use KProcess in block-mode while this should take less
	// than a second!
	KProcess* process = new KProcess();
	*process << kapp->config()->readEntry( "cdrecord path" );
	*process << "-scanbus";
	
//	connect( process, SIGNAL(processExited(KProcess*)),
//			 this, SLOT(mp3FileTestingFinished()) );
	connect( process, SIGNAL(receivedStdout(KProcess*, char*, int)),
			 this, SLOT(parseCdrecordOutput(KProcess*, char*, int)) );
	connect( process, SIGNAL(receivedStderr(KProcess*, char*, int)),
			 this, SLOT(parseCdrecordOutput(KProcess*, char*, int)) );
	
	m_foundDevices = 0;
			
	if( !process->start( KProcess::Block, KProcess::AllOutput ) )
		qDebug( "(K3bDeviceManager) could not start cdrecord: %s", kapp->config()->readEntry( "cdrecord path" ).latin1() );
	
	delete process;
	return m_foundDevices;
}

void K3bDeviceManager::parseCdrecordOutput( KProcess*, char* output, int len )
{
	QString buffer = QString::fromLatin1( output, len );
	
	// split to lines
	QStringList lines = QStringList::split( "\n", buffer );
	
	for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ ) {
		QString line( *str );
		line = line.stripWhiteSpace();
		if( line.startsWith("0,") && line.at(line.length()-1) != '*' ) {
			qDebug("parsing line: [[" + line + "]]" );
			// should be usable
			QString dev = line.mid(0,5).simplifyWhiteSpace();
			QString vendor = line.mid(12,8).simplifyWhiteSpace();
			QString descr = line.mid(23,16).simplifyWhiteSpace();
			QString version = line.mid(42,4).simplifyWhiteSpace();
			
			// calculate id
			bool ok, ok2 = true;
			int id = dev.mid(0,1).toInt(&ok);
			if(!ok)
				ok2 = false;
			id += dev.mid(2,1).toInt(&ok) *10;
			if(!ok)
				ok2 = false;
			id += dev.mid(4,1).toInt(&ok) *100;
			if(!ok)
				ok2 = false;
			if(!ok2)
				qDebug("id-parsing did not work for: " + dev );
				
			// make new device
			// TODO: what about the speed? And what about the type (reader/writer)?
			m_reader.append( new K3bDevice( id, dev, descr, vendor, version, false, 32,12 ) );
			m_writer.append( new K3bDevice( id, dev, descr, vendor, version, true, 32,12 ) );
			
			m_foundDevices++;
		}
	} // for
}


void K3bDeviceManager::printDevices()
{
	cout << "\nReader:" << endl;
	for( K3bDevice* dev = m_reader.first(); dev != 0; dev = m_reader.next() ) {
		cout << "  " << dev->id << ": "<< dev->device << " " << dev->vendor << " " << dev->description << " " << dev->version << endl;
	}
	cout << "\nWriter:" << endl;
	for( K3bDevice* dev = m_writer.first(); dev != 0; dev = m_writer.next() ) {
		cout << "  " << dev->id << ": " << dev->device << " " << dev->vendor << " " << dev->description << " " << dev->version << endl;
	}
	cout << flush;
}

void K3bDeviceManager::clear()
{
	// clear current devices
	m_reader.clear();
	m_writer.clear();
}

int K3bDeviceManager::readConfig()
{
	KConfig* c = kapp->config();
	m_foundDevices = 0;
	
	if( c->hasGroup("Devices") ) {
		c->setGroup("Devices");
		
		// read Readers
		QStringList list = c->readListEntry("Reader1");
		int devNum = 1;
		while( !list.isEmpty() ) {
			// create K3bDevice
			if( list.count() < 5 )
				qDebug("(K3bDeviceManager) Corrupt entry in Kconfig file");
			else {
				int id = 0; // ID is not important so far!! ;-)
				m_reader.append( new K3bDevice( id, list[0], list[2], list[1], list[3], false, list[4].toInt() ) );
				m_foundDevices++;
			}
			devNum++;
			list = c->readListEntry( QString("Reader%1").arg(devNum) );
		}
		
		// read Writers
		list = c->readListEntry("Writer1");
		devNum = 1;
		while( !list.isEmpty() ) {
			// create K3bDevice
			if( list.count() < 6 )
				qDebug("(K3bDeviceManager) Corrupt entry in Kconfig file");
			else {
				int id = 0; // ID is not important so far!! ;-)
				m_writer.append( new K3bDevice( id, list[0], list[2], list[1], list[3], true, list[4].toInt(), list[5].toInt() ) );
				m_foundDevices++;
			}
			devNum++;
			list = c->readListEntry( QString("Writer%1").arg(devNum) );
		}
	}
	
	return m_foundDevices;
}
