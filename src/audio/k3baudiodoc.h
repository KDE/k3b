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
class QTimer;
class QDomDocument;


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
  int numberOfTracks() const { return m_tracks->count(); }

  K3bAudioTrack* first() { return m_tracks->first(); }
  K3bAudioTrack* current() const { return m_tracks->current(); }
  K3bAudioTrack* next() { return m_tracks->next(); }
  K3bAudioTrack* prev() { return m_tracks->prev(); }
  K3bAudioTrack* at( uint i ) { return m_tracks->at( i ); }
  K3bAudioTrack* take( uint i ) { return m_tracks->take( i ); }

  /** get the current size of the project */
  long size() const;
  int length() const;
	
  // CD-Text
  bool cdText() const { return m_cdText; }
  const QString& title() const { return m_cdTextTitle; }
  const QString& artist() const { return m_cdTextArtist; }
  const QString& disc_id() const { return m_cdTextDisc_id; }
  const QString& arranger() const { return m_cdTextArranger; }
  const QString& songwriter() const { return m_cdTextSongwriter; }
  const QString& upc_ean() const { return m_cdTextUpc_Ean; }
  const QString& cdTextMessage() const { return m_cdTextMessage; }

  bool writeTOC( const QString& filename );
  int numOfTracks() const;

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


  // --- TODO: this should read: removeTrack( K3bAudioTrack* )
  void removeTrack( int position );
  void moveTrack( uint oldPos, uint newPos );

  void setPadding( bool p ) { m_padding = p; }
  //	void cancel();

  void setHideFirstTrack( bool b ) { m_hideFirstTrack = b; }

  // CD-Text
  void writeCdText( bool b ) { m_cdText = b; }
  void setTitle( const QString& v ) { m_cdTextTitle = v; }
  void setArtist( const QString& v ) { m_cdTextArtist = v; }
  void setDisc_id( const QString& v ) { m_cdTextDisc_id = v; }
  void setArranger( const QString& v ) { m_cdTextArranger = v; }
  void setSongwriter( const QString& v ) { m_cdTextSongwriter = v; }
  void setUpc_ean( const QString& v ) { m_cdTextUpc_Ean = v; }
  void setCdTextMessage( const QString& v ) { m_cdTextMessage = v; }

 protected slots:
  /** processes queue "urlsToAdd" **/
  void slotWorkUrlQueue();
	
 signals:
  void newTracks();
  void trackRemoved( uint );

 protected:
  /** reimplemented from K3bDoc */
  bool loadDocumentData( QDomDocument* );
  /** reimplemented from K3bDoc */
  bool saveDocumentData( QDomDocument* );

  QString documentType() const;

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
  QTimer* m_urlAddingTimer;
	
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
  bool m_hideFirstTrack;
 	
  // CD-Text
  // --------------------------------------------------
  bool m_cdText;
  QString m_cdTextTitle;
  QString m_cdTextArtist;
  QString m_cdTextDisc_id;
  QString m_cdTextArranger;
  QString m_cdTextUpc_Ean;
  QString m_cdTextSongwriter;
  QString m_cdTextMessage;
  // --------------------------------------------------
};


#endif
