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

#include "k3baudiodoc.h"
#include "k3baudioview.h"
#include "k3baudioproject.h"
#include "audiotracktestdialog.h"

#include <qstring.h>
#include <qdragobject.h>
#include <qfile.h>

#include <kurl.h>
#include <kapp.h>
#include <kmessagebox.h>


K3bAudioDoc::K3bAudioDoc()
 : K3bDoc()
{
	// TODO: this is not good since there should be the possibility go get
	//       the cdrecord and mpg123 path from the application
	m_project = new K3bAudioProject( "sampleprojectname", "/usr/bin/cdrecord", "/usr/bin/mpg123", this );
}

K3bAudioDoc::~K3bAudioDoc(){
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

void K3bAudioDoc::slotAddTrack(QDropEvent* e)
{
	if( !e->isAccepted() )
		return;

	// TODO: parse multible urls
	
	QString text;
	if( QTextDrag::decode( e, text ) ) {
		QString url = text;
		url.truncate( text.find( '\r') );

		addedFile = KURL( url );
		
		if( addedFile.isValid() ) {
			qDebug( addedFile.path() );
			AudioTrackTestDialog* a = new AudioTrackTestDialog( addedFile.fileName(), kapp->mainWidget()  );
			connect( m_project, SIGNAL(percent(int)), a, SLOT(setPercent(int)) );
			connect( m_project, SIGNAL(result()), this, SLOT(slotAddingFinished()) );
			connect( a, SIGNAL(canceled()), m_project, SLOT(cancel()) );
			connect( m_project, SIGNAL(result()), a, SLOT(close()) );
//			connect( m_project, SIGNAL(infoMessage(const QString&)), this, SLOT(slotTestOutput(const QString&)) );
			a->show();
			m_project->addTrack( addedFile.path() );
		}
	}
}


void K3bAudioDoc::addView(K3bView* view)
{
	K3bAudioView* v = (K3bAudioView*)view;
	connect( v, SIGNAL(dropped(QDropEvent*)), this, SLOT(slotAddTrack(QDropEvent*)) );
	connect( m_project, SIGNAL(newTrack(K3bAudioTrack*)), v, SLOT(addItem(K3bAudioTrack*)) );
}


void K3bAudioDoc::slotAddingFinished(){
	if( m_project->error() == CANCELED ) {
		// do nothing
	}
	else if( m_project->error() == FILE_NOT_FOUND ) {
		// say there was an error
		KMessageBox::information( kapp->mainWidget(), "File not found: " + addedFile.fileName(), "Error", QString::null, false );
	}
	else if( m_project->error() == WRONG_FILE_FORMAT ) {
		// say there was an error
		KMessageBox::information( kapp->mainWidget(), "Only mp3 and wav audio files are supported!", "Wrong file format", QString::null, false );
	}
	else if( m_project->error() == CORRUPT_MP3 ) {
		// say there was an error
		KMessageBox::information( kapp->mainWidget(), "Could not decode mp3-file " + addedFile.fileName(), "Error", QString::null, false );
	}
	else if( m_project->error() != SUCCESS ) {
		// say there was an error
		KMessageBox::information( kapp->mainWidget(), "Errorcode " +  QString::number(m_project->error()) + " with file " + addedFile.fileName(), "Error", QString::null, false );
	}

	// very important!
	m_project->disconnect( this );
}

void K3bAudioDoc::slotTestOutput( const QString& text ){
	qDebug( text );
}
