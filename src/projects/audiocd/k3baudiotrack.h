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


#ifndef K3BAUDIOTRACK_H
#define K3BAUDIOTRACK_H

#include <qobject.h>
#include <qstring.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qptrlist.h>

#include <kio/global.h>

#include <k3bmsf.h>

#include "k3bcdtext.h"

class K3bAudioDecoder;


/**
  *@author Sebastian Trueg
  */

class K3bAudioTrack : public QObject
{
  Q_OBJECT

 public:
  K3bAudioTrack( QPtrList<K3bAudioTrack>* parent, const QString& filename );
  ~K3bAudioTrack();

  K3bAudioDecoder* module() const { return m_module; }

  // TODO: this should only be accessable by K3bAudioDoc
  void setModule( K3bAudioDecoder* module );

  QString filename() const { return m_filename.section( '/', -1 ); }
  const QString& path() const { return m_filename; }

  K3b::Msf pregap() const { return m_pregap; }

  /** 
   * @return length of track in frames
   */
  K3b::Msf length() const;

  /**
   * @return the complete length of the audio file
   */
  K3b::Msf fileLength() const;

  const QString& artist() const { return m_cdText.performer(); }
  const QString& title() const { return m_cdText.title(); }
  const QString& arranger() const { return m_cdText.arranger(); }
  const QString& songwriter() const { return m_cdText.songwriter(); }
  const QString& composer() const { return m_cdText.composer(); }
  const QString& isrc() const { return m_cdText.isrc(); }
  const QString& cdTextMessage() const { return m_cdText.message(); }
  const K3bCdDevice::TrackCdText& cdText() const { return m_cdText; }
	
  bool copyProtection() const { return m_copy; }
  bool preEmp() const { return m_preEmp; }
	
  void setPregap( const K3b::Msf& p );

  /**
   * @obsolete use setPerformer
   **/
  void setArtist( const QString& a ) { m_cdText.setPerformer(a); emit changed(); }
  void setPerformer( const QString& a ) { m_cdText.setPerformer(a); emit changed(); }

  /**
   * If the file is a mp3-file, it's mp3-tag is used
   */
  void setTitle( const QString& t ) { m_cdText.setTitle(t); emit changed(); }
  void setArranger( const QString& t ) { m_cdText.setArranger(t); emit changed(); }
  void setSongwriter( const QString& t ) { m_cdText.setSongwriter(t); emit changed(); }
  void setComposer( const QString& t ) { m_cdText.setComposer(t); emit changed(); }
  void setIsrc( const QString& t ) { m_cdText.setIsrc(t); emit changed(); }
  void setCdTextMessage( const QString& t ) { m_cdText.setMessage(t); emit changed(); }

  void setCdText( const K3bCdDevice::TrackCdText& cdtext ) { m_cdText = cdtext; emit changed(); }

  void setPreEmp( bool b ) { m_preEmp = b; emit changed(); }
  void setCopyProtection( bool b ) { m_copy = b; emit changed(); }
	
  /**
   * The position the track starts in the file.
   * This normally equals 0.
   */
  const K3b::Msf& trackStart() const;

  /**
   * The position the track ends in the file.
   * This normally equals @p fileLength()
   */
  K3b::Msf trackEnd() const;

  void setTrackStart( const K3b::Msf& );
  void setTrackEnd( const K3b::Msf& );

  /** 
   * @return The raw size of the track in pcm samples (16bit, 44800 kHz, stereo) 
   */
  KIO::filesize_t size() const;

  /**
   * @return The index in the list 
   */
  int index() const;

  int status() const { return m_status; }
  void setStatus( int status ) { m_status = status; }

 signals:
  /**
   * Emitted if the track has been changed.
   */
  void changed();

 protected:
  QPtrList<K3bAudioTrack>* m_parent;
  QString m_filename;

  /** 
   * The module that does all the work (decoding and stuff)
   */
  K3bAudioDecoder* m_module;

 private:	
  K3b::Msf m_trackStartOffset;
  K3b::Msf m_trackEndOffset;
  K3b::Msf m_pregap;
  
  /** Status of the file. see file_status */
  int m_status;

  /** copy protection */
  bool m_copy;
  bool m_preEmp;

  K3bCdDevice::TrackCdText m_cdText;
};


#endif
