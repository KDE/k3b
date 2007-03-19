/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#include "k3boggvorbisdecoder.h"

#include <k3bpluginfactory.h>

#include <qfile.h>
#include <qstringlist.h>

#include <kurl.h>
#include <kdebug.h>
#include <klocale.h>

#include <stdio.h>
#include <stdlib.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>


K_EXPORT_COMPONENT_FACTORY( libk3boggvorbisdecoder, K3bPluginFactory<K3bOggVorbisDecoderFactory>( "libk3boggvorbisdecoder" ) )


class K3bOggVorbisDecoder::Private
{
public:
  Private()
    : vInfo(0),
      vComment(0),
      isOpen(false) {
  }

  OggVorbis_File oggVorbisFile;
  vorbis_info* vInfo;
  vorbis_comment* vComment;
  bool isOpen;
};


K3bOggVorbisDecoder::K3bOggVorbisDecoder( QObject* parent, const char* name )
  : K3bAudioDecoder( parent, name )
{
  d = new Private();
}


K3bOggVorbisDecoder::~K3bOggVorbisDecoder()
{
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
    else if( ov_open( file, &d->oggVorbisFile, 0, 0 ) ) {
      kdDebug() << "(K3bOggVorbisDecoder) " << filename() 
		<< " seems not to to be an ogg vorbis file." << endl;
      fclose( file );
      return false;
    }
  }

  d->isOpen = true;
  return true;
}


bool K3bOggVorbisDecoder::analyseFileInternal( K3b::Msf& frames, int& samplerate, int& ch )
{
  cleanup();

  if( openOggVorbisFile() ) {
    // check length of track
    double seconds = ov_time_total( &d->oggVorbisFile, -1 );
    if( seconds == OV_EINVAL ) {
      kdDebug() << "(K3bOggVorbisDecoder) Could not determine length of file " << filename() << endl;
      cleanup();
      return false;
    }
    else {

      d->vInfo = ov_info( &d->oggVorbisFile, -1 /* current bitstream */ );
      d->vComment = ov_comment( &d->oggVorbisFile, -1 );

      // add meta tags
      for( int i = 0; i < d->vComment->comments; ++i ) {
	QString comment = QString::fromUtf8( d->vComment->user_comments[i] );
	QStringList values = QStringList::split( "=", comment );
	if( values.count() > 1 ) {
	  if( values[0].lower() == "title" )
	    addMetaInfo( META_TITLE, values[1] );
	  else if( values[0].lower() == "artist" )
	    addMetaInfo( META_ARTIST, values[1] );
	  else if( values[0].lower() == "description" )
	    addMetaInfo( META_COMMENT, values[1] );
	}
      }


      // add technical infos
      addTechnicalInfo( i18n("Version"), QString::number(d->vInfo->version) );
      addTechnicalInfo( i18n("Channels"), QString::number(d->vInfo->channels) );
      addTechnicalInfo( i18n("Sampling Rate"), i18n("%1 Hz").arg(d->vInfo->rate) );
      if( d->vInfo->bitrate_upper > 0 )
	addTechnicalInfo( i18n("Bitrate Upper"), i18n( "%1 bps" ).arg(d->vInfo->bitrate_upper) );
      if( d->vInfo->bitrate_nominal > 0 )
	addTechnicalInfo( i18n("Bitrate Nominal"), i18n( "%1 bps" ).arg(d->vInfo->bitrate_nominal) );
      if( d->vInfo->bitrate_lower > 0 )
	addTechnicalInfo( i18n("Bitrate Lower"), i18n( "%1 bps" ).arg(d->vInfo->bitrate_lower) );

      frames = K3b::Msf::fromSeconds(seconds);
      samplerate = d->vInfo->rate;
      ch = d->vInfo->channels;

      cleanup();

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


int K3bOggVorbisDecoder::decodeInternal( char* data, int maxLen )
{
  int bitStream = 0;
  long bytesRead = ov_read( &d->oggVorbisFile, 
			    data,
			    maxLen,  // max length to be read
			    1,                   // big endian
			    2,                   // word size: 16-bit samples
			    1,                   // signed
			    &bitStream );        // current bitstream

  if( bitStream != 0 ) {
    kdDebug() << "(K3bOggVorbisDecoder) bitstream != 0. Multible bitstreams not supported." << endl;
    return -1;
  }
  
  else if( bytesRead == OV_HOLE ) {
    kdDebug() << "(K3bOggVorbisDecoder) OV_HOLE" << endl;
    // recursive new try
    return decodeInternal( data, maxLen );
  }

  else if( bytesRead < 0 ) {
    kdDebug() << "(K3bOggVorbisDecoder) Error: " << bytesRead << endl;
    return -1;
  }

  else if( bytesRead == 0 ) {
    kdDebug() << "(K3bOggVorbisDecoder) successfully finished decoding." << endl;
    return 0;
  }

  else {
    return bytesRead;
  }
}


void K3bOggVorbisDecoder::cleanup()
{
  if( d->isOpen )
    ov_clear( &d->oggVorbisFile );
  d->isOpen = false;
  d->vComment = 0;
  d->vInfo = 0;
}


bool K3bOggVorbisDecoder::seekInternal( const K3b::Msf& pos )
{
  return ( ov_pcm_seek( &d->oggVorbisFile, pos.pcmSamples() ) == 0 );
}


QString K3bOggVorbisDecoder::fileType() const
{
  return i18n("Ogg-Vorbis");
}


K3bOggVorbisDecoderFactory::K3bOggVorbisDecoderFactory( QObject* parent, const char* name )
  : K3bAudioDecoderFactory( parent, name )
{
}


K3bOggVorbisDecoderFactory::~K3bOggVorbisDecoderFactory()
{
}


K3bAudioDecoder* K3bOggVorbisDecoderFactory::createDecoder( QObject* parent, 
							    const char* name ) const
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
    kdDebug() << "(K3bOggVorbisDecoder) not an Ogg-Vorbis file: " << url.path() << endl;
    return false;
  }

  ov_clear( &of );

  return true;
}


#include "k3boggvorbisdecoder.moc"
