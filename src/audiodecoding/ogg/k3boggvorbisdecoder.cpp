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


#include "k3boggvorbisdecoder.h"

#include <qfile.h>
#include <qstringlist.h>

#include <kurl.h>
#include <kdebug.h>
#include <klocale.h>
#include <kinstance.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>


class K3bOggVorbisDecoder::Private
{
public:
  Private()
    : oggVorbisFile(0),
      vInfo(0),
      vComment(0),
      isOpen(false) {
  }

  OggVorbis_File* oggVorbisFile;
  vorbis_info* vInfo;
  vorbis_comment* vComment;
  bool isOpen;
};


K3bOggVorbisDecoder::K3bOggVorbisDecoder( QObject* parent, const char* name )
  : K3bAudioDecoder( parent, name )
{
  d = new Private();
  d->oggVorbisFile = new OggVorbis_File;
}


K3bOggVorbisDecoder::~K3bOggVorbisDecoder()
{
  delete d->oggVorbisFile;
  delete d;
}


bool K3bOggVorbisDecoder::openOggVorbisFile()
{
  if( !d->isOpen ) {
    FILE* file = fopen( QFile::encodeName(filename()), "r" );
    if( !file ) {
      kdDebug() << "(K3bOggVorbisDecoder) Could not open file " << filename() << endl;
      return false;
    }
    else if( ov_open( file, d->oggVorbisFile, 0, 0 ) ) {
      kdDebug() << "(K3bOggVorbisDecoder) " << filename() 
		<< " seems not to to be an ogg vorbis file." << endl;
      fclose( file );
      return false;
    }
  }

  d->isOpen = true;
  return true;
}


bool K3bOggVorbisDecoder::analyseFileInternal()
{
  cleanup();

  if( openOggVorbisFile() ) {
    // check length of track
    double seconds = ov_time_total( d->oggVorbisFile, -1 );
    if( seconds == OV_EINVAL ) {
      kdDebug() << "(K3bOggVorbisDecoder) Could not determine length of file " << filename() << endl;
      cleanup();
      return false;
    }
    else {
      setLength( (unsigned long)ceil(seconds * 75.0) );
      return true;
    }
  }
  else
    return false;
}


bool K3bOggVorbisDecoder::initDecoderInternal()
{
  cleanup();

  return openOggVorbisFile();
}


int K3bOggVorbisDecoder::decodeInternal( char* _data, int maxLen )
{
  int bitStream;
  long bytesRead = ov_read( d->oggVorbisFile, 
			    _data,
			    maxLen,  // max length to be read
			    1,                   // big endian
			    2,                   // word size: 16-bit samples
			    1,                   // signed
			    &bitStream );        // current bitstream

  if( bytesRead == OV_HOLE ) {
    // I think we can go on here?

    kdDebug() << "(K3bOggVorbisDecoder) OV_HOLE" << endl;

    // recursive new try
    return decodeInternal( _data, maxLen );
  }

  else if( bytesRead == OV_EBADLINK ) {
    kdDebug() << "(K3bOggVorbisDecoder) OV_EBADLINK" << endl;

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

    kdDebug() << "(K3bOggVorbisDecoder) Error: " << bytesRead << endl;

    return -1;
  }

  else if( bytesRead == 0 ) {
    kdDebug() << "(K3bOggVorbisDecoder) successfully finished decoding." << endl;
    return 0;
  }

  else if( bitStream != 0 ) {
    kdDebug() << "(K3bOggVorbisDecoder) bitstream != 0. Multible bitstreams not supported." << endl;
    return -1;
  }

  else {
    return bytesRead;
  }
}


QString K3bOggVorbisDecoder::metaInfo( const QString& tag )
{
  if( openOggVorbisFile() ) {

    // search for artist,title information
    if( !d->vComment )
      d->vComment = ov_comment( d->oggVorbisFile, -1 );

    if( !d->vComment ) {
      kdDebug() << "(K3bOggVorbisDecoder) Could not open OggVorbis comment of file "
		<< filename() << endl;
    }
    else {
      for( int i = 0; i < d->vComment->comments; ++i ) {
	QString comment( d->vComment->user_comments[i] );
	QStringList values = QStringList::split( "=", comment );
	if( values.count() > 1 )
	  if( values[0].lower() == tag.lower() )
	    return values[1];
      }
    }
  }

  return QString::null;
}


void K3bOggVorbisDecoder::cleanup()
{
  if( d->isOpen )
    ov_clear( d->oggVorbisFile );
  d->isOpen = false;
  d->vComment = 0;
  d->vInfo = 0;
}


bool K3bOggVorbisDecoder::seekInternal( const K3b::Msf& pos )
{
  openOggVorbisFile();
  return ( ov_pcm_seek( d->oggVorbisFile, pos.totalFrames() ) == 0 );
}


QString K3bOggVorbisDecoder::fileType() const
{
  return i18n("Ogg-Vorbis");
}


QStringList K3bOggVorbisDecoder::supportedTechnicalInfos() const
{
  return QStringList::split( ";", 
			     i18n("Version") + ";" +
			     i18n("Channels") + ";" +
			     i18n("Sampling Rate") + ";" +
			     i18n("Bitrate Upper") + ";" +
			     i18n("Bitrate Nominal") + ";" +
			     i18n("Bitrate Lower") );
}


QString K3bOggVorbisDecoder::technicalInfo( const QString& info ) const
{
  if( !d->vInfo ) {
    d->vInfo = ov_info( d->oggVorbisFile, -1 /* current bitstream */ );
    
    if( d->vInfo ) {
      if( info == i18n("Version") )
	return QString::number(d->vInfo->version);
      else if( info == i18n("Channels") )
	return QString::number(d->vInfo->channels);
      else if( info == i18n("Sampling Rate") )
	return QString::number(d->vInfo->rate);
      else if( info == i18n("Bitrate Upper") )
	  return QString::number(d->vInfo->bitrate_upper);
      else if( info == i18n("Bitrate Nominal") )
	return QString::number(d->vInfo->bitrate_nominal);
      else if( info == i18n("Bitrate Lower") )
	return QString::number(d->vInfo->bitrate_lower);
    }
  }

  return QString::null;
}



K3bOggVorbisDecoderFactory::K3bOggVorbisDecoderFactory( QObject* parent, const char* name )
  : K3bAudioDecoderFactory( parent, name )
{
  s_instance = new KInstance( "k3boggvorbisdecoder" );
}


K3bOggVorbisDecoderFactory::~K3bOggVorbisDecoderFactory()
{
}


K3bPlugin* K3bOggVorbisDecoderFactory::createPluginObject( QObject* parent, 
							   const char* name,
							   const QStringList& )
{
  return new K3bOggVorbisDecoder( parent, name );
}


bool K3bOggVorbisDecoderFactory::canDecode( const KURL& url )
{
  FILE* file = fopen( QFile::encodeName(url.path()), "r" );
  if( !file ) {
    kdDebug() << "(K3bOggVorbisDecoder) Could not open file " << url.path() << endl;
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


#include "k3boggvorbisdecoder.moc"
