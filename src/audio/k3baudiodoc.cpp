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

#include "../k3b.h"
#include "../k3bglobals.h"
#include "k3baudiodoc.h"
#include "k3baudioview.h"
#include "k3baudiotrack.h"
#include "../k3bprogressdialog.h"
#include "k3baudioburndialog.h"
#include "k3baudioburninfodialog.h"

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
#include <kconfig.h>
#include <klocale.h>
#include <kstddirs.h>

#include <iostream>


K3bAudioDoc::K3bAudioDoc( QObject* parent )
	: K3bDoc( parent )
{
	m_burnDialog = 0L;
	m_tracks = 0L;
}

K3bAudioDoc::~K3bAudioDoc()
{
	if( m_tracks )
		m_tracks->setAutoDelete( true );
		
	delete m_tracks;
}

bool K3bAudioDoc::newDocument()
{
	if( m_tracks )
		m_tracks->setAutoDelete( true );

	delete m_tracks;
	
	m_tracks = new QList<K3bAudioTrack>;
	m_tracks->setAutoDelete( false );
	
	m_currentProcessedTrack = 0L;
	m_lastAddedTrack = 0L;
	
	m_iNumFilesToBuffer = 0;
	m_iNumFilesAlreadyBuffered = 0;
	
	startBurningAfterBuffering = false;
	
	// setting defaults that should come from k3bmain
	testFiles = false;
	
	K3bDoc::newDocument();
	return true;
}

void K3bAudioDoc::prepareTracks()
{
	if( workInProgress() )
		return;

	m_currentProcessedTrack = at(0);

	bufferFiles();
}

void K3bAudioDoc::write()
{
	if( workInProgress() )
		return;

	// check if all the files are converted to wav...
	m_iNumFilesToBuffer = 0;
	for( K3bAudioTrack* i = at(0); i != 0; i = next() )
	{
		if( i->filetype() == K3b::MP3 && i->bufferFile().isEmpty() )
			m_iNumFilesToBuffer++;
	}

	if( m_iNumFilesToBuffer > 0 )
	{
		// there is some convertion work to do before burning...
		emitMessage( QString("There are %1 files to decode...").arg(m_iNumFilesToBuffer) );
		emit startDecoding();
		startBurningAfterBuffering = true;
		m_iNumFilesAlreadyBuffered = 0;
		m_currentProcessedTrack = at(0);
		bufferFiles();
	}
	else
	{
		startRecording();
	}
}

void K3bAudioDoc::writeImage( const QString& )
{
	// since cdrecord does not support this it will not work
	// before i implement it on my own (and i hope so!)
	qDebug("(K3bAudioDoc) Image creation not implemented yet!");
}

