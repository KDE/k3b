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

#include "../k3bdoc.h"

#include <qqueue.h>
#include <qfile.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qdatetime.h>

#include <kurl.h>

class K3bApp;
class K3bAudioTrack;
class QWidget;

/**Document class for an audio project. It uses a @p K3bAudioProject
to store the data and burn.
  *@author Sebastian Trueg
  */

class K3bAudioDoc : public K3bDoc  {

	Q_OBJECT
	
public:
	K3bAudioDoc( QObject* );
	~K3bAudioDoc();
	
	/** reimplemented from K3bDoc */
	K3bView* newView( QWidget* parent );
	/** reimplemented from K3bDoc */
	void addView(K3bView* view);

	bool newDocument();

	QTime audioSize() const;
	bool padding() const;
	int numberOfTracks() const { return m_tracks->count(); }

	K3bAudioTrack* current() const { return m_tracks->current(); }
	K3bAudioTrack* next() { return m_tracks->next(); }
	K3bAudioTrack* prev() { return m_tracks->prev(); }
	K3bAudioTrack* at( uint i ) { return m_tracks->at( i ); }
	K3bAudioTrack* take( uint i ) { return m_tracks->take( i ); }

	/** get the current size of the project */
	int size();
	
	// CD-Text
	bool cdText() const { return m_cdText; }
	const QString& title() const { return m_cdTextTitle; }
	const QString& artist() const { return m_cdTextArtist; }
	const QString& isrc() const { return m_cdTextISRC; }
	const QString& arranger() const { return m_cdTextArranger; }

	QString writeTOC( const QString& filename );
	int numOfTracks() const;
	int allMp3Decoded() const;
	K3bAudioTrack* nextTrackToDecode() const;

	K3bBurnJob* newBurnJob();
		
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

	void setPadding( bool p ) { m_padding = p; }
//	void cancel();

	// CD-Text
	void writeCdText( bool b ) { m_cdText = b; }
	void setTitle( const QString& v ) { m_cdTextTitle = v; }
	void setArtist( const QString& v ) { m_cdTextArtist = v; }
	void setISRC( const QString& v ) { m_cdTextISRC = v; }
	void setArranger( const QString& v ) { m_cdTextArranger = v; }

protected slots:
 	/** processes queue "urlsToAdd" **/
 	void addNextTrack();
 	void addMp3File( const QString& fileName, uint position );
	void addWavFile( const QString& fileName, uint position );
	
	void parseMpgTestingOutput( KProcess*, char* output, int len );
	void mp3FileTestingFinished();

signals:
	void newTrack( K3bAudioTrack* );
	void trackRemoved( uint );

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

	QList<K3bAudioTrack>* m_tracks;
	K3bAudioTrack* m_lastAddedTrack;
	
 	uint lastAddedPosition;
 	
 	// settings
 	/** if true the adding of files will take longer */
 	bool testFiles;
 	bool m_padding;
 	bool m_fileDecodingSuccessful;
 	
 	// CD-Text
 	// --------------------------------------------------
 	bool m_cdText;
 	QString m_cdTextTitle;
 	QString m_cdTextArtist;
 	QString m_cdTextISRC;
 	QString m_cdTextArranger;
 	// --------------------------------------------------
};


#endif
