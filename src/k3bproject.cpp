#include "k3bproject.h"

// QT-includes
#include <qstring.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qtimer.h>

// KDE-includes
#include <kprocess.h>
#include <kapp.h>
#include <kstddirs.h>


K3bProject::K3bProject( const QString& projectName,
							 const QString& cdrecord,
							 QObject* parent, const char* name )
: QObject(parent, name), m_cdrecord(cdrecord), m_projectName(projectName)
{
	m_process = new KProcess();
	m_timer = new QTimer( this );
	m_burner = 0;
	m_dao = true;
	m_error = NOT_STARTED;
	m_speed = 1;

	if( !QFile::exists( m_cdrecord ) )
		qDebug( "(K3bProject) could not find cdrecord!" );
}


K3bProject::~K3bProject()
{
	delete m_process;
}

void K3bProject::write( const QString& imageFile, bool deleteImage )
{
	// use KProcess to start cdrecord
	// write image to cd
}

void K3bProject::setDao( bool b )
{
	m_dao = b;
}

void K3bProject::setDummy( bool b )
{
	m_dummy = b;
}

void K3bProject::setEject( bool e )
{
	m_eject = e;
}

void K3bProject::setSpeed( int speed )
{
	m_speed = speed;
}

void K3bProject::setBurner( K3bDevice* dev )
{
	m_burner = dev;
}


bool K3bProject::workInProgress() const
{
	return m_process->isRunning();
}


int K3bProject::error() const
{
	return m_error;
}

QString K3bProject::findTempFile( const QString& ending )
{
	QString dir = kapp->dirs()->resourceDirs( "tmp" ).first();
	// TODO: check if the returned dirs end with "/"
	if( dir.isEmpty() )
		dir = "/tmp/";

	// find a free filename
	int num = 1;
	while( QFile::exists( dir + "k3b-" + QString::number( num ) + ending ) )
		num++;

	return dir + "k3b-" + QString::number( num ) + ending;
}

void K3bProject::cancel()
{
	if( workInProgress() )
		m_process->kill();
		
	m_error = CANCELED;
	m_process->disconnect();
	emitMessage("Process canceled.");
	emitResult();
}


void K3bProject::emitResult()
{
	emit result();
}

void K3bProject::emitCanceled()
{
	emit canceled();
}

void K3bProject::emitProgress( unsigned long size, unsigned long processed, int speed )
{
	// TODO: do anything with speed or remove it
	int _percent = (int)( 100.0 * (double)processed / (double)size );
	emit percent( _percent );
	emit processedSize( size );
}

void K3bProject::emitMessage( const QString& msg )
{
	emit infoMessage( msg );
}
