/***************************************************************************
                          k3baudiojob.cpp  -  description
                             -------------------
    begin                : Thu May 3 2001
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

#include "k3baudiojob.h"

#include "../k3b.h"
#include "../k3bglobals.h"
#include "../k3bdoc.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "k3bmp3decodingjob.h"

#include <kprocess.h>
#include <klocale.h>
#include <kconfig.h>

#include <qstring.h>

#include <iostream>
#include <cmath>

K3bAudioJob::K3bAudioJob( K3bAudioDoc* doc )
	: K3bJob( )
{
	m_doc = doc;
	m_mp3Job = 0L;
}


K3bAudioJob::~K3bAudioJob()
{
	if( m_mp3Job )
		delete m_mp3Job;
}

void K3bAudioJob::slotParseCdrecordOutput( KProcess*, char* output, int len )
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
//			qDebug("Parsing line [[" + *str + "]]" );
			
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

				if( num )
					m_iNumTracksAlreadyWritten = num -1 ;
																			
				slotEmitProgress( made, size );
				emit bufferStatus( fifo );
				
			}
		}
		else if( (*str).startsWith( "Starting new" ) )
		{
			emit newTrack();
			if(!firstTrack)
				m_iNumTracksAlreadyWritten++;
			else
				firstTrack = false;
				
			emit newSubTask( i18n("Writing track %1: '%2'").arg(m_iNumTracksAlreadyWritten + 1).arg(m_doc->at(m_iNumTracksAlreadyWritten)->fileName()) );
			emit infoMessage( *str );
		}
		else {
			// debugging
//			emitMessage( *str );
		}
	} // for every line
}


void K3bAudioJob::slotParseCdrdaoOutput( KProcess*, char* output, int len )
{
	QString buffer = QString::fromLatin1( output, len );
	
	// split to lines
	QStringList lines = QStringList::split( "\n", buffer );
	
	// do every line
	for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ )
	{
		*str = (*str).stripWhiteSpace();
		if( (*str).startsWith( "Warning" ) || (*str).startsWith( "ERROR" ) ) {
			// TODO: parse the error messages!!
			emit infoMessage( *str );
		}
		else if( (*str).startsWith( "Writing" ) ) {
			// a new track has been started
			emit newTrack();
			if(!firstTrack)
				m_iNumTracksAlreadyWritten++;
			else
				firstTrack = false;
			
			emit newSubTask( i18n("Writing track %1: '%2'").arg(m_iNumTracksAlreadyWritten + 1).arg(m_doc->at(m_iNumTracksAlreadyWritten)->fileName()) );
			emit infoMessage( *str );
		}
		else if( (*str).contains( "Wrote " ) ) {   // here "contains" has to be used since cdrdao sometimes "forgets" to do a newline!!
			// percentage
			int made, size, fifo;
			bool ok;
			
			// --- parse already written mb ------
			int pos1 = 6;
			int pos2 = (*str).find("of");
			
			if( pos2 == -1 )
				return; // there is one line at the end of the writing process that has no 'of'
			
			made = (*str).mid( 6, pos2-pos1-1 ).toInt( &ok );
			if( !ok )
				qDebug( "Parsing did not work for: " + (*str).mid( 6, pos2-pos1-1 ) );
			
			// ---- parse size ---------------------------
			pos1 = pos2 + 2;
			pos2 = (*str).find("MB");
			size = (*str).mid( pos1, pos2-pos1-1 ).toInt(&ok);
			if( !ok )
				qDebug( "Parsing did not work for: " + (*str).mid( pos1, pos2-pos1-1 ) );
				
			// ----- parsing fifo ---------------------------
			pos1 = (*str).findRev(' ');
			pos2 =(*str).findRev('%');
			fifo = (*str).mid( pos1, pos2-pos1 ).toInt(&ok);
			if( !ok )
				qDebug( "Parsing did not work for: " + (*str).mid( pos1, pos2-pos1 ) );
			
			emit bufferStatus( fifo );
			double _w = (double)m_doc->numOfTracks() / (double)(m_iNumFilesToDecode + m_doc->numOfTracks());
			emit percent( (int)((100.0-100.0*_w) + _w * 100.0 * (double)made/(double)size) );  // HACK!!
			emit processedSize( made, size );
			
			// HACK: calculating equal sizes for all tracks:
			// -----------------------------------------------------------------------------
			double trackSize = (double)size / (double)m_doc->numOfTracks();
			double trackMade = fmod((double)made, trackSize);
			if( made != size ) {
				emit subPercent( (int)( 100.0 * trackMade/trackSize ) );
				emit processedSubSize( (int)trackMade, (int)trackSize );
			}
			// ---------------------------------------------------------------- HACK ----
		}
		else if( (*str).startsWith( "Executing power" ) ) {
			emit infoMessage( i18n( *str ) );
			emit newSubTask( i18n(*str) );
		}
		else {
			// debugging
			qDebug(*str);
		}
	}
}


void K3bAudioJob::cancel()
{
	if( error() == K3b::WORKING ) {
		m_process.kill();
		emit infoMessage("Writing canceled.");
		if( m_mp3Job )
			if( m_mp3Job->error() == K3b::WORKING )
				m_mp3Job->cancel();
	
		// remove toc-file
		if( QFile::exists( m_tocFile ) ) {
			qDebug("(K3bAudioJob) Removing temporary TOC-file");
			QFile::remove( m_tocFile );
			m_tocFile = QString::null;
		}
				
		m_error = K3b::CANCELED;
		emit finished( this );
	}
}

void K3bAudioJob::start()
{
	m_iNumFilesToDecode = 0;
	m_iNumFilesAlreadyDecoded = 0;
	m_iNumTracksAlreadyWritten = 0;
	
	emit started();
	
	m_error = K3b::WORKING;
	
	if( (m_iNumFilesToDecode = m_doc->allMp3Decoded()) > 0 ) {
		emit infoMessage( i18n("There are %1 files to decode...").arg(m_iNumFilesToDecode) );
		
		// now use K3bMp3DecodingJob to decode all files
		emit newTask( "Decoding" );
		decodeNextFile();
	}
	else
		startWriting();
}


void K3bAudioJob::slotCdrecordFinished()
{
	if( m_process.normalExit() )
	{
		// TODO: check the process' exitStatus()
		switch( m_process.exitStatus() )
		{
			case 0:
				m_error = K3b::SUCCESS;
				emit infoMessage( "Burning successfully finished" );
				break;
				
			default:
				// no recording device and also other errors!! :-(
				emit infoMessage( "Cdrecord returned some error!" );
				m_error = K3b::CDRECORD_ERROR;
				break;
		}
	}
	else
	{
		m_error = K3b::CDRECORD_ERROR;
		emit infoMessage( "cdrecord did not exit cleanly!" );
	}

	emit finished( this );

	m_process.disconnect();
}


void K3bAudioJob::slotCdrdaoFinished()
{
	if( m_process.normalExit() )
	{
		// TODO: check the process' exitStatus()
		switch( m_process.exitStatus() )
		{
			case 0:
				m_error = K3b::SUCCESS;
				emit infoMessage( "Burning successfully finished" );
				break;
				
			default:
				// no recording device and also other errors!! :-(
				emit infoMessage( "Cdrdao returned some error!" );
				m_error = K3b::CDRDAO_ERROR;
				break;
		}
	}
	else
	{
		m_error = K3b::CDRDAO_ERROR;
		emit infoMessage( "cdrdao did not exit cleanly!" );
	}

	// remove toc-file
	qDebug("(K3bAudioJob) Removing temporary TOC-file");
	QFile::remove( m_tocFile );
	m_tocFile = QString::null;
	
	emit finished( this );

	m_process.disconnect();
}


void K3bAudioJob::decodeNextFile()
{
	// to decode the next mp3-file first get the next track from the doc
	m_currentProcessedTrack = m_doc->nextTrackToDecode();
	if( !m_currentProcessedTrack ) {
		emit infoMessage( i18n("Start writing...") );
		startWriting();
	}
	else {
		// here we start a K3bMp3DecodingJob
		if( !m_mp3Job ) {
			m_mp3Job = new K3bMp3DecodingJob( m_currentProcessedTrack->absPath() );
			
			// connect the signals
//			connect( m_mp3Job, SIGNAL(percent(int)), this, SIGNAL(subPercent(int)) );
			connect( m_mp3Job, SIGNAL(processedSize(int, int)), this, SLOT(slotEmitProgress(int, int)) );
			connect( m_mp3Job, SIGNAL(infoMessage(const QString&)), this, SIGNAL(infoMessage(const QString&)) );
			connect( m_mp3Job, SIGNAL(finished(K3bJob*)), this, SLOT(slotMp3JobFinished()) );
			// TODO: overall progress
		}
		else
			m_mp3Job->setSourceFile( m_currentProcessedTrack->absPath() );

		emit newSubTask( i18n("Decoding file '%1'").arg( m_currentProcessedTrack->fileName() ) );
		m_mp3Job->start();
	}
}

void K3bAudioJob::startWriting()
{
	emit newTask( "Writing" );
	emit newSubTask( i18n("Preparing write process...") );

	m_process.clearArguments();
	m_process.disconnect();
	
	firstTrack = true;
	
	if( m_doc->cdText() ) {
		// write in dao-mode
		if( !m_doc->dao() ) {
			emit infoMessage( "CD-Text is only supported in DAO-mode.");
			emit infoMessage("Swiching to DAO-mode.");
			m_doc->setDao( true );
		}
		
		// use cdrdao to burn the cd
		emit infoMessage( "Writing TOC-file" );
		m_tocFile = m_doc->writeTOC( k3bMain()->findTempFile( "toc" ) );
		if( m_tocFile.isEmpty() ) {
			emit infoMessage( "Could not write TOC-file." );
			m_error = K3b::IO_ERROR;
			emit finished( this );
		}
		else {
			// start a new cdrdao process
			kapp->config()->setGroup("External Programs");
			m_process << kapp->config()->readEntry( "cdrdao path" );
			m_process << "write";

			// device
			m_process << "--device" << m_doc->burner()->device();
			
			// additional parameters from config
			QStringList _params = kapp->config()->readListEntry( "cdrdao parameters" );
			for( QStringList::Iterator it = _params.begin(); it != _params.end(); ++it )
				m_process << *it;
			if( _params.isEmpty() ) // default
				m_process << "--driver" << "generic-mmc";    // !! use generic-mmc as default, TODO: give the option to choose
			
			// supress the 10 seconds gap to the writing
			m_process << "-n";
			
			if( m_doc->dummy() )
				m_process << "--simulate";
			if( k3bMain()->eject() )
				m_process << "--eject";
			
			// writing speed
			m_process << "--speed" << QString::number(  m_doc->speed() );
			
			// toc-file
			m_process << m_tocFile;
			
			// debugging output
			QStrList* _args = m_process.args();
			QStrListIterator _it(*_args);
			while( _it ) {
				cout << *_it << " ";
				++_it;
			}
			cout << flush;
	
			// connect to the cdrdao slots
			connect( &m_process, SIGNAL(processExited(KProcess*)),
					 this, SLOT(slotCdrdaoFinished()) );
			connect( &m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
					 this, SLOT(slotParseCdrdaoOutput(KProcess*, char*, int)) );
			connect( &m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
					 this, SLOT(slotParseCdrdaoOutput(KProcess*, char*, int)) );
		
			if( !m_process.start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
			{
				// something went wrong when starting the program
				// it "should" be the executable
				qDebug("(K3bAudioJob) could not start cdrdao");
				m_error = K3b::CDRDAO_ERROR;
				
				// remove toc-file
				QFile::remove( m_tocFile );
				m_tocFile = QString::null;

				emit infoMessage( "could not start cdrdao!" );
				emit finished( this );
			}
			else
			{
				m_error = K3b::WORKING;
				emit infoMessage( i18n("Start recording at %1x speed...").arg(m_doc->speed()) );
				emit started();
			}
		}
		
	}
	else {
	
	// OK, we need a new cdrecord process...
	kapp->config()->setGroup("External Programs");
	m_process << kapp->config()->readEntry( "cdrecord path" );

	// and now we add the needed arguments...
	// display progress
	m_process << "-v";

	if( m_doc->dummy() )
		m_process << "-dummy";
	if( m_doc->dao() )
		m_process << "-dao";
	if( k3bMain()->eject() )
		m_process << "-eject";

	// add speed
	QString s = QString("-speed=%1").arg( m_doc->speed() );
	m_process << s;

	// add the device
	s = QString("-dev=%1").arg( m_doc->burner()->device() );
	m_process << s;
	
	// test if padding is nessessary
	// padding is enabled by default if any mp3 files have been converted!
	if( m_doc->padding() )
		m_process << "-pad";

	// additional parameters from config
	QStringList _params = kapp->config()->readListEntry( "cdrecord parameters" );
	for( QStringList::Iterator it = _params.begin(); it != _params.end(); ++it )
		m_process << *it;

				
	// add all the tracks
	for( K3bAudioTrack* i = m_doc->at(0); i != 0; i = m_doc->next() )
	{
		s = QString("-pregap=%1").arg( i->pregap() );
		m_process << s;
		if( !i->bufferFile().isEmpty() )
			m_process << i->bufferFile();
		else
			m_process << i->absPath();
	}

	
	// debugging output
	QStrList* _args = m_process.args();
	QStrListIterator _it(*_args);
	while( _it ) {
		cout << *_it << " ";
		++_it;
	}
	cout << flush;
	
	// connect to the cdrecord slots
	connect( &m_process, SIGNAL(processExited(KProcess*)),
			 this, SLOT(slotCdrecordFinished()) );
	connect( &m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
			 this, SLOT(slotParseCdrecordOutput(KProcess*, char*, int)) );
	connect( &m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
			 this, SLOT(slotParseCdrecordOutput(KProcess*, char*, int)) );
	
	if( !m_process.start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
	{
		// something went wrong when starting the program
		// it "should" be the executable
		qDebug("(K3bAudioJob) could not start cdrecord");
		m_error = K3b::CDRECORD_ERROR;
		emit infoMessage( "could not start cdrecord!" );
		emit finished( this );
	}
	else
	{
		m_error = K3b::WORKING;
		emit infoMessage( i18n("Start recording at %1x speed...").arg(m_doc->speed()) );
		emit started();
	}
	
	}

}


void K3bAudioJob::slotMp3JobFinished()
{
	// if the mp3-decoding failed stop the main job
	if( m_mp3Job->error() != K3b::SUCCESS ) {
		emit infoMessage( i18n("Decoding failed! Process canceled!") );
		m_error = K3b::CORRUPT_MP3;
		emit finished( this );
	}
	else {
		m_currentProcessedTrack->setBufferFile( m_mp3Job->decodedFile() );
		m_iNumFilesAlreadyDecoded++;
		
		// continue with the decoding
		decodeNextFile();
	}
}


void K3bAudioJob::slotEmitProgress( int trackMade, int trackSize )
{
	double _trackPercent;
	if( trackSize > 0 )
		_trackPercent = 100.0 * (double)trackMade/(double)trackSize;
	else
		_trackPercent = 0;
		
	double _percentPerTrack = 100.0 / (double)( m_doc->numOfTracks() + m_iNumFilesToDecode );
	// already done files
	double _overallPercent = _percentPerTrack * ( m_iNumFilesAlreadyDecoded + m_iNumTracksAlreadyWritten );
	// and the current file
	_overallPercent += _percentPerTrack * _trackPercent / 100.0;
	
	emit processedSubSize( trackMade, trackSize );
	emit subPercent( (int)_trackPercent  );
	emit percent( (int)_overallPercent );
}
