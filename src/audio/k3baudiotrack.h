/***************************************************************************
                          k3baudiotrack.h  -  description
                             -------------------
    begin                : Wed Mar 28 2001
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

#ifndef K3BAUDIOTRACK_H
#define K3BAUDIOTRACK_H

#include <qstring.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qdatetime.h>
#include <qlist.h>

/**
  *@author Sebastian Trueg
  */

class K3bAudioTrack
{
public:
	K3bAudioTrack( QList<K3bAudioTrack>* parent, const QString& filename );
	K3bAudioTrack( const K3bAudioTrack& a );
	~K3bAudioTrack();

	QString fileName() const { return QFileInfo(m_file).fileName(); }
	QString absPath() const { return QFileInfo(m_file).absFilePath(); }
	const QString& bufferFile() const { return m_bufferFile; }
	int  filetype() const { return m_filetype; }
	int  pregap() const { return m_pregap; }
	const QString& artist() const { return m_artist; }
	const QString& title() const { return m_title; }
	const QString& album() const { return m_album; }
	const QTime& length() const { return m_length; }

	/** Default vaule is 2 **/
	void setPregap( int p ) { m_pregap = p; }

 	/**
 	 * If the file is a mp3-file, it's mp3-tag is used
 	 **/
	void setArtist( const QString& a ) { m_artist = a; }

 	/**
 	 * If the file is a mp3-file, it's mp3-tag is used
 	 **/
	void setTitle( const QString& t ) { m_title = t; }

	void setAlbum( const QString& t ) { m_album = t; }
	
	void setLength( const QTime& time ) { m_length = time; }
	
	void setBufferFile( const QString& file ) { m_bufferFile = file; }
	/** returns the filesize of the track */
	uint size() const;
	/** returns the index in the list */
	int index() const;

protected:
	QList<K3bAudioTrack>* m_parent;

private:	
	QFile m_file;
	QString m_bufferFile;
	QTime m_length;
	int  m_filetype;
	int  m_pregap;
	
	/** CD-Text: copy protection */
	bool m_copy;
	/** CD-Text: PERFORMER */
	QString m_artist;
	/** CD-Text: TITLE (track) */
	QString m_title;
	/** CD-Text: TITLE (cd) */
	QString m_album;
};


#endif
