#include "k3baudioproject.h"
#include "k3baudiotrack.h"

#include <iostream>

// QT-includes
#include <qstring.h>
#include <qstringlist.h>
#include <qfile.h>
#include <qdatetime.h>

// KDE-includes
#include <kprocess.h>



K3bAudioProject::K3bAudioProject( const QString& projectName,
									   const QString& cdrecord,
									   const QString& mpg123,
									   QObject* parent, const char* name )
: K3bProject( projectName, cdrecord, parent, name ), m_tracks(), m_mpg123(mpg123), lastTempFile(), lastTestedFile()
{
	m_currentBufferedTrack = 0;
	m_lastAddedTrack = 0;
	
	startBurningAfterBuffering = false;

	if( !QFile::exists( m_mpg123 ) )
		qDebug( "(K3bAudioProject) could not find mpg123!" );
}

K3bAudioProject::~K3bAudioProject()
{
	m_tracks.setAutoDelete( true );
}


void K3bAudioProject::prepareTracks()
{
	if( workInProgress() )
		return;

	m_currentBufferedTrack = at(0);

	bufferFiles();
}

void K3bAudioProject::write()
{
	if( workInProgress() )
		return;

	// check if all the files are converted to wav...
	bool _allConv = true;
	for( K3bAudioTrack* i = at(0); i != 0 && _allConv; i = next() )
	{
		if( i->filetype() == MP3 && i->bufferFile().isEmpty() )
			_allConv = false;
	}

	if( !_allConv )
	{
		// there is some convertion work to do before burning...
		startBurningAfterBuffering = true;
		bufferFiles();
	}
	else
	{
		startRecording();
	}
}

void K3bAudioProject::writeImage( const QString& filename )
{
	// since cdrecord does not support this it will not work
	// before i implement it on my own (and i hope so!)
	qDebug("(K3bAudioProject) Image creation not implemented yet!");
}

void K3bAudioProject::parseCdrecordOutput( KProcess* process, char* output, int len )
{
	// for debugging
	cout << "(cdrecord output)\n" << output;

	// simple shit for the beginning
	emitMessage( output );
}

void K3bAudioProject::parseMpg123Output( KProcess* process, char* output, int len )
{
	QString buffer = QString::fromLatin1( output, len );
	
	// split to lines
	QStringList lines = QStringList::split( "\n", buffer );
	
	for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ )
	{
		if( (*str).left(10).contains("Frame#") ) {
			int made, togo;
			bool ok;
			made = (*str).mid(8,5).toInt(&ok);
			if( !ok )
				qDebug("parsing did not work for " + (*str).mid(8,5) );
			else {
				togo = (*str).mid(15,5).toInt(&ok);
				if( !ok )
					qDebug("parsing did not work for " + (*str).mid(15,5) );
				else
					emitProgress( made+togo, made );
			}
		}
		else if( (*str).left(10).contains("Title") ) {
			qDebug("parsing this: [[" + (*str) +"]]");
			qDebug("setting mp3-tags to: " + (*str).mid(9,30).simplifyWhiteSpace()  + " and " +  (*str).mid(49,30).simplifyWhiteSpace());
			m_lastAddedTrack->setTitle( (*str).mid(9,30).simplifyWhiteSpace() );
			m_lastAddedTrack->setArtist( (*str).mid(49,30).simplifyWhiteSpace() );
		}
		else if( (*str).contains("Decoding of") ) {
			qDebug("parsing this: [[" + (*str)+"]]" );
			int h, m, s;
			h=m=s=0;
			QString timeStr = (*str).mid( 1, 10 );
			timeStr.truncate( timeStr.find(']') );
			qDebug("parsing this: [[" + timeStr +"]]" );
			QStringList list = QStringList::split( ':', timeStr );
			int i = 0;
			if( list.count() > 2 )
				h = list[i++].toInt();
			m = list[i++].toInt();
			s = list[i++].toInt();
			
			qDebug("setting length to %i:%i:%i", h,m,s);
			m_lastAddedTrack->setLength( QTime(h,m,s) );
		}
	}
}


