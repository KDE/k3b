/* 
 *
 * $Id$
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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

#include <config.h>

#include "k3bcdtext.h"
#include "k3bcrc.h"

#include <k3bdebug.h>

#include <qtextcodec.h>

#include <string.h>


namespace K3bDevice {

  struct cdtext_pack {
    unsigned char id1;
    unsigned char id2;
    unsigned char id3;
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
    unsigned char dbcc:       1;
    unsigned char blocknum:   3;
    unsigned char charpos:    4;
#else
    unsigned char charpos:    4;
    unsigned char blocknum:   3;
    unsigned char dbcc:       1;
#endif
    unsigned char data[12];
    unsigned char crc[2];
  };

  /**
   * This one is taken from cdrecord
   */
  struct text_size_block {
    char charcode;
    char first_track;
    char last_track;
    char copyr_flags;
    char pack_count[16];
    char last_seqnum[8];
    char language_codes[8];
  };

  void debugRawTextPackData( const unsigned char* data, int dataLen )
  {
    k3bDebug() << endl << " id1    | id2    | id3    | charps | blockn | dbcc | data           | crc |" << endl;
  
    cdtext_pack* pack = (cdtext_pack*)data;
  
    for( int i = 0; i < dataLen/18; ++i ) {
      QString s;
      s += QString( " %1 |" ).arg( pack[i].id1, 6, 16 );
      s += QString( " %1 |" ).arg( pack[i].id2, 6 );
      s += QString( " %1 |" ).arg( pack[i].id3, 6 );
      s += QString( " %1 |" ).arg( pack[i].charpos, 6 );
      s += QString( " %1 |" ).arg( pack[i].blocknum, 6 );
      s += QString( " %1 |" ).arg( pack[i].dbcc, 4 );
//       char str[12];
//       sprintf( str, "%c%c%c%c%c%c%c%c%c%c%c%c",
// 	       pack[i].data[0] == '\0' ? '°' : pack[i].data[0],
// 	       pack[i].data[1] == '\0' ? '°' : pack[i].data[1],
// 	       pack[i].data[2] == '\0' ? '°' : pack[i].data[2],
// 	       pack[i].data[3] == '\0' ? '°' : pack[i].data[3],
// 	       pack[i].data[4] == '\0' ? '°' : pack[i].data[4],
// 	       pack[i].data[5] == '\0' ? '°' : pack[i].data[5],
// 	       pack[i].data[6] == '\0' ? '°' : pack[i].data[6],
// 	       pack[i].data[7] == '\0' ? '°' : pack[i].data[7],
// 	       pack[i].data[8] == '\0' ? '°' : pack[i].data[8],
// 	       pack[i].data[9] == '\0' ? '°' : pack[i].data[9],
// 	       pack[i].data[10] == '\0' ? '°' : pack[i].data[10],
// 	       pack[i].data[11] == '\0' ? '°' : pack[i].data[11] );
//       s += QString( " %1 |" ).arg( "'" + QCString(str,13) + "'", 14 );
//       Q_UINT16 crc = pack[i].crc[0]<<8|pack[i].crc[1];
//       s += QString( " %1 |" ).arg( crc );
      k3bDebug() << s << endl;
    }
  }

}



K3bDevice::CdText::CdText()
{
}


K3bDevice::CdText::CdText( const K3bDevice::CdText& text )
  : QValueVector<K3bDevice::TrackCdText>( text ),
    m_title( text.title() ),
    m_performer( text.performer() ),
    m_songwriter( text.songwriter() ),
    m_composer( text.composer() ),
    m_arranger( text.arranger() ),
    m_message( text.message() ),
    m_discId( text.discId() ),
    m_upcEan( text.upcEan() )
{
}


K3bDevice::CdText::CdText( const unsigned char* data, int len )
{
  setRawPackData( data, len );
}


K3bDevice::CdText::CdText( const QByteArray& b )
{
  setRawPackData( b );
}


K3bDevice::CdText::CdText( int size )
{
  resize( size );
}


void K3bDevice::CdText::clear()
{
  QValueVector<TrackCdText>::clear();

  m_title.setLength(0);
  m_performer.setLength(0);
  m_songwriter.setLength(0);
  m_composer.setLength(0);
  m_arranger.setLength(0);
  m_message.setLength(0);
  m_discId.setLength(0);
  m_upcEan.setLength(0);
}


