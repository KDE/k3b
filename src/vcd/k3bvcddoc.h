/***************************************************************************
                          k3bvcddoc.h  -  description
                             -------------------
    begin                : Mon Nov 4 2002
    copyright            : (C) 2002 by Sebastian Trueg
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

#ifndef K3BVCDDOC_H
#define K3BVCDDOC_H

#include "k3bvcdoptions.h"
#include "mpeginfo/mpeg.h"
#include "mpeginfo/chunkTab.h"
#include "../k3bdoc.h"

#include <qptrqueue.h>
#include <qfile.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qdatetime.h>
#include <qtextstream.h>

#include <kurl.h>

class K3bApp;
class K3bVcdTrack;
class K3bVcdJob;
class K3bView;
class QWidget;
class QTimer;
class QDomDocument;
class QDomElement;


class K3bVcdDoc : public K3bDoc
{
  Q_OBJECT

 public:
  K3bVcdDoc( QObject* );
  ~K3bVcdDoc();

  /** reimplemented from K3bDoc */
  K3bView* newView( QWidget* parent );
  /** reimplemented from K3bDoc */
  void addView(K3bView* view);

  enum vcdTypes { VCD11, VCD20, SVCD10, HQVCD, NONE};
  
  bool newDocument();
  int numOfTracks() const { return m_tracks->count(); }

  const QString& vcdImage() const { return m_vcdImage; }
  void setVcdImage( const QString& s ) { m_vcdImage = s; }

  K3bVcdTrack* first() { return m_tracks->first(); }
  K3bVcdTrack* current() const { return m_tracks->current(); }
  K3bVcdTrack* next() { return m_tracks->next(); }
  K3bVcdTrack* prev() { return m_tracks->prev(); }
  K3bVcdTrack* at( uint i ) { return m_tracks->at( i ); }
  K3bVcdTrack* take( uint i ) { return m_tracks->take( i ); }

  const QList<K3bVcdTrack>* tracks() const { return m_tracks; }

  /** get the current size of the project */
  unsigned long long size() const;
  unsigned long long length() const;
    
  K3bBurnJob* newBurnJob();
  K3bVcdOptions* vcdOptions() const { return m_vcdOptions; }

  bool deleteImage() const { return m_deleteImage; }

  int vcdType() const { return m_vcdType; }
  void setVcdType( int type );
  void setDeleteImage( bool b ) { m_deleteImage = b; }
      
  static unsigned int identifyMpegFile( const KURL& url );

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
  void addTrack( K3bVcdTrack* track, uint position = 0 );


  // --- TODO: this should read: removeTrack( K3bVcdTrack* )
  void removeTrack( K3bVcdTrack* );
  void moveTrack( const K3bVcdTrack* track, const K3bVcdTrack* after );

 protected slots:
  /** processes queue "urlsToAdd" **/
  void slotWorkUrlQueue();

 signals:
  void newTracks();
  //  void trackRemoved( uint );

 protected:
  /** reimplemented from K3bDoc */
  bool loadDocumentData( QDomElement* root );
  /** reimplemented from K3bDoc */
  bool saveDocumentData( QDomElement* );

  
  QString documentType() const;

  void loadDefaultSettings();

 private:
  K3bVcdTrack* createTrack( const KURL& url );
  void informAboutNotFoundFiles();


  QStringList m_notFoundFiles;
  QString m_vcdImage;
  
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

  QList<K3bVcdTrack>* m_tracks;
  unsigned long long calcTotalSize() const;
  K3bVcdTrack* m_lastAddedTrack;
  K3bVcdOptions* m_vcdOptions;

  bool m_deleteImage;
  int m_vcdType;
  uint lastAddedPosition;
 
};


#endif