void K3bAudioProject::cdrecordFinished()
{
	if( m_process->normalExit() )
	{
		// TODO: check the process' exitStatus()
		m_error = SUCCESS;
		emitMessage( "Burning finished" );
	}
	else
	{
		m_error = CDRECORD_ERROR;
		emitMessage( "cdrecord did not exit cleanly!" );
	}

	emitResult();

	m_process->disconnect();
}

void K3bAudioProject::startRecording()
{
	// OK, we need a new cdrecord process...
	m_process->clearArguments();
	*m_process << m_cdrecord;

	// and now we add the needed arguments...
	// display progress
	*m_process << "-v";

	if( dummy() )
		*m_process << "-dummy";
	if( dao() )
		*m_process << "-dao";
	if( eject() )
		*m_process << "-eject";

	// add speed
	QString s = "-speed=" + speed();
	*m_process << s;

	// add the device, perhaps in the future "-dev=X,X,X" would be better
	s = "-dev=" + burner()->device;
	*m_process << s;

	*m_process << "-audio";

	// add all the tracks
	for( K3bAudioTrack* i = at(0); i != 0; i = next() )
	{
		s = "-pregap=" + i->pregap();
		*m_process << s;
		if( !i->bufferFile().isEmpty() )
			*m_process << i->bufferFile();
		else
			*m_process << i->fileName();
	}

	if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
	{
		// something went wrong when starting the program
		// it "should" be the executable
		qDebug("(K3bAudioProject) could not start cdrecord");
		m_error = CDRECORD_ERROR;
		emitResult();
	}
	else
	{
		// connect to the cdrecord slots
		connect( m_process, SIGNAL(processExited(KProcess*)),
				 this, SLOT(cdrecordFinished()) );
		connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
				 this, SLOT(parseCdrecordOutput(char*, int)) );
		m_error = WORKING;
	}
}


void K3bAudioProject::bufferFiles( )
{
	// find first file to buffer
	while( m_currentBufferedTrack &&
		  !( m_currentBufferedTrack->filetype() == MP3 && m_currentBufferedTrack->bufferFile().isEmpty() ) )
		m_currentBufferedTrack = next();

	if( m_currentBufferedTrack == 0 )
	{
		m_process->disconnect();
		if( startBurningAfterBuffering )
			startRecording();
		else
		{
			m_error = SUCCESS;
			emitResult();
		}
		return;
	}

	// start a new mpg123 process for this track
	m_process->clearArguments();
	*m_process << m_mpg123;
	lastTempFile = findTempFile( "wav" );
	*m_process << "-w";
	*m_process << lastTempFile;
	*m_process << m_currentBufferedTrack->fileName();

	if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
	{
		// something went wrong when starting the program
		// it "should" be the executable
		emitMessage( "could not start mpg123");
		m_error = MPG123_ERROR;
		emitResult();
	}
	else
	{
		// connect to the mpg123-slots
		connect( m_process, SIGNAL(processExited(KProcess*)),
				 this, SLOT(fileBufferingFinished()) );
		connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
				 this, SLOT(parseMpg123Output(char*, int)) );

		m_error = WORKING;
	}
}


void K3bAudioProject::fileBufferingFinished()
{
	// check if process exited successfully and if so:
	m_currentBufferedTrack->setBufferFile( lastTempFile );

	bufferFiles();
}



int K3bAudioProject::size(){
	// TODO: sum the sizes of all tracks, the pregaps, and leadin/leadout stuff
	return 0;
}


void K3bAudioProject::addTrack( const QString& fileName, uint position )
{
	if( workInProgress() )
		return;
	
	m_error = WORKING;
	
	// append at the end by default
	if( position > m_tracks.count() )
		position = m_tracks.count();
	
	if( !QFile::exists( fileName ) ) {
		emitMessage( "File not found: " + fileName );
		m_error = FILE_NOT_FOUND;
		emitResult();
		return;
	}
	
	QString ending = fileName.right( 7 );
	if( ending.contains("wav") ) {
		addWavFile( fileName, position );
	}
	else if( ending.contains( "mp3" ) ) {
		addMp3File( fileName, position );
	}
	else {
		emitMessage( "Wrong file format. Only wav and mp3 audio files can be used." );
		m_error = WRONG_FILE_FORMAT;
		result();
	}
}

