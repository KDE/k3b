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


#ifndef _K3B_CDTEXT_H_
#define _K3B_CDTEXT_H_

#include <qstring.h>
#include <qvaluevector.h>

#include <tools/k3baudiotitlemetainfo.h>

namespace K3bCdDevice
{
  // TODO: add language stuff

  class TrackCdText
    {
    public:
      TrackCdText() {
      }

      // TODO: add some more info
      TrackCdText( const K3bAudioTitleMetaInfo& info ) 
	: m_title( info.title() ),
	m_performer( info.artist() ) {
      }

      const QString& title() const { return m_title; }
      const QString& performer() const { return m_performer; }
      const QString& songwriter() const { return m_songwriter; }
      const QString& composer() const { return m_composer; }
      const QString& arranger() const { return m_arranger; }
      const QString& message() const { return m_message; }
      const QString& isrc() const { return m_isrc; }

      void setTitle( const QString& s ) { m_title = s; }
      void setPerformer( const QString& s ) { m_performer = s; }
      void setSongwriter( const QString& s ) { m_songwriter = s; }
      void setComposer( const QString& s ) { m_composer = s; }
      void setArranger( const QString& s ) { m_arranger = s; }
      void setMessage( const QString& s ) { m_message = s; }
      void setIsrc( const QString& s ) { m_isrc = s; }

    private:
      QString m_title;
      QString m_performer;
      QString m_songwriter;
      QString m_composer;
      QString m_arranger;
      QString m_message;
      QString m_isrc;
    };

  class AlbumCdText
    {
    public:
      AlbumCdText() {
      }

      const QString& title() const { return m_title; }
      const QString& performer() const { return m_performer; }
      const QString& songwriter() const { return m_songwriter; }
      const QString& composer() const { return m_composer; }
      const QString& arranger() const { return m_arranger; }
      const QString& message() const { return m_message; }
      const QString& discId() const { return m_discId; }
      const QString& upcEan() const { return m_upcEan; }

      void setTitle( const QString& s ) { m_title = s; }
      void setPerformer( const QString& s ) { m_performer = s; }
      void setSongwriter( const QString& s ) { m_songwriter = s; }
      void setComposer( const QString& s ) { m_composer = s; }
      void setArranger( const QString& s ) { m_arranger = s; }
      void setMessage( const QString& s ) { m_message = s; }
      void setDiscId( const QString& s ) { m_discId = s; }
      void setUpcEan( const QString& s ) { m_upcEan = s; }

      const TrackCdText& trackCdText( int i ) const { return m_trackCdText[i]; }
      void addTrackCdText( const TrackCdText& t ) { m_trackCdText.append(t); }

    private:
      QString m_title;
      QString m_performer;
      QString m_songwriter;
      QString m_composer;
      QString m_arranger;
      QString m_message;
      QString m_discId;
      QString m_upcEan;

      QValueVector<TrackCdText> m_trackCdText;
    };
}

#endif