void K3bAudioDoc::parseCdrecordOutput( KProcess*, char* output, int len )
{
	QString buffer = QString::fromLatin1( output, len );
	
	// split to lines
	QStringList lines = QStringList::split( "\n", buffer );
	
	// do every line
	for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ )
	{
		*str = (*str).stripWhiteSpace();
		if( (*str).startsWith( "Track" ) )
		{
			qDebug("Parsing line [[" + *str + "]]" );
			
			if( (*str).contains( "fifo", false ) > 0 )
			{
				// parse progress
				int num, made, size, fifo;
				bool ok;

				// --- parse number of track ---------------------------------------				
				// ----------------------------------------------------------------------
				int pos1 = 5;
				int pos2 = (*str).find(':');
				if( pos1 == -1 ) {
					qDebug("parsing did not work");
					continue;
				}
				// now pos2 to the first colon :-)
				num = (*str).mid(pos1,pos2-pos1).toInt(&ok);				
				if(!ok)
					qDebug("parsing did not work");
				
				// --- parse already written Megs -----------------------------------				
				// ----------------------------------------------------------------------
				pos1 = (*str).find(':');
				if( pos1 == -1 ) {
					qDebug("parsing did not work");
					continue;
				}
				pos2 = (*str).find("of");
				if( pos2 == -1 ) {
					qDebug("parsing did not work");
					continue;
				}
				// now pos1 point to the colon and pos2 to the 'o' of 'of' :-)
				pos1++;
				made = (*str).mid(pos1,pos2-pos1).toInt(&ok);
				if(!ok)
					qDebug("parsing did not work");
					
				// --- parse total size of track ---------------------------------------
				// ------------------------------------------------------------------------
				pos1 = (*str).find("MB");
				if( pos1 == -1 ) {
					qDebug("parsing did not work");
					continue;
				}
				// now pos1 point to the 'M' of 'MB' and pos2 to the 'o' of 'of' :-)
				pos2 += 2;
				size = (*str).mid(pos2,pos1-pos2).toInt(&ok);
				if(!ok)
					qDebug("parsing did not work");
				
				// --- parse status of fifo --------------------------------------------
				// ------------------------------------------------------------------------
				pos1 = (*str).find("fifo");
				if( pos1 == -1 ) {
					qDebug("parsing did not work");
					continue;
				}
				pos2 = (*str).find('%');
				if( pos2 == -1 ) {
					qDebug("parsing did not work");
					continue;
				}
				// now pos1 point to the 'f' of 'fifo' and pos2 to the %o'  :-)
				pos1+=4;
				fifo = (*str).mid(pos1,pos2-pos1).toInt(&ok);
				if(!ok)
					qDebug("parsing did not work");

				// -------------------------------------------------------------------
				// -------- parsing finished --------------------------------------
																	
				double _trackPercent;
				if( size > 0 )
					_trackPercent = ((double)made/(double)size) * 100.0;
				else
					_trackPercent = 0;
					
				double _percentPerTrack = 100.0 / (double)( m_iNumFilesToBuffer + m_tracks->count() );
				// already done files
				double _overallPercent = _percentPerTrack * ( m_iNumFilesToBuffer + num -1 );
				// and the current file
				_overallPercent += _percentPerTrack * _trackPercent / 100.0;
				
				emit trackProcessedSize( made, size );
				emit trackPercent( (int)_trackPercent  );
				emit percent( (int)_overallPercent );
				
				emit bufferStatus( fifo );
				
				if( num > 0 )
					m_currentProcessedTrack = m_tracks->at(num);
						
			}
		}
		else if( (*str).startsWith( "Starting new" ) )
		{
			emit nextTrackProcessed();
			emitMessage( *str );
		}
		else {
			// debugging
//			emitMessage( *str );
		}
	} // for every line
}

void K3bAudioDoc::parseMpgTestingOutput( KProcess*, char* output, int len )
{
	QString buffer = QString::fromLatin1( output, len );
	
	// split to lines
	QStringList lines = QStringList::split( "\n", buffer );
	
	for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ )
	{
		if( testFiles && (*str).left(10).contains("Frame#") ) {
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


void K3bAudioDoc::parseMpgDecodingOutput( KProcess*, char* output, int len )
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
			togo = (*str).mid(15,5).toInt(&ok);
			if( !ok )
				qDebug("parsing did not work for " + (*str).mid(15,5) );
			
			double _trackPercent = (double)made/(double)(made+togo) * 100.0;
			double _percentPerTrack = 100.0 / (double)( m_iNumFilesToBuffer + m_tracks->count() );
			// already done files
			double _overallPercent = _percentPerTrack * m_iNumFilesAlreadyBuffered;
			// and the current file
			_overallPercent += _percentPerTrack * _trackPercent / 100.0;
			
			emit trackProcessedSize( made, made+togo );
			emit trackPercent( (int)_trackPercent  );
			emit percent( (int)_overallPercent );
		}
//		else {
//			// debugging
//			emitMessage( *str );
//		}
	}
}


