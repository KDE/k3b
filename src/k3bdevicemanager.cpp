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
	if( scanbus() < 1 )
		qDebug( "(K3bDeviceManager) No SCSI-devices found.");
	else if( m_writer.count() == 0 )
		qDebug( "(K3bDeviceManager) No burning-devices found.");
}


K3bDeviceManager::~K3bDeviceManager()
{
	m_reader.setAutoDelete( true );
	m_writer.setAutoDelete( true );
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
	
	if( !process->start( KProcess::Block, KProcess::AllOutput ) )
		qDebug( "(K3bDeviceManager) could not start cdrecord: %s", kapp->config()->readEntry( "cdrecord path" ).latin1() );
	
	delete process;
	return m_reader.count() + m_writer.count();
}

void K3bDeviceManager::parseCdrecordOutput( KProcess* p, char* output, int len )
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
			QString dev = line.mid(0,5);
			QString vendor = line.mid(12,8);
			QString descr = line.mid(23,16);
			QString version = line.mid(42,4);
			
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
			m_reader.append( new K3bDevice( id, dev, descr, vendor, version, true, 32,12 ) );
			m_writer.append( new K3bDevice( id, dev, descr, vendor, version, true, 32,12 ) );
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