void K3bDevice::CdText::setRawPackData( const unsigned char* data, int len )
{
  clear();

  int r = len%18;
  if( r > 0 && r != 4 ) {
    k3bDebug() << "(K3bDevice::CdText) invalid cdtext size: " << len << endl;
  }
  else if( len-r > 0 ) {
    debugRawTextPackData( &data[r], len-r );

    cdtext_pack* pack = (cdtext_pack*)&data[r];


    for( int i = 0; i < (len-r)/18; ++i ) {

      if( pack[i].dbcc ) {
	k3bDebug() << "(K3bDevice::CdText) Double byte code not supported" << endl;
	return;
      }

      //
      // For some reason all crc bits are inverted.
      //
      pack[i].crc[0] ^= 0xff;
      pack[i].crc[1] ^= 0xff;

      Q_UINT16 crc = calcX25( reinterpret_cast<unsigned char*>(&pack[i]), 18 );

      pack[i].crc[0] ^= 0xff;
      pack[i].crc[1] ^= 0xff;

      if( crc != 0x0000 )
	k3bDebug() << "(K3bDevice::CdText) CRC invalid!" << endl;


      //
      // pack.data has a length of 12
      //
      // id1 tells us the tracknumber of the data (0 for global)
      // data may contain multiple \0. In that case after every \0 the track number increases 1
      //

      char* nullPos = (char*)pack[i].data - 1;
	
      unsigned int trackNo = pack[i].id2;

      while( nullPos ) {
	if( count() < trackNo )
	  resize( trackNo );

	char* nextNullPos = (char*)::memchr( nullPos+1, '\0', 11 - (nullPos - (char*)pack[i].data) );
	QString txtstr;	    
	if( nextNullPos ) // take all chars up to the next null
	  txtstr = QString::fromLatin1( (char*)nullPos+1, nextNullPos - nullPos - 1 );
	else // take all chars to the end of the pack data (12 bytes)
	  txtstr = QString::fromLatin1( (char*)nullPos+1, 11 - (nullPos - (char*)pack[i].data) );
	  
	//
	// a tab character means to use the same as for the previous track
	//
	if( txtstr == "\t" )
	  txtstr = textForPackType( pack[i].id1, trackNo-1 );

	switch( pack[i].id1 ) {
	case 0x80: // Title
	  if( trackNo == 0 )
	    m_title.append( txtstr );
	  else
	    at(trackNo-1).m_title.append( txtstr );
	  break;

	case 0x81: // Performer
	  if( trackNo == 0 )
	    m_performer.append( txtstr );
	  else
	    at(trackNo-1).m_performer.append( txtstr );
	  break;

	case 0x82: // Writer
	  if( trackNo == 0 )
	    m_songwriter.append( txtstr );
	  else
	    at(trackNo-1).m_songwriter.append( txtstr );
	  break;

	case 0x83: // Composer
	  if( trackNo == 0 )
	    m_composer.append( txtstr );
	  else
	    at(trackNo-1).m_composer.append( txtstr );
	  break;

	case 0x84: // Arranger
	  if( trackNo == 0 )
	    m_arranger.append( txtstr );
	  else
	    at(trackNo-1).m_arranger.append( txtstr );
	  break;

	case 0x85: // Message
	  if( trackNo == 0 )
	    m_message.append( txtstr );
	  else
	    at(trackNo-1).m_message.append( txtstr );
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
	    at(trackNo-1).m_isrc.append( txtstr );
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
    unsigned int i = count();
    while( i > 0 && at(i-1).isEmpty() )
      --i;
    resize( i );
  }
  else
    k3bDebug() << "(K3bDevice::CdText) zero-sized CD-TEXT: " << len << endl; 
}


void K3bDevice::CdText::setRawPackData( const QByteArray& b )
{
  setRawPackData( reinterpret_cast<const unsigned char*>(b.data()), b.size() );
}

QByteArray K3bDevice::CdText::rawPackData() const
{ 
  // FIXME: every pack block may only consist of up to 255 packs.

  unsigned int pc = 0;
  unsigned int alreadyCountedPacks = 0;


  //
  // prepare the size information block
  //
  text_size_block tsize;
  ::memset( &tsize, 0, sizeof(text_size_block) );
  tsize.charcode = 0;              // ISO 8859-1
  tsize.first_track = 1;
  tsize.last_track = count();
  tsize.pack_count[0xF] = 3;
  tsize.language_codes[0] = 0x09;  // English (from cdrecord)


  //
  // create the CD-Text packs
  //
  QByteArray data(0);
  for( int i = 0; i <= 6; ++i ) {
    if( textLengthForPackType( 0x80 | i ) ) {
      appendByteArray( data, createPackData( 0x80 | i, pc ) );
      tsize.pack_count[i] = pc - alreadyCountedPacks;
      alreadyCountedPacks = pc;
    }
  }
  if( textLengthForPackType( 0x8E ) ) {
    appendByteArray( data, createPackData( 0x8E, pc ) );
    tsize.pack_count[0xE] = pc - alreadyCountedPacks;
    alreadyCountedPacks = pc;
  }


  // pc is the number of the next pack and we add 3 size packs
  tsize.last_seqnum[0] = pc + 2;


  //
  // create the size info packs
  //
  unsigned int dataFill = data.size();
  data.resize( data.size() + 3 * sizeof(cdtext_pack) );
  for( int i = 0; i < 3; ++i ) {
    cdtext_pack pack;
    ::memset( &pack, 0, sizeof(cdtext_pack) );
    pack.id1 = 0x8F;
    pack.id2 = i;
    pack.id3 = pc+i;
    ::memcpy( pack.data, &reinterpret_cast<char*>(&tsize)[i*12], 12 );
    savePack( &pack, data, dataFill );
  }

  //
  // add MMC header
  //
  QByteArray a( 4 );
  a[0] = (data.size()+2)>>8 & 0xff;
  a[1] = (data.size()+2) & 0xff;
  a[2] = a[3] = 0;
  appendByteArray( a, data );

  return a;
}


void K3bDevice::CdText::appendByteArray( QByteArray& a, const QByteArray& b ) const
{
  unsigned int oldSize = a.size();
  a.resize( oldSize + b.size() );
  ::memcpy( &a.data()[oldSize], b.data(), b.size() );
}


// this method also creates completely empty packs
QByteArray K3bDevice::CdText::createPackData( int packType, unsigned int& packCount ) const
{
  QByteArray data;
  unsigned int dataFill = 0;
  QCString text = encodeCdText( textForPackType( packType, 0 ) );
  unsigned int currentTrack = 0;
  unsigned int textPos = 0;
  unsigned int packPos = 0;

  //
  // initialize the first pack
  //
  cdtext_pack pack;
  ::memset( &pack, 0, sizeof(cdtext_pack) );
  pack.id1 = packType;
  pack.id3 = packCount;

  //
  // We break this loop when all texts have been packed
  //
  while( 1 ) {
    //
    // Copy as many bytes as possible into the pack
    //
    int copyBytes = QMIN( 12-packPos, text.length()-textPos );
    ::memcpy( reinterpret_cast<char*>(&pack.data[packPos]), &text.data()[textPos], copyBytes );
    textPos += copyBytes;
    packPos += copyBytes;


    //
    // Check if the packdata is full
    //
    if( packPos > 11 ) {

      savePack( &pack, data, dataFill );
      ++packCount;

      //
      // reset the pack
      //
      ::memset( &pack, 0, sizeof(cdtext_pack) );
      pack.id1 = packType;
      pack.id2 = currentTrack;
      pack.id3 = packCount;
      packPos = 0;

      // update the charpos in case we continue a text in the next pack
      if( textPos <= text.length() )
	pack.charpos = ( textPos > 15 ? 15 : textPos );
    }


    //
    // Check if we have no text data left
    //
    if( textPos >= text.length() ) {

      // add one zero spacer byte
      ++packPos;

      ++currentTrack;

      // Check if all texts have been packed
      if( currentTrack > count() ) {
	savePack( &pack, data, dataFill );
	++packCount;

	data.resize( dataFill );
	return data;
      }

      // next text block
      text = encodeCdText( textForPackType( packType, currentTrack ) );
      textPos = 0;
    }
  }
}


void K3bDevice::CdText::savePack( cdtext_pack* pack, QByteArray& data, unsigned int& dataFill ) const
{
  // create CRC
  Q_UINT16 crc = calcX25( reinterpret_cast<unsigned char*>(pack), sizeof(cdtext_pack)-2 );

  // invert for Redbook compliance
  crc ^= 0xffff;

  pack->crc[0] = (crc>>8) & 0xff;
  pack->crc[1] = crc & 0xff;


  // append the pack to data  
  if( data.size() < dataFill + sizeof(cdtext_pack) )
    data.resize( dataFill + sizeof(cdtext_pack), QGArray::SpeedOptim );

  ::memcpy( &data.data()[dataFill], reinterpret_cast<char*>( pack ), sizeof(cdtext_pack) );

  dataFill += sizeof(cdtext_pack);
}


// track 0 means global cdtext
const QString& K3bDevice::CdText::textForPackType( int packType, unsigned int track ) const
{
  switch( packType ) {
  default:
  case 0x80:
    if( track == 0 )
      return title();
    else
      return at(track-1).title();

  case 0x81:
    if( track == 0 )
      return performer();
    else
      return at(track-1).performer();

  case 0x82:
    if( track == 0 )
      return songwriter();
    else
      return at(track-1).songwriter();

  case 0x83:
    if( track == 0 )
      return composer();
    else
      return at(track-1).composer();

  case 0x84:
    if( track == 0 )
      return arranger();
    else
      return at(track-1).arranger();

  case 0x85:
    if( track == 0 )
      return message();
    else
      return at(track-1).message();

  case 0x86:
    if( track == 0 )
      return discId();
    else 
      return QString::null;

//   case 0x87:
//     if( track == 0 )
//       return genre();
//     else
//       return at(track-1).title();

  case 0x8E:
    if( track == 0 )
      return upcEan();
    else
      return at(track-1).isrc();
  }
}


// count the overall length of a certain packtype texts
unsigned int K3bDevice::CdText::textLengthForPackType( int packType ) const
{
  unsigned int len = 0;
  for( unsigned int i = 0; i <= count(); ++i )
    len += encodeCdText( textForPackType( packType, i ) ).length();
  return len;
}


QCString K3bDevice::encodeCdText( const QString& s, bool* illegalChars )
{
  if( illegalChars )
    *illegalChars = false;

  // TODO: do this without QT
  QTextCodec* codec = QTextCodec::codecForName("ISO8859-1");
  if( codec ) {
    QCString encoded = codec->fromUnicode( s );
    return encoded;
  }
  else {
    QCString r(s.length()+1);

    for( unsigned int i = 0; i < s.length(); ++i ) {
      if( s[i].latin1() == 0 ) { // non-ASCII character
	r[i] = ' ';
	if( illegalChars )
	  *illegalChars = true;
      }
      else
	r[i] = s[i].latin1();
    }

    return r;
  }
}


bool K3bDevice::CdText::checkCrc( const QByteArray& rawData )
{
  return checkCrc( reinterpret_cast<const unsigned char*>(rawData.data()), rawData.size() );
}


bool K3bDevice::CdText::checkCrc( const unsigned char* data, int len )
{
  int r = len%18;
  if( r > 0 && r != 4 ) {
    k3bDebug() << "(K3bDevice::CdText) invalid cdtext size: " << len << endl;
    return false;
  }
  else {
    len -= r;

    // TODO: what if the crc field is not used? All zeros?

    for( int i = 0; i < (len-r)/18; ++i ) {
      cdtext_pack* pack = (cdtext_pack*)&data[r];

      //
      // For some reason all crc bits are inverted.
      //
      pack[i].crc[0] ^= 0xff;
      pack[i].crc[1] ^= 0xff;

      int crc = calcX25( reinterpret_cast<unsigned char*>(&pack[i]), 18 );

      pack[i].crc[0] ^= 0xff;
      pack[i].crc[1] ^= 0xff;

      if( crc != 0x0000 )
	return false;
    }

    return true;
  }
}


void K3bDevice::CdText::debug() const
{
  // debug the stuff
  k3bDebug() << "CD-TEXT data:" << endl
	    << "Global:" << endl
	    << "  Title:      '" << title() << "'" << endl
	    << "  Performer:  '" << performer() << "'" << endl
	    << "  Songwriter: '" << songwriter() << "'" << endl
	    << "  Composer:   '" << composer() << "'" << endl
	    << "  Arranger:   '" << arranger() << "'" << endl
	    << "  Message:    '" << message() << "'" << endl
	    << "  Disc ID:    '" << discId() << "'" << endl
	    << "  Upc Ean:    '" << upcEan() << "'" << endl;
  for( unsigned int i = 0; i < count(); ++i ) {
    k3bDebug() << "Track " << (i+1) << ":" << endl
	      << "  Title:      '" << at(i).title() << "'" << endl
	      << "  Performer:  '" << at(i).performer() << "'" << endl
	      << "  Songwriter: '" << at(i).songwriter() << "'" << endl
	      << "  Composer:   '" << at(i).composer() << "'" << endl
	      << "  Arranger:   '" << at(i).arranger() << "'" << endl
	      << "  Message:    '" << at(i).message() << "'" << endl
	      << "  Isrc:       '" << at(i).isrc() << "'" << endl;
  }
}


bool K3bDevice::TrackCdText::operator==( const K3bDevice::TrackCdText& other ) const
{
  return( m_title == other.m_title &&
	  m_performer == other.m_performer &&
	  m_songwriter == other.m_songwriter &&
	  m_composer == other.m_composer &&
	  m_arranger == other.m_arranger &&
	  m_message == other.m_message &&
	  m_isrc == other.m_isrc );
}


bool K3bDevice::TrackCdText::operator!=( const K3bDevice::TrackCdText& other ) const
{
  return !operator==( other );
}


bool K3bDevice::CdText::operator==( const K3bDevice::CdText& other ) const
{
  return( m_title == other.m_title &&
	  m_performer == other.m_performer &&
	  m_songwriter == other.m_songwriter &&
	  m_composer == other.m_composer &&
	  m_arranger == other.m_arranger &&
	  m_message == other.m_message &&
	  m_discId == other.m_discId &&
	  m_upcEan == other.m_upcEan &&
	  QValueVector<TrackCdText>::operator==( other ) );
}


bool K3bDevice::CdText::operator!=( const K3bDevice::CdText& other ) const
{
  return !operator==( other );
}