void K3bAudioDoc::cdrecordFinished()
{
	if( m_process->normalExit() )
	{
		// TODO: check the process' exitStatus()
		switch( m_process->exitStatus() )
		{
			case 0:
				m_error = K3b::SUCCESS;
				emitMessage( "Burning successfully finished" );
				break;
				
			default:
				// no recording device and also other errors!! :-(
				emitMessage( "Cdrecord returned some error!" );
				m_error = K3b::CDRECORD_ERROR;
				break;
		}
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
	m_process->disconnect();

	m_currentProcessedTrack = at(0);

	kapp->config()->setGroup("External Programs");	
	// OK, we need a new cdrecord process...
	m_process->clearArguments();
	*m_process << kapp->config()->readEntry( "cdrecord path" );

	// and now we add the needed arguments...
	// display progress
	*m_process << "-v";

	if( dummy() )
		*m_process << "-dummy";
	if( dao() )
		*m_process << "-dao";
	if( eject() )
		*m_process << "-eject";

	// test if padding is nessessary
	// padding is enabled by default if any mp3 files have been converted!
	if( padding() || m_iNumFilesToBuffer > 0 )
		*m_process << "-pad";
	
	// add speed
	QString s = QString("-speed=%1").arg( speed() );
	*m_process << s;

	// add the device
//	s = "-dev=" + burner()->device;
//	*m_process << s;
	*m_process << "-dev=0,0,0";

//	*m_process << "-audio";

	// add all the tracks
	for( K3bAudioTrack* i = at(0); i != 0; i = next() )
	{
		s = QString("-pregap=%1").arg( i->pregap() );
		*m_process << s;
		if( !i->bufferFile().isEmpty() )
			*m_process << i->bufferFile();
		else
			*m_process << i->absPath();
	}

	// debugging output
	QStrList* _args = m_process->args();
	QStrListIterator _it(*_args);
	while( _it ) {
		cout << *_it << " ";
		++_it;
	}
	cout << flush;
	
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
		connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
				 this, SLOT(parseCdrecordOutput(KProcess*, char*, int)) );
		connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
				 this, SLOT(parseCdrecordOutput(KProcess*, char*, int)) );
		
		m_error = K3b::WORKING;
		emitMessage( QString("Start recording at %1x speed...").arg(speed()) );
		emit startWriting();
	}
}


void K3bAudioDoc::bufferFiles( )
{
	// find first file to buffer
	m_currentProcessedTrack = at(0);
	while( m_currentProcessedTrack &&
		  !( m_currentProcessedTrack->filetype() == K3b::MP3 && m_currentProcessedTrack->bufferFile().isEmpty() ) )
		m_currentProcessedTrack = next();
	
	m_process->disconnect();
	
	if( m_currentProcessedTrack == 0L )
	{
		if( startBurningAfterBuffering )
			startRecording();
		else
		{
			m_error = K3b::SUCCESS;
			emitResult();
		}
		return;
	}

	kapp->config()->setGroup( "External Programs");
	
	// start a new mpg123 process for this track
	m_process->clearArguments();
	*m_process << kapp->config()->readEntry( "mpg123 path" );
	
	kapp->config()->setGroup( "General Options");
	lastTempFile = findTempFile( "wav", k3bMain()->config()->readEntry( "Temp Dir", locateLocal( "appdata", "temp/" ) ) );
	*m_process << "-v";
	*m_process << "-w";
	*m_process << lastTempFile;
	*m_process << m_currentProcessedTrack->absPath();

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
		connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
				 this, SLOT(parseMpgDecodingOutput(KProcess*, char*, int)) );
		connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
				 this, SLOT(parseMpgDecodingOutput(KProcess*, char*, int)) );
		
		emitMessage( "Decoding file " + m_currentProcessedTrack->fileName() );
		emit nextTrackProcessed();
		m_error = K3b::WORKING;
	}
}


