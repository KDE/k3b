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

#include <kdebug.h>


namespace K3bCdDevice
{
  // TODO: add language stuff

  class TrackCdText
    {
      friend class CdDevice;
      
    public:
      TrackCdText() {
      }

      const QString& title() const { return m_title; }
      const QString& performer() const { return m_performer; }
      const QString& songwriter() const { return m_songwriter; }
      const QString& composer() const { return m_composer; }
      const QString& arranger() const { return m_arranger; }
      const QString& message() const { return m_message; }
      const QString& isrc() const { return m_isrc; }

      // TODO: use the real CD-TEXT charset (a modified ISO8859-1)
      void setTitle( const QString& s ) { m_title = s; fixup(m_title); }
      void setPerformer( const QString& s ) { m_performer = s; fixup(m_performer); }
      void setSongwriter( const QString& s ) { m_songwriter = s; fixup(m_songwriter); }
      void setComposer( const QString& s ) { m_composer = s; fixup(m_composer); }
      void setArranger( const QString& s ) { m_arranger = s; fixup(m_arranger); }
      void setMessage( const QString& s ) { m_message = s; fixup(m_message); }
      void setIsrc( const QString& s ) { m_isrc = s; fixup(m_isrc); }

      bool isEmpty() const {
	if( !m_title.isEmpty() )
	  return false;
	if( !m_performer.isEmpty() )
	  return false;
	if( !m_songwriter.isEmpty() )
	  return false;
	if( !m_composer.isEmpty() )
	  return false;
	if( !m_arranger.isEmpty() )
	  return false;
	if( !m_message.isEmpty() )
	  return false;
	if( !m_isrc.isEmpty() )
	  return false;

	return true;
      }

    private:
      // TODO: remove this (see above)
      void fixup( QString& s ) { s.replace( '/', "_" ); s.replace( '\"', "_" ); }

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
      friend class CdDevice;

    public:
      AlbumCdText() {
      }

      AlbumCdText( int size ) {
	resize( size );
      }

      unsigned int count() const {
	return m_trackCdText.count();
      }

      void resize( int size ) {
	m_trackCdText.resize( size );
      }

      bool isEmpty() const {
	if( !m_title.isEmpty() )
	  return false;
	if( !m_performer.isEmpty() )
	  return false;
	if( !m_songwriter.isEmpty() )
	  return false;
	if( !m_composer.isEmpty() )
	  return false;
	if( !m_arranger.isEmpty() )
	  return false;
	if( !m_message.isEmpty() )
	  return false;
	if( !m_discId.isEmpty() )
	  return false;
	if( !m_upcEan.isEmpty() )
	  return false;
	
	for( unsigned int i = 0; i < m_trackCdText.count(); ++i )
	  if( !m_trackCdText[i].isEmpty() )
	    return false;

	return true;
      }

      void clear() {
	m_trackCdText.clear();
	m_title.setLength(0);
	m_performer.setLength(0);
	m_songwriter.setLength(0);
	m_composer.setLength(0);
	m_arranger.setLength(0);
	m_message.setLength(0);
	m_discId.setLength(0);
	m_upcEan.setLength(0);
      }

      const QString& title() const { return m_title; }
      const QString& performer() const { return m_performer; }
      const QString& songwriter() const { return m_songwriter; }
      const QString& composer() const { return m_composer; }
      const QString& arranger() const { return m_arranger; }
      const QString& message() const { return m_message; }
      const QString& discId() const { return m_discId; }
      const QString& upcEan() const { return m_upcEan; }

      // TODO: use the real CD-TEXT charset (a modified ISO8859-1)
      void setTitle( const QString& s ) { m_title = s; fixup(m_title); }
      void setPerformer( const QString& s ) { m_performer = s; fixup(m_performer); }
      void setSongwriter( const QString& s ) { m_songwriter = s; fixup(m_songwriter); }
      void setComposer( const QString& s ) { m_composer = s; fixup(m_composer); }
      void setArranger( const QString& s ) { m_arranger = s; fixup(m_arranger); }
      void setMessage( const QString& s ) { m_message = s; fixup(m_message); }
      void setDiscId( const QString& s ) { m_discId = s; fixup(m_discId); }
      void setUpcEan( const QString& s ) { m_upcEan = s; fixup(m_upcEan); }

      const TrackCdText& trackCdText( int i ) const { return m_trackCdText[i]; }
      void addTrackCdText( const TrackCdText& t ) { m_trackCdText.append(t); }

      void debug() {
	// debug the stuff
	kdDebug() << "CD-TEXT data:" << endl
		  << "Global:" << endl
		  << "  Title:      " << title() << endl
		  << "  Performer:  " << performer() << endl
		  << "  Songwriter: " << songwriter() << endl
		  << "  Composer:   " << composer() << endl
		  << "  Arranger:   " << arranger() << endl
		  << "  Message:    " << message() << endl
		  << "  Disc ID:    " << discId() << endl
		  << "  Upc Ean:    " << upcEan() << endl;
	for( unsigned int i = 0; i < m_trackCdText.count(); ++i ) {
	  kdDebug() << "Track " << (i+1) << ":" << endl
		    << "  Title:      " << trackCdText(i).title() << endl
		    << "  Performer:  " << trackCdText(i).performer() << endl
		    << "  Songwriter: " << trackCdText(i).songwriter() << endl
		    << "  Composer:   " << trackCdText(i).composer() << endl
		    << "  Arranger:   " << trackCdText(i).arranger() << endl
		    << "  Message:    " << trackCdText(i).message() << endl
		    << "  Isrc:       " << trackCdText(i).isrc() << endl;
	}
      }
	
    private:
      // TODO: remove this (see above)
      void fixup( QString& s ) { s.replace( '/', "_" ); s.replace( '\"', "_" ); }

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
