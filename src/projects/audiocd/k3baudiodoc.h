/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bcdtext.h"

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
class KConfig;


/**Document class for an audio project. 
 *@author Sebastian Trueg
 */

class K3bAudioDoc : public K3bDoc  
{
  Q_OBJECT
	
 public:
  K3bAudioDoc( QObject* );
  ~K3bAudioDoc();
	
  bool newDocument();

  bool padding() const;
  bool hideFirstTrack() const { return m_hideFirstTrack; }
  int numberOfTracks() const { return m_tracks->count(); }

  bool normalize() const { return m_normalize; }

  K3bAudioTrack* first() { return m_tracks->first(); }
  K3bAudioTrack* current() const { return m_tracks->current(); }
  K3bAudioTrack* next() { return m_tracks->next(); }
  K3bAudioTrack* prev() { return m_tracks->prev(); }
  K3bAudioTrack* at( uint i ) { return m_tracks->at( i ); }
  K3bAudioTrack* take( uint i ) { return m_tracks->take( i ); }

  const QPtrList<K3bAudioTrack>* tracks() const { return m_tracks; }

  /** get the current size of the project */
  KIO::filesize_t size() const;
  K3b::Msf length() const;
	
  // CD-Text
  bool cdText() const { return m_cdText; }
  const QString& title() const { return m_cdTextData.title(); }
  const QString& artist() const { return m_cdTextData.performer(); }
  const QString& disc_id() const { return m_cdTextData.discId(); }
  const QString& arranger() const { return m_cdTextData.arranger(); }
  const QString& songwriter() const { return m_cdTextData.songwriter(); }
  const QString& composer() const { return m_cdTextData.composer(); }
  const QString& upc_ean() const { return m_cdTextData.upcEan(); }
  const QString& cdTextMessage() const { return m_cdTextData.message(); }

  const K3bCdDevice::AlbumCdText& cdTextData() const { return m_cdTextData; }

  int numOfTracks() const;

  K3bBurnJob* newBurnJob( K3bJobHandler*, QObject* parent = 0 );

 public slots:
  /**
   * will test the file and add it to the project.
   * connect to at least result() to know when
   * the process is finished and check error()
   * to know about the result.
   **/
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

  void setNormalize( bool b ) { m_normalize = b; }

  // CD-Text
  void writeCdText( bool b ) { m_cdText = b; }
  void setTitle( const QString& v ) { m_cdTextData.setTitle( v ); }
  void setArtist( const QString& v ) { m_cdTextData.setPerformer( v ); }
  void setDisc_id( const QString& v ) { m_cdTextData.setDiscId( v ); }
  void setArranger( const QString& v ) { m_cdTextData.setArranger( v ); }
  void setSongwriter( const QString& v ) { m_cdTextData.setSongwriter( v ); }
  void setComposer( const QString& v ) { m_cdTextData.setComposer( v ); }
  void setUpc_ean( const QString& v ) { m_cdTextData.setUpcEan( v ); }
  void setCdTextMessage( const QString& v ) { m_cdTextData.setMessage( v ); }

  void removeCorruptTracks();

 protected slots:
  /** processes queue "urlsToAdd" **/
  void slotWorkUrlQueue();
  void slotDetermineTrackStatus();

  void slotTrackChanged();
	
 signals:
  void newTracks();
  void trackRemoved( K3bAudioTrack* );

 protected:
  /** reimplemented from K3bDoc */
  bool loadDocumentData( QDomElement* );
  /** reimplemented from K3bDoc */
  bool saveDocumentData( QDomElement* );

  QString documentType() const;

  unsigned long isWaveFile( const KURL& url );

  void loadDefaultSettings( KConfig* );

  K3bProjectBurnDialog* newBurnDialog( QWidget* parent = 0, const char* name = 0 );

  /** reimplemented from K3bDoc */
  K3bView* newView( QWidget* parent );

 private:
  K3bAudioTrack* createTrack( const KURL& url );
  void informAboutNotFoundFiles();
  bool readM3uFile( const KURL&, int );


  QStringList m_notFoundFiles;
  QStringList m_unknownFileFormatFiles;

  class PrivateUrlToAdd
    {
    public:
      PrivateUrlToAdd( const KURL& u, int _pos )
	: url( u ), position(_pos) {}
      KURL url;
      int position;
    };
  /** Holds all the urls that have to be added to the list of tracks. **/
  QPtrQueue<PrivateUrlToAdd> urlsToAdd;
  QTimer* m_urlAddingTimer;
	
  QPtrList<K3bAudioTrack>* m_tracks;
  K3bAudioTrack* m_lastAddedTrack;
	
  uint lastAddedPosition;
 	
  // settings
  /** if true the adding of files will take longer */
  bool testFiles;
  bool m_padding;
  bool m_hideFirstTrack;

  bool m_normalize;
 	
  // CD-Text
  // --------------------------------------------------
  K3bCdDevice::AlbumCdText m_cdTextData;
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

  class AudioTrackStatusThread;
  AudioTrackStatusThread* m_trackStatusThread;
  K3bThreadJob* m_trackMetaInfoJob;
};


#endif
