/* 
 *
 * $Id$
 * Copyright (C) 2003-2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2006 Sebastian Trueg <trueg@k3b.org>
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
#include "k3bdevice_export.h"

namespace K3bDevice
{
  struct cdtext_pack;

  class TrackCdText
    {
      friend class Device;
      
    public:
      TrackCdText() {
      }

      void clear() {
	m_title.truncate(0);
	m_performer.truncate(0);
	m_songwriter.truncate(0);
	m_composer.truncate(0);
	m_arranger.truncate(0);
	m_message.truncate(0);
	m_isrc.truncate(0);
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

      friend class CdText;
    };

  class LIBK3BDEVICE_EXPORT CdText : public QValueVector<TrackCdText>
    {
      friend class Device;

    public:
      CdText();
      CdText( const unsigned char* data, int len );
      CdText( const QByteArray& );
      CdText( int size );
      CdText( const CdText& );

      void setRawPackData( const unsigned char*, int );
      void setRawPackData( const QByteArray& );

      QByteArray rawPackData() const;

      bool empty() const {
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
	
	for( unsigned int i = 0; i < count(); ++i )
	  if( !at(i).isEmpty() )
	    return false;

	return true;
      }

      bool isEmpty() const {
	return empty();
      }

      void clear();

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

      void debug() const;

      /**
       * Returns false if found a crc error in the raw cdtext block or it has a
       * wrong length.
       */
      static bool checkCrc( const unsigned char*, int );
      static bool checkCrc( const QByteArray& );
	
    private:
      // TODO: remove this (see above)
      void fixup( QString& s ) { s.replace( '/', "_" ); s.replace( '\"', "_" ); }

      const QString& textForPackType( int packType, unsigned int track ) const;
      unsigned int textLengthForPackType( int packType ) const;
      QByteArray createPackData( int packType, unsigned int& ) const;
      void savePack( cdtext_pack* pack, QByteArray& data, unsigned int& dataFill ) const;
      void appendByteArray( QByteArray& a, const QByteArray& b ) const;

      QString m_title;
      QString m_performer;
      QString m_songwriter;
      QString m_composer;
      QString m_arranger;
      QString m_message;
      QString m_discId;
      QString m_upcEan;
    };

  QCString encodeCdText( const QString& s, bool* illegalChars = 0 );
}

#endif
