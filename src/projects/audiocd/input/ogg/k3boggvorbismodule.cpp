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


#include <config.h>
#include <kdebug.h>


#ifdef OGG_VORBIS

#include "k3boggvorbismodule.h"
#include "../../k3baudiotrack.h"

#include <qfile.h>
#include <qstringlist.h>

#include <kurl.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>


// TODO: only open an OggVorbis_File at in one method that returnes the structure or null on failure

K3bOggVorbisModule::K3bOggVorbisModule( QObject* parent, const char* name )
  : K3bAudioModule( parent, name )
{
  m_oggVorbisFile = new OggVorbis_File;
}


K3bOggVorbisModule::~K3bOggVorbisModule()
{
  delete m_oggVorbisFile;
}


bool K3bOggVorbisModule::initDecodingInternal( const QString& filename )
{
  // open the file
  FILE* file = fopen( QFile::encodeName(filename), "r" );
  if( !file ) {
    kdError() << "(K3bOggVorbisModule) Could not open file " << filename << endl;
    return false;
  }
  else if( ov_open( file, m_oggVorbisFile, 0, 0 ) ) {
    kdError() << "(K3bOggVorbisModule) " << filename << " seems not to to be an ogg vorbis file." << endl;
    fclose( file );
    return false;
  }

  return true;
}


int K3bOggVorbisModule::decodeInternal( char* _data, int maxLen )
{
  int bitStream;
  long bytesRead = ov_read( m_oggVorbisFile, 
			    _data,
			    maxLen,  // max length to be read
			    1,                   // big endian
			    2,                   // word size: 16-bit samples
			    1,                   // signed
			    &bitStream );        // current bitstream

  if( bytesRead == OV_HOLE ) {
    // I think we can go on here?

    kdDebug() << "(K3bOggVorbisModule) OV_HOLE" << endl;

    // recursive new try
    return decodeInternal( _data, maxLen );
  }

  else if( bytesRead == OV_EBADLINK ) {
    kdDebug() << "(K3bOggVorbisModule) OV_EBADLINK" << endl;

    return -1;
  }

  else if( bytesRead < 0 ) {

    // TODO: add a method in the upcoming vorbisLib class that returnes
    // an error string
    // #define OV_FALSE      -1
    // #define OV_EOF        -2
    // #define OV_HOLE       -3

    // #define OV_EREAD      -128
    // #define OV_EFAULT     -129
    // #define OV_EIMPL      -130
    // #define OV_EINVAL     -131
    // #define OV_ENOTVORBIS -132
    // #define OV_EBADHEADER -133
    // #define OV_EVERSION   -134
    // #define OV_ENOTAUDIO  -135
    // #define OV_EBADPACKET -136
    // #define OV_EBADLINK   -137
    // #define OV_ENOSEEK    -138

    kdDebug() << "(K3bOggVorbisModule) Error: " << bytesRead << endl;

    return -1;
  }

  else if( bytesRead == 0 ) {
    kdDebug() << "(K3bOggVorbisModule) successfully finished decoding." << endl;
    return 0;
  }

  else if( bitStream != 0 ) {
    kdDebug() << "(K3bOggVorbisModule) bitstream != 0. Multible bitstreams not supported." << endl;
    return -1;
  }

  else {
    return bytesRead;
  }
}


bool K3bOggVorbisModule::canDecode( const KURL& url )
{
  FILE* file = fopen( QFile::encodeName(url.path()), "r" );
  if( !file ) {
    kdDebug() << "(K3bOggVorbisModule) Could not open file " << url.path() << endl;
    return false;
  }

  OggVorbis_File of;

  if( ov_open( file, &of, 0, 0 ) ) {
    fclose( file );
    return false;
  }

  ov_clear( &of );

  return true;
}


int K3bOggVorbisModule::analyseTrack( const QString& filename, unsigned long& size )
{
  int ret = K3bAudioTitleMetaInfo::OK;

  // do some initialization
  FILE* file = fopen( QFile::encodeName(filename), "r" );
  if( !file ) {
    kdDebug() << "(K3bOggVorbisModule) Could not open file " << filename << endl;
    ret =  K3bAudioTitleMetaInfo::CORRUPT;
  }
  else {
    OggVorbis_File oggVorbisFile;
    if( ov_open( file, &oggVorbisFile, 0, 0 ) ) {
      kdDebug() << "(K3bOggVorbisModule) " << filename << " seems to to be an ogg vorbis file." << endl;

      fclose( file );
      ret = K3bAudioTitleMetaInfo::CORRUPT;
    }
    else {
      // check length of track
      double seconds = ov_time_total( &oggVorbisFile, -1 );
      if( seconds == OV_EINVAL ) {
	kdDebug() << "(K3bOggVorbisModule) Could not determine length of file " << filename << endl;
	ret = K3bAudioTitleMetaInfo::CORRUPT;
      }
      else {
	size = (unsigned long)ceil(seconds * 75.0);
      }

      ov_clear( &oggVorbisFile );
    }
  }

  return ret;
}


bool K3bOggVorbisModule::metaInfo( const QString& filename, K3bAudioTitleMetaInfo& info )
{
  // do some initialization
  FILE* file = fopen( QFile::encodeName(filename), "r" );
  if( !file ) {
    kdDebug() << "(K3bOggVorbisModule) Could not open file " << filename << endl;
    return false;
  }
  else {
    OggVorbis_File oggVorbisFile;
    if( ov_open( file, &oggVorbisFile, 0, 0 ) ) {
      kdDebug() << "(K3bOggVorbisModule) " << filename << " seems to to be an ogg vorbis file." << endl;

      fclose( file );
      return false;
    }
    else {
      // search for artist,title information
      vorbis_comment* vComment = ov_comment( &oggVorbisFile, -1 );
      if( !vComment ) {
	kdDebug() << "(K3bOggVorbisModule) Could not open OggVorbis comment of file " << filename << endl;
      }
      else {
	for( int i = 0; i < vComment->comments; ++i ) {
	  QString comment( vComment->user_comments[i] );
	  QStringList values = QStringList::split( "=", comment );
	  if( values.count() > 1 ) {
	    if( values[0].lower() == "title" )
	      info.setTitle( values[1] );
	    else if( values[0].lower() == "artist" )
	      info.setArtist( values[1] );
	    else if( values[0].lower() == "album" )
	      info.setAlbumTitle( values[1] );
	  }
	}
      }

      ov_clear( &oggVorbisFile );
    }
  }

  return true;
}


void K3bOggVorbisModule::cleanup()
{
  ov_clear( m_oggVorbisFile );
}

#include "k3boggvorbismodule.moc"

#endif
