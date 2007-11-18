//Added by qt3to4:
#include <Q3PtrList>
/* 
 *
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_MUSICBRAINZ_JOB_H_
#define _K3B_MUSICBRAINZ_JOB_H_

#ifdef HAVE_CONFIG_H
#include <config-k3b.h>
#endif

#ifdef HAVE_MUSICBRAINZ

#include <k3bjob.h>


class K3bAudioTrack;
class K3bThreadJob;
class QWidget;


/**
 * This job tries to determine K3bAudioTrack's title and artist using
 * Musicbrainz.
 */
class K3bMusicBrainzJob : public K3bJob
{
  Q_OBJECT

 public:
  /**
   * \param parent since we do not use this job with a normal progressdialog we need a widget
   *        as parent
   */
  K3bMusicBrainzJob( QWidget* parent = 0 );
  ~K3bMusicBrainzJob();

  bool hasBeenCanceled() const { return m_canceled; }

 signals:
  /**
   * Emitted for each track. This is signal can be used
   * to display further information.
   *
   * \param track The track for which metadata was searched.
   * \param success True if metadata was found
   */
  void trackFinished( K3bAudioTrack* track, bool success );

 public slots:
  void start();
  void cancel();

  void setTracks( const Q3PtrList<K3bAudioTrack>& tracks ) { m_tracks = tracks; }

 private slots:
  void slotTrmPercent( int p );
  void slotTrmJobFinished( bool success );
  void slotMbJobFinished( bool success );

 private:
  class TRMThread;
  class MusicBrainzThread;
  TRMThread* m_trmThread;
  MusicBrainzThread* m_mbThread;

  K3bThreadJob* m_trmJob;
  K3bThreadJob* m_mbJob;

  Q3PtrList<K3bAudioTrack> m_tracks;

  bool m_canceled;
};


#endif

#endif
