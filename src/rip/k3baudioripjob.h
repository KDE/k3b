/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_AUDIORIP_JOB_H_
#define _K3B_AUDIORIP_JOB_H_

#include <k3bjob.h>

#include "k3baudioripthread.h"
#include <k3bdevice.h>
#include <k3bcddbresult.h>
#include <k3baudioencoder.h>

#include <qvaluevector.h>

class K3bInterferingSystemsHandler;
class K3bThreadJob;


class K3bAudioRipJob : public K3bJob
{
  Q_OBJECT

 public:
  K3bAudioRipJob( K3bJobHandler* hdl, QObject* parent );
  ~K3bAudioRipJob();

  QString jobDescription() const;
  QString jobDetails() const;

 public slots:
  void start();
  void cancel();

  void setDevice( K3bDevice::Device* dev ) { m_thread->setDevice( dev ); }
  void setCddbEntry( const K3bCddbResultEntry& entry ) { m_thread->setCddbEntry( entry ); }
  void setTracksToRip( const QValueVector<QPair<int, QString> >& tracksToRip ) { m_thread->setTracksToRip( tracksToRip ); }
  void setParanoiaMode( int mode ) { m_thread->setParanoiaMode( mode ); }
  void setMaxRetries( int retries ) { m_thread->setMaxRetries( retries ); }
  void setNeverSkip( bool neverSkip ) { m_thread->setNeverSkip( neverSkip ); }
  void setSingleFile( bool singleFile ) { m_thread->setSingleFile( singleFile ); }
  void setWriteCueFile( bool cue ) { m_thread->setWriteCueFile( cue ); }
  void setEncoder( K3bAudioEncoder* encoder ) { m_thread->setEncoder( encoder ); }
  void setWritePlaylist( bool playlist ) { m_thread->setWritePlaylist( playlist ); }
  void setPlaylistFilename( const QString& filename ) { m_thread->setPlaylistFilename( filename ); }
  void setUseRelativePathInPlaylist( bool relative ) { m_thread->setUseRelativePathInPlaylist( relative ); }
  void setUseIndex0( bool index0 ) { m_thread->setUseIndex0( index0 ); }
  void setFileType( const QString& filetype ) { m_thread->setFileType( filetype ); }

 private slots:
  void slotRippingFinished( bool );

 private:
  K3bInterferingSystemsHandler* m_interferingSystemsHandler;
  K3bThreadJob* m_threadJob;
  K3bAudioRipThread* m_thread;
};

#endif
