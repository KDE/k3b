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

#include <qfile.h>
#include <qstring.h>
#include <qdatetime.h>

#include <kurl.h>

class K3bAudioTrack;
class QWidget;

/**Document class for an audio project. It uses a @p K3bAudioProject
to store the data and burn.
  *@author Sebastian Trueg
  */

class K3bAudioDoc : public K3bDoc  {

	Q_OBJECT
	
public:
	K3bAudioDoc( const QString& cdrecord = "/usr/bin/cdrecord", const QString& mpg123 = "/usr/bin/mpg123");
	~K3bAudioDoc();
	
  /** reimplemented from K3bDoc */
  K3bView* newView( QWidget* parent );
  /** reimplemented from K3bDoc */
  void addView(K3bView* view);

public slots:
	/**
	 * will test the file and add it to the project.
	 * connect to at least result() to know when
	 * the process is finished and check error()
	 * to know about the result.
	 **/
	void slotAddTrack( const QString&, uint );
	/** adds a track without any testing */
	void addTrack( K3bAudioTrack* track, uint position = 0 );
//	void addTracks( QList<K3bAudioTrack>& tracks );
	void removeTrack( uint position );
	void moveTrack( uint oldPos, uint newPos );

protected slots:
	void slotTestOutput( const QString& text );
	/** Convert mp3 into wav since cdrecord is used **/
	void prepareTracks();
	void write();
	void writeImage( const QString& filename );

//	const QTime& audioSize() const;
	int numberOfTracks() const { return m_tracks.count(); }

	K3bAudioTrack* current() const { return m_tracks.current(); }
	K3bAudioTrack* next() { return m_tracks.next(); }
	K3bAudioTrack* prev() { return m_tracks.prev(); }
	K3bAudioTrack* at( uint i ) { return m_tracks.at( i ); }
	K3bAudioTrack* take( uint i ) { return m_tracks.take( i ); }

	/** get the current size of the project */
	int size();

  	void startRecording();
	void parseCdrecordOutput( KProcess*, char* output, int len );
	void cdrecordFinished();
	void bufferFiles( );
	void parseMpg123Output( KProcess*, char* output, int len );
	void fileBufferingFinished();
	void mp3FileTestingFinished();

signals:
	void newTrack( K3bAudioTrack* );
	void trackRemoved( uint );
	void nextTrackProcessed();
	void trackPercent( unsigned long precent );
	void trackProcessedSize( unsigned long size );
	void trackProcessedMinutes( const QTime& );
	void processedMinutes( const QTime& );

protected:
 	/** reimplemented from K3bDoc */
 	bool loadDocumentData( QFile& f );
 	/** reimplemented from K3bDoc */
 	bool saveDocumentData( QFile& f );
	void addMp3File( const QString& fileName, uint position );
	void addWavFile( const QString& fileName, uint position );

private: 	
	/** The last added file. This is saved even if the file not exists or
	the url is malformed. */
	KURL addedFile;
	bool startBurningAfterBuffering;

	QList<K3bAudioTrack> m_tracks;
	K3bAudioTrack* m_currentBufferedTrack;
	K3bAudioTrack* m_lastAddedTrack;

	/** path to mpg123 executable **/
	QString m_mpg123;

 	QString lastTempFile;
 	int lastAddedPosition;

 	// settings
 	/** if true the adding of files will take longer */
 	bool testFiles;
};

#endif