void K3bAudioDoc::fileBufferingFinished()
{
	// check if process exited successfully and if so:
	m_process->disconnect();
	
	if( !m_process->normalExit() ) {
		// do some shit
		qDebug( "mpg123 did exit with errors." );
		m_error = K3b::MPG123_ERROR;
		emitMessage( "mpg123 did exit with errors." );
	}
	
	else if( !(m_process->exitStatus() == 0 ) ) {
		delete m_lastAddedTrack;
		qDebug( "Mp3-File " + m_currentProcessedTrack->absPath() + " is corrupt." );
		m_error = K3b::CORRUPT_MP3;
		emitMessage( "Mp3-File " + m_currentProcessedTrack->fileName() + " is corrupt." );
	}
	else {	
		// here the file should be valid!
		m_currentProcessedTrack->setBufferFile( lastTempFile );
		m_iNumFilesAlreadyBuffered++;
	
		m_error = K3b::SUCCESS;
	}
	
	bufferFiles();
}



int K3bAudioDoc::size(){
	// TODO: sum the sizes of all tracks, the pregaps, and leadin/leadout stuff
	
	return 76;
}


QTime K3bAudioDoc::audioSize() const
{
	//TODO: make it right!
	return QTime();
}


void K3bAudioDoc::addTracks(const QStringList& urls, uint position )
{
	// TODO: addNextTrack() should be called whenever there is nothing else to do and urlsToAdd is not empty!
	
	for( QStringList::ConstIterator it = urls.begin(); it != urls.end(); it++ ) {
		urlsToAdd.enqueue( new PrivateUrlToAdd( *it, position++ ) );
		cerr <<  "adding url to queue: " << *it;
	}
	addNextTrack();
}

void K3bAudioDoc::addNextTrack()
{
	if( workInProgress() )
		return;

	if( !urlsToAdd.isEmpty() ) {
		PrivateUrlToAdd* _item = urlsToAdd.dequeue();
		lastAddedPosition = _item->position;
		
		// append at the end by default
		if( lastAddedPosition > m_tracks->count() )
			lastAddedPosition = m_tracks->count();
	
		addedFile = KURL( _item->url );
		delete _item;
		
		if( !addedFile.isValid() ) {
			qDebug( addedFile.path() + " not valid" );
			return;
		}
	
		if( !QFile::exists( addedFile.path() ) ) {
			KMessageBox::information( kapp->mainWidget(), "File not found: " + addedFile.fileName(), "Error", QString::null, false );
			return;
		}
	
		K3bProgressDialog* a = new K3bProgressDialog( addedFile.fileName(), k3bMain()  );
		connect( this, SIGNAL(result()), a, SLOT(close()) );
		connect( this, SIGNAL(percent(int)), a, SLOT(setPercent(int)) );
		connect( a, SIGNAL(canceled()), this, SLOT(cancel()) );
		a->show();
	
		QString ending = addedFile.fileName().right( 7 );
		if( ending.contains("wav") ) {
			addWavFile( addedFile.path(), lastAddedPosition );
		}
		else if( ending.contains( "mp3" ) ) {
			addMp3File(  addedFile.path(), lastAddedPosition );
		}
		else {
			emitResult();
			KMessageBox::information( kapp->mainWidget(), "Only mp3 and wav audio files are supported!", "Wrong file format", QString::null, false );		
		}
	}
}

void K3bAudioDoc::addTrack(const QString& url, uint position )
{
	urlsToAdd.enqueue( new PrivateUrlToAdd( url, position ) );
	addNextTrack();
}

void K3bAudioDoc::addWavFile( const QString& fileName, uint position )
{
	// TODO: test the wavfile
	// for now without any testing
	// at this point we assume that the file exists
	emitMessage( "Adding wav-audio file " + fileName + " to project." );
	lastAddedPosition = position;
	
	addTrack( new K3bAudioTrack( m_tracks, fileName ), lastAddedPosition );
	emitProgress( 100, 100 );
	emitResult();
	addNextTrack();
}

