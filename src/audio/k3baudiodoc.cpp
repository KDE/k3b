/***************************************************************************
                          k3baudiodoc.cpp  -  description
                             -------------------
    begin                : Tue Mar 27 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3bglobals.h"
#include "k3baudiodoc.h"
#include "k3baudioview.h"
#include "k3baudiotrack.h"
#include "k3bprogressdialog.h"

// QT-includes
#include <qstring.h>
#include <qstringlist.h>
#include <qfile.h>
#include <qdatetime.h>

// KDE-includes
#include <kprocess.h>
#include <kurl.h>
#include <kapp.h>
#include <kmessagebox.h>


K3bAudioDoc::K3bAudioDoc( const QString& cdrecord, const QString& mpg123 )
	: K3bDoc( cdrecord ), m_mpg123( mpg123 )
{
	m_currentBufferedTrack = 0;
	m_lastAddedTrack = 0;
	
	startBurningAfterBuffering = false;

	if( !QFile::exists( m_mpg123 ) )
		qDebug( "(K3bAudioDoc) could not find mpg123!" );
}

K3bAudioDoc::~K3bAudioDoc()
{
	m_tracks.setAutoDelete( true );
}

void K3bAudioDoc::prepareTracks()
{
	if( workInProgress() )
		return;

	m_currentBufferedTrack = at(0);

	bufferFiles();
}

void K3bAudioDoc::write()
{
	if( workInProgress() )
		return;

	// check if all the files are converted to wav...
	bool _allConv = true;
	for( K3bAudioTrack* i = at(0); i != 0 && _allConv; i = next() )
	{
		if( i->filetype() == K3b::MP3 && i->bufferFile().isEmpty() )
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

void K3bAudioDoc::writeImage( const QString& filename )
{
	// since cdrecord does not support this it will not work
	// before i implement it on my own (and i hope so!)
	qDebug("(K3bAudioDoc) Image creation not implemented yet!");
}

void K3bAudioDoc::parseCdrecordOutput( KProcess* process, char* output, int len )
{
	// simple shit for the beginning
	qDebug( QString::fromLatin1( output, len ) );
}

void K3bAudioDoc::parseMpg123Output( KProcess* process, char* output, int len )
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


void K3bAudioDoc::cdrecordFinished()
{
	if( m_process->normalExit() )
	{
		// TODO: check the process' exitStatus()
		m_error = K3b::SUCCESS;
		emitMessage( "Burning finished" );
	}
	else
	{
		m_error = K3b::CDRECORD_ERROR;
		emitMessage( "cdrecord did not exit cleanly!" );
	}

	emitResult();

	m_process->disconnect();
}

void K3bAudioDoc::startRecording()
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
		qDebug("(K3bAudioDoc) could not start cdrecord");
		m_error = K3b::CDRECORD_ERROR;
		emitMessage( "could not start cdrecord!" );
		emitResult();
	}
	else
	{
		// connect to the cdrecord slots
		connect( m_process, SIGNAL(processExited(KProcess*)),
				 this, SLOT(cdrecordFinished()) );
		connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
				 this, SLOT(parseCdrecordOutput(char*, int)) );
		m_error = K3b::WORKING;
	}
}


void K3bAudioDoc::bufferFiles( )
{
	// find first file to buffer
	while( m_currentBufferedTrack &&
		  !( m_currentBufferedTrack->filetype() == K3b::MP3 && m_currentBufferedTrack->bufferFile().isEmpty() ) )
		m_currentBufferedTrack = next();

	if( m_currentBufferedTrack == 0 )
	{
		m_process->disconnect();
		if( startBurningAfterBuffering )
			startRecording();
		else
		{
			m_error = K3b::SUCCESS;
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
		m_error = K3b::MPG123_ERROR;
		emitResult();
	}
	else
	{
		// connect to the mpg123-slots
		connect( m_process, SIGNAL(processExited(KProcess*)),
				 this, SLOT(fileBufferingFinished()) );
		connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
				 this, SLOT(parseMpg123Output(char*, int)) );

		emitMessage( "Decoding file " + m_currentBufferedTrack->fileName() );
		m_error = K3b::WORKING;
	}
}


void K3bAudioDoc::fileBufferingFinished()
{
	// check if process exited successfully and if so:
	m_currentBufferedTrack->setBufferFile( lastTempFile );

	bufferFiles();
}



int K3bAudioDoc::size(){
	// TODO: sum the sizes of all tracks, the pregaps, and leadin/leadout stuff
	return 0;
}


void K3bAudioDoc::slotAddTrack(const QString& url, uint position )
{
	if( workInProgress() )
		return;

	// TODO: parse multible urls
	
	// append at the end by default
	if( position > m_tracks.count() )
		position = m_tracks.count();
	
	addedFile = KURL( url );
		
	if( !addedFile.isValid() ) {
		qDebug( addedFile.path() + " not valid" );
		return;
	}
	
	if( !QFile::exists( addedFile.path() ) ) {
		KMessageBox::information( kapp->mainWidget(), "File not found: " + addedFile.fileName(), "Error", QString::null, false );
		return;
	}
	
	K3bProgressDialog* a = new K3bProgressDialog( addedFile.fileName(), kapp->mainWidget()  );
	connect( this, SIGNAL(result()), a, SLOT(close()) );
	connect( this, SIGNAL(percent(int)), a, SLOT(setPercent(int)) );
	connect( a, SIGNAL(canceled()), this, SLOT(cancel()) );
	a->show();
	
	QString ending = addedFile.fileName().right( 7 );
	if( ending.contains("wav") ) {
		addWavFile( addedFile.path(), position );
	}
	else if( ending.contains( "mp3" ) ) {
		addMp3File(  addedFile.path(), position );
	}
	else {
		emitResult();
		KMessageBox::information( kapp->mainWidget(), "Only mp3 and wav audio files are supported!", "Wrong file format", QString::null, false );		
	}
}

void K3bAudioDoc::addWavFile( const QString& fileName, uint position )
{
	// TODO: test the wavfile
	// for now without any testing
	// at this point we assume that the file exists
	emitMessage( "Adding wav-audio file " + fileName + " to project." );
	lastAddedPosition = position;
	
	addTrack( new K3bAudioTrack( &m_tracks, fileName ), lastAddedPosition );
	emitProgress( 100, 100 );
	emitResult();
}

void K3bAudioDoc::addMp3File( const QString& fileName, uint position )
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
		qDebug("Could not start mpg123.");
		m_error = K3b::MPG123_ERROR;
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
		
		emitMessage( "Testing file " + fileName );
	}
}


void K3bAudioDoc::mp3FileTestingFinished()
{
	m_process->disconnect();
	
	if( !m_process->normalExit() ) {
		// do some shit
		qDebug( "mpg123 did exit with errors." );
		delete m_lastAddedTrack;
		m_lastAddedTrack = 0;
		m_error = K3b::MPG123_ERROR;
		emitResult();
		emitMessage( "mpg123 did exit with errors." );
		KMessageBox::information( kapp->mainWidget(), "mpg123 did exit with errors and returned: "
													+ QString::number(m_process->exitStatus() ), "Error", QString::null, false );		
		return;
	}
	
	// check the process' errorlevel
	if( !(m_process->exitStatus() == 0 ) ) {
		delete m_lastAddedTrack;
		m_lastAddedTrack = 0;
		qDebug( "Mp3-File " + addedFile.path() + " is corrupt." );
		m_error = K3b::CORRUPT_MP3;
		emitResult();
		emitMessage( "Mp3-File " + addedFile.path() + " is corrupt." );
		KMessageBox::information( kapp->mainWidget(), "Could not decode mp3-file " + addedFile.fileName(), "Error", QString::null, false );		
		return;
	}
	
	// here the file should be valid!
	addTrack( m_lastAddedTrack, lastAddedPosition );
	
	// some debugging shit...
	qDebug( m_lastAddedTrack->fileName() + " title: " + m_lastAddedTrack->artist() + " " + m_lastAddedTrack->title() );
	
	m_error = K3b::SUCCESS;
	emitResult();
}


void K3bAudioDoc::addTrack( K3bAudioTrack* _track, uint position )
{
	lastAddedPosition = position;
	
	if( !m_tracks.insert( position, _track ) ) {
		lastAddedPosition = m_tracks.count();
		m_tracks.insert( m_tracks.count(), _track );
	}
	
	emit newTrack( _track );
}

void K3bAudioDoc::removeTrack( uint position )
{
	K3bAudioTrack* _track = take( position );
	if( _track ) {
		emitMessage( "removed track " + _track->fileName() );
		delete _track;
		emit trackRemoved( position );
	}
}

void K3bAudioDoc::moveTrack( uint oldPos, uint newPos )
{
	K3bAudioTrack* _track = m_tracks.take( oldPos );
	if(_track)
		m_tracks.insert( newPos, _track );
}

K3bView* K3bAudioDoc::newView( QWidget* parent )
{
	return new K3bAudioView( this, parent );
}


bool K3bAudioDoc::loadDocumentData( QFile& f )
{
	// TODO: so what? load the shit! ;-)
	return true;
}

bool K3bAudioDoc::saveDocumentData( QFile& f )
{
	// TODO: some saving work...
	return true;
}


void K3bAudioDoc::addView(K3bView* view)
{
	K3bAudioView* v = (K3bAudioView*)view;
	connect( v, SIGNAL(dropped(const QString&, uint)), this, SLOT(slotAddTrack(const QString&, uint)) );
	connect( this, SIGNAL(newTrack(K3bAudioTrack*)), v, SLOT(addItem(K3bAudioTrack*)) );
	connect( v, SIGNAL(itemMoved(uint,uint)), this, SLOT(moveTrack(uint,uint)) );
}

void K3bAudioDoc::slotTestOutput( const QString& text ){
	qDebug( text );
}
