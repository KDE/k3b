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

#include <k3bcdtext.h>
#include <k3btoc.h>

#include <qptrlist.h>
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
class K3bAudioDataSource;
class K3bAudioDecoder;

/**Document class for an audio project. 
 *@author Sebastian Trueg
 */

class K3bAudioDoc : public K3bDoc  
{
  Q_OBJECT

  friend class K3bMixedDoc;
  friend class K3bAudioTrack;
  friend class K3bAudioFile;
	
 public:
  K3bAudioDoc( QObject* );
  ~K3bAudioDoc();
	
  bool newDocument();

  bool hideFirstTrack() const { return m_hideFirstTrack; }
  int numOfTracks() const;

  bool normalize() const { return m_normalize; }

  K3bAudioTrack* firstTrack() const;
  K3bAudioTrack* lastTrack() const;

  /**
   * Slow.
   * \return the K3bAudioTrack with track number trackNum starting at 1 or 0 if trackNum > numOfTracks()
   */
  K3bAudioTrack* getTrack( unsigned int trackNum );

  /**
   * Creates a new audiofile inside this doc which has no track yet.
   */
  K3bAudioFile* createAudioFile( const KURL& url );

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

  /**
   * Create complete CD-Text including the tracks' data.
   */
  K3bDevice::CdText cdTextData() const;

  int audioRippingParanoiaMode() const { return m_audioRippingParanoiaMode; }
  int audioRippingRetries() const { return m_audioRippingRetries; }
  bool audioRippingIgnoreReadErrors() const { return m_audioRippingIgnoreReadErrors; }

  /**
   * Represent the structure of the doc as CD Table of Contents.
   */
  K3bDevice::Toc toToc() const;

  K3bBurnJob* newBurnJob( K3bJobHandler*, QObject* parent = 0 );

  /**
   * Shows dialogs.
   */
  void informAboutNotFoundFiles();

 public slots:
  void addUrls( const KURL::List& );
  void addTrack( const KURL&, uint );
  void addTracks( const KURL::List&, uint );
  /** 
   * Adds a track without any testing 
   *
   * Slow because it uses getTrack.
   */
  void addTrack( K3bAudioTrack* track, uint position = 0 );

  void addSources( K3bAudioTrack* parent, const KURL::List& urls, K3bAudioDataSource* sourceAfter = 0 );

  void removeTrack( K3bAudioTrack* );
  void moveTrack( K3bAudioTrack* track, K3bAudioTrack* after );

  void setHideFirstTrack( bool b ) { m_hideFirstTrack = b; }
  void setNormalize( bool b ) { m_normalize = b; }

  // CD-Text
  void writeCdText( bool b ) { m_cdText = b; }
  void setTitle( const QString& v ) { m_cdTextData.setTitle( v ); }
  void setArtist( const QString& v ) { setPerformer( v ); }
  void setPerformer( const QString& v ) { m_cdTextData.setPerformer( v ); }
  void setDisc_id( const QString& v ) { m_cdTextData.setDiscId( v ); }
  void setArranger( const QString& v ) { m_cdTextData.setArranger( v ); }
  void setSongwriter( const QString& v ) { m_cdTextData.setSongwriter( v ); }
  void setComposer( const QString& v ) { m_cdTextData.setComposer( v ); }
  void setUpc_ean( const QString& v ) { m_cdTextData.setUpcEan( v ); }
  void setCdTextMessage( const QString& v ) { m_cdTextData.setMessage( v ); }

  // Audio-CD Ripping
  void setAudioRippingParanoiaMode( int i ) { m_audioRippingParanoiaMode = i; }
  void setAudioRippingRetries( int r ) { m_audioRippingRetries = r; }
  void setAudioRippingIgnoreReadErrors( bool b ) { m_audioRippingIgnoreReadErrors = b; }

  void removeCorruptTracks();

 private slots:
  void slotTrackChanged( K3bAudioTrack* );
  void slotTrackRemoved( K3bAudioTrack* );
  void slotHouseKeeping();

 signals:
  void trackChanged( K3bAudioTrack* );
  void trackRemoved( K3bAudioTrack* );

 protected:
  /** reimplemented from K3bDoc */
  bool loadDocumentData( QDomElement* );
  /** reimplemented from K3bDoc */
  bool saveDocumentData( QDomElement* );

  QString documentType() const;

  void loadDefaultSettings( KConfig* );

  /** reimplemented from K3bDoc */
  //  K3bView* newView( QWidget* parent );

 private:
  // the stuff for adding files
  // ---------------------------------------------------------
  bool readM3uFile( const KURL& url, KURL::List& playlist );
  K3bAudioTrack* createTrack( const KURL& url );
  K3bAudioDecoder* getDecoderForUrl( const KURL& url );
  /**
   * returns the new after track, ie. the the last added track or null if
   * the import failed.
   */
  K3bAudioTrack* importCueFile( const QString& cuefile, K3bAudioTrack* after );
  /**
   * Handle directories and M3u files
   */
  KURL::List extractUrlList( const KURL::List& urls );
  // ---------------------------------------------------------

  /**
   * Used by K3bAudioTrack to update the track list
   */
  void setFirstTrack( K3bAudioTrack* track );
  /**
   * Used by K3bAudioTrack to update the track list
   */
  void setLastTrack( K3bAudioTrack* track );

  /**
   * Used by K3bAudioFile to tell the doc that it does not need the decoder anymore.
   */
  void decreaseDecoderUsage( K3bAudioDecoder* );
  void increaseDecoderUsage( K3bAudioDecoder* );

  K3bAudioTrack* m_firstTrack;
  K3bAudioTrack* m_lastTrack;
 	
  bool m_hideFirstTrack;
  bool m_normalize;

  KURL::List m_notFoundFiles;
  KURL::List m_unknownFileFormatFiles;
 	
  // CD-Text
  // --------------------------------------------------
  K3bDevice::CdText m_cdTextData;
  bool m_cdText;
  // --------------------------------------------------

  // Audio ripping
  int m_audioRippingParanoiaMode;
  int m_audioRippingRetries;
  bool m_audioRippingIgnoreReadErrors;

  //
  // decoder housekeeping
  // --------------------------------------------------
  // used to check if we may delete a decoder
  QMap<K3bAudioDecoder*, int> m_decoderUsageCounterMap;
  // used to check if we already have a decoder for a specific file
  QMap<QString, K3bAudioDecoder*> m_decoderPresenceMap;
  // used to properly delete the finished analyser threads
  class AudioFileAnalyzerThread;
  QPtrList<AudioFileAnalyzerThread> m_audioTrackStatusThreads;
  QMap<K3bAudioDecoder*, QPtrList<K3bAudioTrack> > m_decoderMetaInfoSetMap;
};


#endif
