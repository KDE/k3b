/***************************************************************************
                          k3baudiodoc.h  -  description
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

#ifndef K3BAUDIODOC_H
#define K3BAUDIODOC_H

#include "k3bdoc.h"

#include <qqueue.h>
#include <qfile.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qdatetime.h>

#include <kurl.h>

class K3bApp;
class K3bAudioTrack;
class K3bAudioBurnDialog;
class QWidget;

/**Document class for an audio project. It uses a @p K3bAudioProject
to store the data and burn.
  *@author Sebastian Trueg
  */

class K3bAudioDoc : public K3bDoc  {

	Q_OBJECT
	
public:
	K3bAudioDoc( K3bApp*, const QString& cdrecord = "/usr/bin/cdrecord", const QString& mpg123 = "/usr/bin/mpg123");
	~K3bAudioDoc();
	
	/** reimplemented from K3bDoc */
	K3bView* newView( QWidget* parent );
	/** reimplemented from K3bDoc */
	void addView(K3bView* view);

	bool newDocument();

	QTime audioSize() const;
	bool padding() const { return m_padding; }
	int numberOfTracks() const { return m_tracks->count(); }

	K3bAudioTrack* currentProcessedTrack() const { return m_currentProcessedTrack; }
	
	K3bAudioTrack* current() const { return m_tracks->current(); }
	K3bAudioTrack* next() { return m_tracks->next(); }
	K3bAudioTrack* prev() { return m_tracks->prev(); }
	K3bAudioTrack* at( uint i ) { return m_tracks->at( i ); }
	K3bAudioTrack* take( uint i ) { return m_tracks->take( i ); }

	/** get the current size of the project */
	int size();
	
public slots:
	/**
	 * will test the file and add it to the project.
	 * connect to at least result() to know when
	 * the process is finished and check error()
	 * to know about the result.
	 **/
	void addTrack( const QString&, uint );
	void addTracks( const QStringList&, uint );
	/** adds a track without any testing */
	void addTrack( K3bAudioTrack* track, uint position = 0 );
//	void addTracks( QList<K3bAudioTrack>& tracks );
	void removeTrack( int position );
	void moveTrack( uint oldPos, uint newPos );
	void showBurnDialog();

	void setPadding( bool p ) { m_padding = p; }
	
protected slots:
 	/** processes queue "urlsToAdd" **/
 	void addNextTrack();
 	void addMp3File( const QString& fileName, uint position );
	void addWavFile( const QString& fileName, uint position );
	
	void slotTestOutput( const QString& text );
	/** Convert mp3 into wav since cdrecord is used **/
	void prepareTracks();
	void write();
	void writeImage( const QString& filename );

 	void startRecording();
	void parseCdrecordOutput( KProcess*, char* output, int len );
	void cdrecordFinished();
	void bufferFiles( );
	void parseMpgDecodingOutput( KProcess*, char* output, int len );
	void parseMpgTestingOutput( KProcess*, char* output, int len );
	void fileBufferingFinished();
	void mp3FileTestingFinished();

signals:
	void newTrack( K3bAudioTrack* );
	void trackRemoved( uint );
	void nextTrackProcessed();
	void trackPercent( int percent );
	void trackProcessedSize( int processed, int size );
	void trackProcessedMinutes( const QTime& );
	void processedMinutes( const QTime& );
	void startDecoding();

protected:
 	/** reimplemented from K3bDoc */
 	bool loadDocumentData( QFile& f );
 	/** reimplemented from K3bDoc */
 	bool saveDocumentData( QFile& f );

private:
	class PrivateUrlToAdd
	{
	public:
		PrivateUrlToAdd( const QString& _url, int _pos )
			: url( _url ), position(_pos) {}
		QString url;
		int position;
	};
	/** Holds all the urls that have to be added to the list of tracks. **/
	QQueue<PrivateUrlToAdd> urlsToAdd;
	
	/** The last added file. This is saved even if the file not exists or
	the url is malformed. */
	KURL addedFile;
	bool startBurningAfterBuffering;
	
	/** used to determine the overall progress **/
	int m_iNumFilesToBuffer;
	int m_iNumFilesAlreadyBuffered;
	
	QList<K3bAudioTrack>* m_tracks;
	K3bAudioTrack* m_currentProcessedTrack;
	K3bAudioTrack* m_lastAddedTrack;
	
	/** path to mpg123 executable **/
	QString m_mpg123;

 	QString lastTempFile;
 	uint lastAddedPosition;
 	
	K3bAudioBurnDialog* m_burnDialog;

 	// settings
 	/** if true the adding of files will take longer */
 	bool testFiles;
 	bool m_padding;
};


#endif
