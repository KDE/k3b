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


#ifndef K3BAUDIOTRACK_H
#define K3BAUDIOTRACK_H

#include <qobject.h>
#include <qstring.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qptrlist.h>

#include <kio/global.h>

#include <k3bmsf.h>

#include <k3bcdtext.h>
#include <k3btrack.h>


class K3bAudioDecoder;
class K3bAudioDataSource;
class K3bAudioDoc;


/**
 * @author Sebastian Trueg
 */
class K3bAudioTrack : public QObject
{
  Q_OBJECT

  friend class K3bAudioDataSource;

 public:
  K3bAudioTrack( K3bAudioDoc* parent );
  ~K3bAudioTrack();

  K3bAudioDoc* doc() const { return m_parent; }

  K3bCdDevice::Track toCdTrack() const;

  /** 
   * @return length of track in frames
   */
  K3b::Msf length() const;
  KIO::filesize_t size() const;

  const QString& artist() const { return m_cdText.performer(); }
  const QString& performer() const { return m_cdText.performer(); }
  const QString& title() const { return m_cdText.title(); }
  const QString& arranger() const { return m_cdText.arranger(); }
  const QString& songwriter() const { return m_cdText.songwriter(); }
  const QString& composer() const { return m_cdText.composer(); }
  const QString& isrc() const { return m_cdText.isrc(); }
  const QString& cdTextMessage() const { return m_cdText.message(); }
  const K3bCdDevice::TrackCdText& cdText() const { return m_cdText; }
	
  bool copyProtection() const { return m_copy; }
  bool preEmp() const { return m_preEmp; }
	
  /**
   * @obsolete use setPerformer
   **/
  void setArtist( const QString& a ) { m_cdText.setPerformer(a); emit changed(this); }
  void setPerformer( const QString& a ) { m_cdText.setPerformer(a); emit changed(this); }
  void setTitle( const QString& t ) { m_cdText.setTitle(t); emit changed(this); }
  void setArranger( const QString& t ) { m_cdText.setArranger(t); emit changed(this); }
  void setSongwriter( const QString& t ) { m_cdText.setSongwriter(t); emit changed(this); }
  void setComposer( const QString& t ) { m_cdText.setComposer(t); emit changed(this); }
  void setIsrc( const QString& t ) { m_cdText.setIsrc(t); emit changed(this); }
  void setCdTextMessage( const QString& t ) { m_cdText.setMessage(t); emit changed(this); }

  void setCdText( const K3bCdDevice::TrackCdText& cdtext ) { m_cdText = cdtext; emit changed(this); }

  void setPreEmp( bool b ) { m_preEmp = b; emit changed(this); }
  void setCopyProtection( bool b ) { m_copy = b; emit changed(this); }

  K3b::Msf index0() const;
  /**
   * The length of the postgap, ie. the number of blocks with index0.
   * This is always 0 for the last track.
   */
  K3b::Msf postGap() const;
  void setIndex0( const K3b::Msf& );

  /**
   * @return The index in the list 
   */
  unsigned int index() const;

  /**
   * Remove this track from the list and return it.
   */
  K3bAudioTrack* take();

  /**
   * Move this track after @p track.
   * If @p track is null this track will be merged into the beginning
   * of the docs list.
   */
  void moveAfter( K3bAudioTrack* track );

  /**
   * Move this track ahead of @p track.
   * If @p track is null this track will be appended to the end
   * of the docs list.
   */
  void moveAhead( K3bAudioTrack* track );

  /**
   * Merge @p trackToMerge into this one.
   */
  void merge( K3bAudioTrack* trackToMerge, K3bAudioDataSource* sourceAfter = 0 );

  K3bAudioTrack* prev() const { return m_prev; }
  K3bAudioTrack* next() const { return m_next; }

  /**
   * Use with care.
   */
  void setFirstSource( K3bAudioDataSource* source );
  K3bAudioDataSource* firstSource() const { return m_firstSource; }
  K3bAudioDataSource* lastSource() const;
  int numberSources() const;

  /**
   * Append source to the end of the sources list.
   */
  void addSource( K3bAudioDataSource* source );

  bool seek( const K3b::Msf& );

  /**
   * Read data from the track.
   *
   * @return number of read bytes
   */
  int read( char* data, unsigned int max );

  /**
   * called by K3bAudioDataSource because of the lack of signals
   */
  void sourceChanged( K3bAudioDataSource* );

  /**
   * Create a copy of this track containing copies of all the sources
   * but not beeing part of some list.
   */
  K3bAudioTrack* copy() const;

  /**
   * Split the track at position pos and return the splitted track
   * on success.
   * The new track will be moved after this track.
   */
  K3bAudioTrack* split( const K3b::Msf& pos );

  /**
   * Is this track in a list
   */
  bool inList() const;

 signals:
  /**
   * Emitted if the track has been changed.
   */
  void changed( K3bAudioTrack* );

 private:	
  /**
   * Removes the track from the list
   */
  void remove();

  K3bAudioDoc* m_parent;

  /** copy protection */
  bool m_copy;
  bool m_preEmp;

  K3b::Msf m_index0Offset;

  K3bCdDevice::TrackCdText m_cdText;

  // list
  K3bAudioTrack* m_prev;
  K3bAudioTrack* m_next;

  K3bAudioDataSource* m_firstSource;


  K3bAudioDataSource* m_currentSource;
  long long m_alreadyReadBytes;

  bool m_currentlyDeleting;
};


#endif
