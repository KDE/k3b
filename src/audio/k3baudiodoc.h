/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BAUDIODOC_H
#define K3BAUDIODOC_H

#include <k3bdoc.h>

#include <qptrqueue.h>
#include <qfile.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qdatetime.h>
#include <qtextstream.h>

#include <kurl.h>

class K3bApp;
class K3bAudioTrack;
class QWidget;
class QTimer;
class QDomDocument;
class QDomElement;
class K3bThreadJob;


/**Document class for an audio project. 
 *@author Sebastian Trueg
 */

class K3bAudioDoc : public K3bDoc  
{
  Q_OBJECT
	
 public:
  K3bAudioDoc( QObject* );
  ~K3bAudioDoc();
	
  /** reimplemented from K3bDoc */
  K3bView* newView( QWidget* parent );
  /** reimplemented from K3bDoc */
  void addView(K3bView* view);

  bool newDocument();

  bool padding() const;
  bool hideFirstTrack() const { return m_hideFirstTrack; }
  bool removeBufferFiles() const { return m_removeBufferFiles; }
  bool onlyCreateImages() const { return m_onlyCreateImages; }
  int numberOfTracks() const { return m_tracks->count(); }

  K3bAudioTrack* first() { return m_tracks->first(); }
  K3bAudioTrack* current() const { return m_tracks->current(); }
  K3bAudioTrack* next() { return m_tracks->next(); }
  K3bAudioTrack* prev() { return m_tracks->prev(); }
  K3bAudioTrack* at( uint i ) { return m_tracks->at( i ); }
  K3bAudioTrack* take( uint i ) { return m_tracks->take( i ); }

  const QList<K3bAudioTrack>* tracks() const { return m_tracks; }

  /** get the current size of the project */
  KIO::filesize_t size() const;
  K3b::Msf length() const;
	
  // CD-Text
  bool cdText() const { return m_cdText; }
  const QString& title() const { return m_cdTextTitle; }
  const QString& artist() const { return m_cdTextArtist; }
  const QString& disc_id() const { return m_cdTextDisc_id; }
  const QString& arranger() const { return m_cdTextArranger; }
  const QString& songwriter() const { return m_cdTextSongwriter; }
  const QString& composer() const { return m_cdTextComposer; }
  const QString& upc_ean() const { return m_cdTextUpc_Ean; }
  const QString& cdTextMessage() const { return m_cdTextMessage; }

  int numOfTracks() const;

  K3bBurnJob* newBurnJob();

 public slots:
  /**
   * will test the file and add it to the project.
   * connect to at least result() to know when
   * the process is finished and check error()
   * to know about the result.
   **/
  void addUrl( const KURL& url );
  void addUrls( const KURL::List& );
  void addTrack( const KURL&, uint );
  void addTracks( const KURL::List&, uint );
  /** adds a track without any testing */
  void addTrack( K3bAudioTrack* track, uint position = 0 );


  void removeTrack( K3bAudioTrack* );
  void moveTrack( const K3bAudioTrack* track, const K3bAudioTrack* after );

  void setPadding( bool p ) { m_padding = p; }
  //	void cancel();

  void setHideFirstTrack( bool b ) { m_hideFirstTrack = b; }

  void setRemoveBufferFiles( bool b ) { m_removeBufferFiles = b; }

  void setOnlyCreateImages( bool b ) { m_onlyCreateImages = b; }

  // CD-Text
  void writeCdText( bool b ) { m_cdText = b; }
  void setTitle( const QString& v ) { m_cdTextTitle = v; }
  void setArtist( const QString& v ) { m_cdTextArtist = v; }
  void setDisc_id( const QString& v ) { m_cdTextDisc_id = v; }
  void setArranger( const QString& v ) { m_cdTextArranger = v; }
  void setSongwriter( const QString& v ) { m_cdTextSongwriter = v; }
  void setComposer( const QString& v ) { m_cdTextComposer = v; }
  void setUpc_ean( const QString& v ) { m_cdTextUpc_Ean = v; }
  void setCdTextMessage( const QString& v ) { m_cdTextMessage = v; }

  void removeCorruptTracks();

 protected slots:
  /** processes queue "urlsToAdd" **/
  void slotWorkUrlQueue();
  void slotDetermineTrackMetaInfo();
	
 signals:
  void newTracks();
  //  void trackRemoved( uint );

 protected:
  /** reimplemented from K3bDoc */
  bool loadDocumentData( QDomElement* );
  /** reimplemented from K3bDoc */
  bool saveDocumentData( QDomElement* );

  QString documentType() const;

  unsigned long isWaveFile( const KURL& url );

  void loadDefaultSettings();

 private:
  K3bAudioTrack* createTrack( const KURL& url );
  void informAboutNotFoundFiles();
  bool readM3uFile( const KURL&, int );


  QStringList m_notFoundFiles;

  class PrivateUrlToAdd
    {
    public:
      PrivateUrlToAdd( const KURL& u, int _pos )
	: url( u ), position(_pos) {}
      KURL url;
      int position;
    };
  /** Holds all the urls that have to be added to the list of tracks. **/
  QQueue<PrivateUrlToAdd> urlsToAdd;
  QTimer* m_urlAddingTimer;
	
  QList<K3bAudioTrack>* m_tracks;
  K3bAudioTrack* m_lastAddedTrack;
	
  uint lastAddedPosition;
 	
  // settings
  /** if true the adding of files will take longer */
  bool testFiles;
  bool m_padding;
  bool m_hideFirstTrack;
  bool m_removeBufferFiles;
  bool m_onlyCreateImages;
 	
  // CD-Text
  // --------------------------------------------------
  bool m_cdText;
  QString m_cdTextTitle;
  QString m_cdTextArtist;
  QString m_cdTextDisc_id;
  QString m_cdTextArranger;
  QString m_cdTextUpc_Ean;
  QString m_cdTextSongwriter;
  QString m_cdTextComposer;
  QString m_cdTextMessage;
  // --------------------------------------------------

  friend class K3bMixedDoc;

  class AudioTrackMetaInfoThread;
  AudioTrackMetaInfoThread* m_trackMetaInfoThread;
  K3bThreadJob* m_trackMetaInfoJob;
};


#endif