void K3bAudioProject::addWavFile( const QString& fileName, uint position )
{
	// TODO: test the wavfile
	// for now without any testing
	// at this point we assume that the file exists
	emitMessage( "Adding wav-audio file " + fileName + " to project." );
	lastTestedFile = fileName;
	lastAddedPosition = position;
	
	addTrack( new K3bAudioTrack( &m_tracks, fileName ), lastAddedPosition );
	m_error = SUCCESS;
	emitProgress( 100, 100 );
	emitResult();
}

void K3bAudioProject::addMp3File( const QString& fileName, uint position )
{
	emitMessage( "Adding mp3-audio file " + fileName + " to project." );

	// create a new K3bAudioTrack without knowing if we can use it
	// but it will be easier for further shit
	m_lastAddedTrack = new K3bAudioTrack( &m_tracks, fileName );
	
	lastAddedPosition = position;
	
	// test the file with mpg123
	// at this point we assume that the file exists
	m_process->disconnect();
	m_process->clearArguments();
	*m_process << m_mpg123;
	*m_process << "-v";
	*m_process << "-t";
	*m_process << fileName;
	
	if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
	{
		// something went wrong when starting the program
		// it "should" be the executable
		delete m_lastAddedTrack;
		m_lastAddedTrack = 0;
		emitMessage("Could not start mpg123.");
		m_error = MPG123_ERROR;
		emitResult();
	}
	else
	{
		// connect to the mpg123-slots
		connect( m_process, SIGNAL(processExited(KProcess*)),
				 this, SLOT(mp3FileTestingFinished()) );
		connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
				 this, SLOT(parseMpg123Output(KProcess*, char*, int)) );
 		connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
				 this, SLOT(parseMpg123Output(KProcess*, char*, int)) );
	}
}


void K3bAudioProject::mp3FileTestingFinished()
{
	m_process->disconnect();
	
	if( !m_process->normalExit() ) {
		// do some shit
		emitMessage( "mpg123 did not exit with errors." );
		delete m_lastAddedTrack;
		m_lastAddedTrack = 0;
		m_error = MPG123_ERROR;
		emitResult();
		return;
	}
	
	// check the process' errorlevel
	if( !(m_process->exitStatus() == 0 ) ) {
		delete m_lastAddedTrack;
		m_lastAddedTrack = 0;
		emitMessage( "Mp3-File " + lastTestedFile + " is corrupt." );
		m_error = CORRUPT_MP3;
		emitResult();
		return;
	}
	
	// here the file should be valid!
	
	// TODO: what about the size of the track?
	// TODO: test the file and find the mp3-tags! Do it like testFile or use it
	addTrack( m_lastAddedTrack, lastAddedPosition );
	
	// some debugging shit...
	qDebug( m_lastAddedTrack->fileName() + " title: " + m_lastAddedTrack->artist() + " " + m_lastAddedTrack->title() );
	
	m_error = SUCCESS;
	emitResult();
}


void K3bAudioProject::addTrack( K3bAudioTrack* _track, uint position )
{
	lastAddedPosition = position;
	
	if( !m_tracks.insert( position, _track ) ) {
		lastAddedPosition = m_tracks.count();
		m_tracks.insert( m_tracks.count(), _track );
	}
		
	emit newTrack( _track );
}

void K3bAudioProject::removeTrack( uint position )
{
	K3bAudioTrack* _track = take( position );
	if( _track ) {
		delete _track;
		emit trackRemoved( position );
	}
}

void K3bAudioProject::clear()
{
	m_tracks.setAutoDelete( true );
	m_tracks.clear();
	m_tracks.setAutoDelete( false );
}

void K3bAudioProject::moveTrack( uint oldPos, uint newPos )
{
	K3bAudioTrack* _track = m_tracks.take( oldPos );
	if(_track)
		m_tracks.insert( newPos, _track );
}
