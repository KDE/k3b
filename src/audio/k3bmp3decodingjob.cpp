/***************************************************************************
                          k3bmp3decodingjob.cpp  -  description
                             -------------------
    begin                : Fri May 4 2001
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

#include "k3bmp3decodingjob.h"

#include "../k3bglobals.h"
#include "../k3b.h"

#include <kapp.h>
#include <kconfig.h>
#include <kprocess.h>
#include <klocale.h>

#include <qstring.h>
#include <qfile.h>
#include <qfileinfo.h>


K3bMp3DecodingJob::K3bMp3DecodingJob( const QString& fileName )
	: K3bJob(), m_sourceFile( fileName ), m_decodedFile( QString::null )
{
	// connect to the mpg123-slots
	connect( &m_process, SIGNAL(processExited(KProcess*)),
			 this, SLOT(slotMpg123Finished()) );
	connect( &m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
			 this, SLOT(slotParseMpg123Output(KProcess*, char*, int)) );
	connect( &m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
			 this, SLOT(slotParseMpg123Output(KProcess*, char*, int)) );
}


bool K3bMp3DecodingJob::setSourceFile( const QString& fileName )
{
	if( QFile::exists( fileName ) ) {
		// test on read permission
		if( !QFileInfo( fileName ).isReadable() ) {
			qDebug( i18n( "(K3bMp3DecodingJob) No read access to file %1").arg(fileName) );
			return false;
		}
		
		// set new source file
		m_sourceFile = fileName;
		// reset target file
		m_decodedFile = QString::null;
		return true;
	}
	else {
		qDebug( i18n("(K3bMp3DecodingJob) File %1 does not exist!").arg(fileName) );
		return false;
	}
}


void K3bMp3DecodingJob::start()
{
	if( !m_process.isRunning() ) {
		if( QFile::exists( m_decodedFile ) ) {
			qDebug( i18n("(K3bMp3DecodingJob) File already exists: %1").arg(m_decodedFile) );
			return;
		}
		
		// if no target file is specified use temporary file from K3bMain
		if( m_decodedFile.isEmpty() ) {
			m_decodedFile = k3bMain()->findTempFile("wav");
		}
		
		m_fileDecodingSuccessful = false;
		
		// start a new mpg123 process
		kapp->config()->setGroup( "External Programs");
	
		// start a new mpg123 process for this track
		m_process.clearArguments();
		m_process << kapp->config()->readEntry( "mpg123 path" );
	
		m_process << "-v";
		m_process << "-w";
		m_process << m_decodedFile;
		m_process << m_sourceFile;

		if( !m_process.start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
		{
			// something went wrong when starting the program
			// it "should" be the executable
			emit infoMessage( "could not start mpg123");
			m_error = K3b::MPG123_ERROR;
			emit finished( this );
		}
		else
		{
			emit infoMessage( i18n("Decoding file %1...").arg(m_sourceFile) );
			qDebug( i18n("Decoding file %1 to %2").arg(m_sourceFile).arg(m_decodedFile) );
			m_error = K3b::WORKING;
			m_fileDecodingSuccessful = false;
		}
	}
}

void K3bMp3DecodingJob::cancel()
{
	if( m_error == K3b::WORKING ) {
		m_process.kill();
		m_error = K3b::CANCELED;
		emit infoMessage("Decoding canceled.");
		emit finished( this );

		// remove not finished temp-files
		if( !m_fileDecodingSuccessful ) {
			qDebug( "Removing temporary file " + m_decodedFile );
			QFile::remove( m_decodedFile );
		}
	}
}


void K3bMp3DecodingJob::slotParseMpg123Output( KProcess*, char* output, int len )
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
			
			emit processedSize( made, made+togo );
			emit percent( 100 * made / (made+togo) );
		}
//		else {
//			// debugging
//			emitMessage( *str );
//		}
	}
}

void K3bMp3DecodingJob::slotMpg123Finished()
{
	// check if process exited successfully and if so:
	if( !m_process.normalExit() ) {
		// do some shit
		qDebug( "mpg123 did exit with errors." );
		m_error = K3b::MPG123_ERROR;
		emit infoMessage( "mpg123 did exit with errors." );
	}
	
	else if( !(m_process.exitStatus() == 0 ) ) {
		qDebug( "Mp3-File " + m_sourceFile + " is corrupt." );
		m_error = K3b::CORRUPT_MP3;
		emit infoMessage( i18n("Mp3-File %1 is corrupt.").arg(m_sourceFile) );
	}
	else {	
		// here the file should be valid!
		m_fileDecodingSuccessful = true;
		m_error = K3b::SUCCESS;
	}
	
	emit finished( this );
}

