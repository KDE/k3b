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


K3bOggVorbisModule::K3bOggVorbisModule( QObject* parent, const char* name )
  : K3bAudioModule( parent, name )
{
  m_oggVorbisFile = new OggVorbis_File;
  m_outputBuffer  = new char[OUTPUT_BUFFER_SIZE];
}


K3bOggVorbisModule::~K3bOggVorbisModule()
{
  delete m_oggVorbisFile;
  delete [] m_outputBuffer;
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


int K3bOggVorbisModule::decodeInternal( const char** _data )
{
  long bytesRead = ov_read( m_oggVorbisFile, m_outputBuffer, OUTPUT_BUFFER_SIZE, 1, 2, 1, &m_currentOggVorbisSection );

  if( bytesRead == OV_HOLE ) {
    // TODO: I think we can go on here?

    kdDebug() << "(K3bOggVorbisModule) OV_HOLE" << endl;
      
    return -1;
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

  else {
    // correct data
    *_data = m_outputBuffer;
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


int K3bOggVorbisModule::analyseTrack( const QString& filename, unsigned long& size, K3bAudioTitleMetaInfo& info )
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
	    if( values[0] == "title" )
	      info.setTitle( values[1] );
	    else if( values[0] == "artist" )
	      info.setArtist( values[1] );
	    else if( values[0] == "album" )
	      info.setAlbumTitle( values[1] );
	  }
	}
   }

      ov_clear( &oggVorbisFile );
    }
  }

  return ret;
}

void K3bOggVorbisModule::cleanup()
{
  ov_clear( m_oggVorbisFile );
}

#include "k3boggvorbismodule.moc"

#endif