void K3bAudioDoc::addMp3File( const QString& fileName, uint position )
{
	emitMessage( "Adding mp3-audio file " + fileName + " to project." );

	// create a new K3bAudioTrack without knowing if we can use it
	// but it will be easier for further shit
	m_lastAddedTrack = new K3bAudioTrack( m_tracks, fileName );
	
	lastAddedPosition = position;
	
	kapp->config()->setGroup( "External Programs" );
	
	// test the file with mpg123
	// at this point we assume that the file exists
	m_process->disconnect();
	m_process->clearArguments();
	*m_process << kapp->config()->readEntry( "mpg123 path" );
	*m_process << "-v";
	*m_process << "-t";
	if( !testFiles )
		*m_process << "-n1";
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
		addNextTrack();
	}
	else
	{
		// connect to the mpg123-slots
		connect( m_process, SIGNAL(processExited(KProcess*)),
				 this, SLOT(mp3FileTestingFinished()) );
//		connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
//				 this, SLOT(parseMpgTestingOutput(KProcess*, char*, int)) );
 		connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
				 this, SLOT(parseMpgTestingOutput(KProcess*, char*, int)) );
		
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
	}
	
	else if( !(m_process->exitStatus() == 0 ) ) {
		delete m_lastAddedTrack;
		m_lastAddedTrack = 0;
		qDebug( "Mp3-File " + addedFile.path() + " is corrupt." );
		m_error = K3b::CORRUPT_MP3;
		emitResult();
		emitMessage( "Mp3-File " + addedFile.path() + " is corrupt." );
		KMessageBox::information( kapp->mainWidget(), "Could not decode mp3-file " + addedFile.fileName(), "Error", QString::null, false );		
	}
	else {	
		// here the file should be valid!
		addTrack( m_lastAddedTrack, lastAddedPosition );
	
		// some debugging shit...
		qDebug( m_lastAddedTrack->fileName() + " title: " + m_lastAddedTrack->artist() + " " + m_lastAddedTrack->title() );
	
		m_error = K3b::SUCCESS;
		emitResult();
	}
	
	addNextTrack();
}


void K3bAudioDoc::addTrack( K3bAudioTrack* _track, uint position )
{
	lastAddedPosition = position;
	
	if( !m_tracks->insert( position, _track ) ) {
		lastAddedPosition = m_tracks->count();
		m_tracks->insert( m_tracks->count(), _track );
	}
	
	emit newTrack( _track );
	setModified( true );
}

void K3bAudioDoc::removeTrack( int position )
{
	if( position < 0 ) {
		qDebug( "(K3bAudioDoc) tried to remove track with index < 0!" );
		return;
	}
	
	K3bAudioTrack* _track = take( position );
	if( _track ) {
		emitMessage( "removed track " + _track->fileName() );
		delete _track;
		emit trackRemoved( position );
	}
}

void K3bAudioDoc::moveTrack( uint oldPos, uint newPos )
{
	K3bAudioTrack* _track = m_tracks->take( oldPos );
	if(_track)
		m_tracks->insert( newPos, _track );
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
	connect( v, SIGNAL(dropped(const QStringList&, uint)), this, SLOT(addTracks(const QStringList&, uint)) );
	connect( this, SIGNAL(newTrack(K3bAudioTrack*)), v, SLOT(addItem(K3bAudioTrack*)) );
	connect( v, SIGNAL(itemMoved(uint,uint)), this, SLOT(moveTrack(uint,uint)) );
	
	K3bDoc::addView( view );
}

void K3bAudioDoc::slotTestOutput( const QString& text ){
	qDebug( text );
}


void K3bAudioDoc::showBurnDialog()
{
	// test if there is something to burn
	if( m_tracks->count() == 0 ) {
		KMessageBox::information( kapp->mainWidget(), "There is nothing to burn!", "So what?", QString::null, false );
		return;
	}
	if( !m_burnDialog ) {
		m_burnDialog = new K3bAudioBurnDialog( this, k3bMain(), "audioBurnDialog", true );
	}
		
	if( m_burnDialog->exec( true ) == K3bAudioBurnDialog::Burn ) {
		K3bAudioBurnInfoDialog _test( this, k3bMain() );
		connect( &_test, SIGNAL(cancelPressed()), this, SLOT(cancel()) );
		write();
		_test.show();
	}
}
