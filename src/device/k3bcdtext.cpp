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

#include <config.h>

#include "k3bcdtext.h"

#include <kdebug.h>


namespace K3bCdDevice {

  void debugRawTextPackData( const unsigned char* data, int dataLen )
  {
    kdDebug() << endl << " id1    | id2    | id3    | charps | blockn | dbcc | data           | crc |" << endl;
  
    cdtext_pack* pack = (cdtext_pack*)data;
  
    for( int i = 0; i < dataLen/18; ++i ) {
      QString s;
      s += QString( " %1 |" ).arg( pack[i].id1, 6, 16 );
      s += QString( " %1 |" ).arg( pack[i].id2, 6 );
      s += QString( " %1 |" ).arg( pack[i].id3, 6 );
      s += QString( " %1 |" ).arg( pack[i].charpos, 6 );
      s += QString( " %1 |" ).arg( pack[i].blocknum, 6 );
      s += QString( " %1 |" ).arg( pack[i].dbcc, 4 );
      char str[12];
      sprintf( str, "%c%c%c%c%c%c%c%c%c%c%c%c",
	       pack[i].data[0] == '\0' ? '°' : pack[i].data[0],
	       pack[i].data[1] == '\0' ? '°' : pack[i].data[1],
	       pack[i].data[2] == '\0' ? '°' : pack[i].data[2],
	       pack[i].data[3] == '\0' ? '°' : pack[i].data[3],
	       pack[i].data[4] == '\0' ? '°' : pack[i].data[4],
	       pack[i].data[5] == '\0' ? '°' : pack[i].data[5],
	       pack[i].data[6] == '\0' ? '°' : pack[i].data[6],
	       pack[i].data[7] == '\0' ? '°' : pack[i].data[7],
	       pack[i].data[8] == '\0' ? '°' : pack[i].data[8],
	       pack[i].data[9] == '\0' ? '°' : pack[i].data[9],
	       pack[i].data[10] == '\0' ? '°' : pack[i].data[10],
	       pack[i].data[11] == '\0' ? '°' : pack[i].data[11] );
      s += QString( " %1 |" ).arg( "'" + QCString(str,13) + "'", 14 );
      //      s += QString( " %1 |" ).arg( QString::fromLatin1( (char*)pack[i].crc, 2 ), 3 );
      kdDebug() << s << endl;
    }
  }
}



K3bCdDevice::AlbumCdText::AlbumCdText()
{
}


K3bCdDevice::AlbumCdText::AlbumCdText( const unsigned char* data, int len )
{
  setRawPackData( data, len );
}


K3bCdDevice::AlbumCdText::AlbumCdText( const QByteArray& b )
{
  setRawPackData( b );
}


K3bCdDevice::AlbumCdText::AlbumCdText( int size )
{
  resize( size );
}


void K3bCdDevice::AlbumCdText::clear()
{
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


void K3bCdDevice::AlbumCdText::setRawPackData( const unsigned char* data, int len )
{
  clear();

  int r = len%18;
  if( r > 0 && r != 4 ) {
    kdDebug() << "(K3bCdDevice::AlbumCdText) invalid cdtext size: " << len << endl;
  }
  else if( len-r > 0 ) {
    debugRawTextPackData( &data[r], len-r );

    cdtext_pack* pack = (cdtext_pack*)&data[r];


    for( int i = 0; i < (len-r)/18; ++i ) {

      if( pack[i].dbcc ) {
	kdDebug() << "(K3bCdDevice::AlbumCdText) Double byte code not supported" << endl;
	return;
      }

      //
      // pack.data has a length of 12
      //
      // id1 tells us the tracknumber of the data (0 for global)
      // data may contain multible \0. In that case after every \0 the track number increases 1
      //

      char* nullPos = (char*)pack[i].data - 1;
	
      unsigned int trackNo = pack[i].id2;

      while( nullPos ) {
	if( m_trackCdText.count() < trackNo )
	  resize( trackNo );

	char* nextNullPos = (char*)::memchr( nullPos+1, '\0', 11 - (nullPos - (char*)pack[i].data) );
	QString txtstr;	    
	if( nextNullPos ) // take all chars up to the next null
	  txtstr = QString::fromLatin1( (char*)nullPos+1, nextNullPos - nullPos - 1 );
	else // take all chars to the end of the pack data (12 bytes)
	  txtstr = QString::fromLatin1( (char*)nullPos+1, 11 - (nullPos - (char*)pack[i].data) );
	  
	switch( pack[i].id1 ) {
	case 0x80: // Title
	  if( trackNo == 0 )
	    m_title.append( txtstr );
	  else
	    m_trackCdText[trackNo-1].m_title.append( txtstr );
	  break;

	case 0x81: // Performer
	  if( trackNo == 0 )
	    m_performer.append( txtstr );
	  else
	    m_trackCdText[trackNo-1].m_performer.append( txtstr );
	  break;

	case 0x82: // Writer
	  if( trackNo == 0 )
	    m_songwriter.append( txtstr );
	  else
	    m_trackCdText[trackNo-1].m_songwriter.append( txtstr );
	  break;

	case 0x83: // Composer
	  if( trackNo == 0 )
	    m_composer.append( txtstr );
	  else
	    m_trackCdText[trackNo-1].m_composer.append( txtstr );
	  break;

	case 0x84: // Arranger
	  if( trackNo == 0 )
	    m_arranger.append( txtstr );
	  else
	    m_trackCdText[trackNo-1].m_arranger.append( txtstr );
	  break;

	case 0x85: // Message
	  if( trackNo == 0 )
	    m_message.append( txtstr );
	  else
	    m_trackCdText[trackNo-1].m_message.append( txtstr );
	  break;

	case 0x86: // Disc identification
	  // only global
	  if( trackNo == 0 )
	    m_discId.append( txtstr );
	  break;

	case 0x8e: // Upc or isrc
	  if( trackNo == 0 )
	    m_upcEan.append( txtstr );
	  else
	    m_trackCdText[trackNo-1].m_isrc.append( txtstr );
	  break;

	  // TODO: support for binary data
	  // 0x88: TOC 
	  // 0x89: second TOC
	  // 0x8f: Size information

	default:
	  break;
	}
	  
	trackNo++;
	nullPos = nextNullPos;
      }
    }

    // remove empty fields at the end
    unsigned int i = m_trackCdText.count();
    while( i > 0 && m_trackCdText[i-1].isEmpty() )
      --i;
    resize( i );
  }
  else
    kdDebug() << "(K3bCdDevice::AlbumCdText) zero-sized CD-TEXT: " << len << endl; 
}


void K3bCdDevice::AlbumCdText::setRawPackData( const QByteArray& b )
{
  setRawPackData( reinterpret_cast<const unsigned char*>(b.data()), b.size() );
}

QByteArray K3bCdDevice::AlbumCdText::rawPackData() const
{ 
  kdDebug() << "(K3bCdDevice::AlbumCdText) NEED TO UPDATE RAW PACK DATA." << endl;
  return QByteArray();
}


void K3bCdDevice::AlbumCdText::debug()
{
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
