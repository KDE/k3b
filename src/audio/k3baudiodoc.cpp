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

// QT-includes
#include <qstring.h>
#include <qstringlist.h>
#include <qfile.h>
#include <qdatetime.h>
#include <qtextstream.h>

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
	m_fileDecodingSuccessful = true;
	m_cdText = true;
	
	m_docType = AUDIO;
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
	
	// setting defaults that should come from k3bmain
	testFiles = false;

	m_cdText = true;
	m_padding = false;
	
	return K3bDoc::newDocument();
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
					emit percent( 100*made/(made+togo) );
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



int K3bAudioDoc::size(){
	// TODO: sum the sizes of all tracks, the pregaps, and leadin/leadout stuff
	
	return 700;
}


QTime K3bAudioDoc::audioSize() const
{
	//TODO: make it right!
	return QTime(1, 14);
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
	if( m_process->isRunning() )
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
		a->show();
	
		QString ending = addedFile.fileName().right( 7 );
		if( ending.contains("wav") ) {
			addWavFile( addedFile.path(), lastAddedPosition );
		}
		else if( ending.contains( "mp3" ) ) {
			addMp3File(  addedFile.path(), lastAddedPosition );
		}
		else {
			emit result();
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
	lastAddedPosition = position;
	
	addTrack( new K3bAudioTrack( m_tracks, fileName ), lastAddedPosition );
	emit percent( 100 );
	emit result();
	addNextTrack();
}

void K3bAudioDoc::addMp3File( const QString& fileName, uint position )
{
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
		emit result();
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
		emit result();
//		emitMessage( "mpg123 did exit with errors." );
		KMessageBox::information( kapp->mainWidget(), "mpg123 did exit with errors and returned: "
													+ QString::number(m_process->exitStatus() ), "Error", QString::null, false );		
	}
	
	else if( !(m_process->exitStatus() == 0 ) ) {
		delete m_lastAddedTrack;
		m_lastAddedTrack = 0;
		qDebug( "Mp3-File " + addedFile.path() + " is corrupt." );
		m_error = K3b::CORRUPT_MP3;
		emit result();
//		emitMessage( "Mp3-File " + addedFile.path() + " is corrupt." );
		KMessageBox::information( kapp->mainWidget(), "Could not decode mp3-file " + addedFile.fileName(), "Error", QString::null, false );		
	}
	else {	
		// here the file should be valid!
		addTrack( m_lastAddedTrack, lastAddedPosition );
	
		// some debugging shit...
		qDebug( m_lastAddedTrack->fileName() + " title: " + m_lastAddedTrack->artist() + " " + m_lastAddedTrack->title() );
	
		m_error = K3b::SUCCESS;
		emit result();
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
//		emitMessage( "removed track " + _track->fileName() );
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


K3bAudioBurnDialog* K3bAudioDoc::burnDialog()
{
	// test if there is something to burn
	if( m_tracks->count() == 0 ) {
		KMessageBox::information( kapp->mainWidget(), "There is nothing to burn!", "So what?", QString::null, false );
		return 0;
	}
	if( !m_burnDialog ) {
		m_burnDialog = new K3bAudioBurnDialog( this, k3bMain(), "audioBurnDialog", true );
	}
		
	return m_burnDialog;
}


QString K3bAudioDoc::writeTOC( const QString& filename )
{
	QFile file( filename );
	if( !file.open( IO_WriteOnly ) ) {
		qDebug( "(K3bAudioDoc) Could not open toc-file %s", filename.latin1() );
		return QString::null;
	}
	
	QTextStream t(&file);
	// --- writing the TOC -------------------
	// header
	t << "// TOC-file to use with cdrdao created by K3b" << "\n\n";
	t << "CD_DA\n\n";
	if( cdText() ) {
		t << "CD_TEXT {" << "\n";
		t << "  LANGUAGE_MAP { 0: EN }\n";
		t << "  LANGUAGE 0 {\n";
		t << "    TITLE " << "\"" << title() << "\"" << "\n";
		t << "    PERFORMER " << "\"" << artist() << "\"" << "\n";
		t << "  }" << "\n";
		t << "}" << "\n\n";
	}
	
	// tracks
	for( K3bAudioTrack* _track = at(0); _track != 0; _track = next() ) {
		t << "TRACK AUDIO" << "\n";
		if( cdText() ) {
			t << "CD_TEXT {" << "\n";
			t << "  LANGUAGE 0 {" << "\n";
			t << "    TITLE " << "\"" << _track->title() << "\"" << "\n";
			t << "    PERFORMER " << "\"" << _track->artist() << "\"" << "\n";
			t << "  }" << "\n";
			t << "}" << "\n";
		}
		if( _track->pregap() > 0 ) {
			t << "PREGAP 0:" << _track->pregap() << ":0" << "\n";
		}

		t << "FILE ";
		if( _track->bufferFile().isEmpty() ) {
			// TODO: test if MP3 and do something about it!
			t << "\"" << _track->absPath() << "\"" << " 0" << "\n";
		}
		else
			t << "\"" << _track->bufferFile() << "\"" << " 0" << "\n";
		t << "\n";
	}
	// --------------------------------- TOC --	
	
	file.close();
	return filename;
}


int K3bAudioDoc::numOfTracks()
{
	return m_tracks->count();
}


int K3bAudioDoc::allMp3Decoded()
{
	// check if all the files are converted to wav...
	 int iNumFilesToBuffer = 0;
	for( QListIterator<K3bAudioTrack> i(*m_tracks); i.current(); ++i )
	{
		if( i.current()->filetype() == K3b::MP3 && i.current()->bufferFile().isEmpty() )
			iNumFilesToBuffer++;
	}

	return iNumFilesToBuffer;
}

bool K3bAudioDoc::padding() const
{
	QListIterator<K3bAudioTrack> it(*m_tracks);
	for( ; it.current(); ++it )
		if( it.current()->filetype() == K3b::MP3 )
			return true;
	
	return m_padding;
}


K3bAudioTrack* K3bAudioDoc::nextTrackToDecode()
{
	for( QListIterator<K3bAudioTrack> i(*m_tracks); i.current(); ++i )
	{
		if( i.current()->filetype() == K3b::MP3 && i.current()->bufferFile().isEmpty() )
			return i.current();
	}
	return 0;
}
