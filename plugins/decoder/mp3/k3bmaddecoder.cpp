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


//
// Some notes on mp3:
// A mp3 Frame is always samples/samplerate seconds in length
//
//
//
// What we need are raw 16 bit stereo samples at 44100 Hz which results in 588 samples
// per block (2352 bytes: 32*588 bit). 1 second are 75 blocks.
//

#include <config.h>

#include "k3bmaddecoder.h"
#include "k3bmad.h"

#include <k3bpluginfactory.h>

#include <kurl.h>
#include <kdebug.h>
#include <klocale.h>

#include <qstring.h>
#include <qfile.h>
#include <qvaluevector.h>

#include <stdlib.h>
#include <cmath>
#include <cstdlib>

#include <config.h>

#ifdef HAVE_TAGLIB
#include <taglib/tag.h>
#include <taglib/mpegfile.h>
#endif


K_EXPORT_COMPONENT_FACTORY( libk3bmaddecoder, K3bPluginFactory<K3bMadDecoderFactory>( "k3bmaddecoder" ) )


int K3bMadDecoder::MaxAllowedRecoverableErrors = 10;



class K3bMadDecoder::MadDecoderPrivate
{
public:
  MadDecoderPrivate()
    : outputBuffer(0),
      outputPointer(0),
      outputBufferEnd(0) {
    mad_header_init( &firstHeader );
  }

  K3bMad* handle;

  QValueVector<unsigned long long> seekPositions;

  bool bOutputFinished;

  char* outputBuffer;
  char* outputPointer;
  char* outputBufferEnd;

  // the first frame header for technical info
  mad_header firstHeader;
  bool vbr;
};




K3bMadDecoder::K3bMadDecoder( QObject* parent, const char* name )
  : K3bAudioDecoder( parent, name )
{
  d = new MadDecoderPrivate();
  d->handle = new K3bMad();
}


K3bMadDecoder::~K3bMadDecoder()
{
  cleanup();
  delete d->handle;
  delete d;
}


QString K3bMadDecoder::metaInfo( MetaDataField f )
{
#ifdef HAVE_TAGLIB
  TagLib::MPEG::File file( QFile::encodeName( filename() ).data() );

  if ( file.tag() ) {
      switch( f ) {
      case META_TITLE:
          return TStringToQString( file.tag()->title() );
      case META_ARTIST:
          return TStringToQString( file.tag()->artist() );
      case META_COMMENT:
          return TStringToQString( file.tag()->comment() );
      default:
          return QString::null;
      }
  }
  else {
      return QString::null;
  }

#else
  return K3bAudioDecoder::metaInfo( f );
#endif
}


bool K3bMadDecoder::analyseFileInternal( K3b::Msf& frames, int& samplerate, int& ch )
{
  initDecoderInternal();
  frames = countFrames();
  if( frames > 0 ) {
    // we convert mono to stereo all by ourselves. :)
    ch = 2;
    samplerate = d->firstHeader.samplerate;
    return true;
  }
  else
    return false;
}


bool K3bMadDecoder::initDecoderInternal()
{
  cleanup();

  d->bOutputFinished = false;

  if( !d->handle->open( filename() ) )
    return false;

  if( !d->handle->skipTag() )
    return false;

  if( !d->handle->seekFirstHeader() )
    return false;

  return true;
}


unsigned long K3bMadDecoder::countFrames()
{
  kdDebug() << "(K3bMadDecoder::countFrames)" << endl;

  unsigned long frames = 0;
  bool error = false;
  d->vbr = false;
  bool bFirstHeaderSaved = false;

  d->seekPositions.clear();

  while( !error && d->handle->findNextHeader() ) {

    if( !bFirstHeaderSaved ) {
      bFirstHeaderSaved = true;
      d->firstHeader = d->handle->madFrame->header;
    }
    else if( d->handle->madFrame->header.bitrate != d->firstHeader.bitrate )
      d->vbr = true;

    //
    // position in stream: postion in file minus the not yet used buffer
    //
    unsigned long long seekPos = d->handle->inputPos() -
      (d->handle->madStream->bufend - d->handle->madStream->this_frame + 1);

    // save the number of bytes to be read to decode i-1 frames at position i
    // in other words: when seeking to seekPos the next decoded frame will be i
    d->seekPositions.append( seekPos );
  }

  if( !d->handle->inputError() && !error ) {
    // we need the length of the track to be multiple of frames (1/75 second)
    float seconds = (float)d->handle->madTimer->seconds +
      (float)d->handle->madTimer->fraction/(float)MAD_TIMER_RESOLUTION;
    frames = (unsigned long)ceil(seconds * 75.0);
    kdDebug() << "(K3bMadDecoder) length of track " << seconds << endl;
  }

  cleanup();

  kdDebug() << "(K3bMadDecoder::countFrames) end" << endl;

  return frames;
}


int K3bMadDecoder::decodeInternal( char* _data, int maxLen )
{
  d->outputBuffer = _data;
  d->outputBufferEnd = d->outputBuffer + maxLen;
  d->outputPointer = d->outputBuffer;

  bool bOutputBufferFull = false;

  while( !bOutputBufferFull && d->handle->fillStreamBuffer() ) {

    // a mad_synth contains of the data of one mad_frame
    // one mad_frame represents a mp3-frame which is always 1152 samples
    // for us that means we need 4*1152 bytes of output buffer for every frame
    // since one sample has 16 bit
    if( d->outputBufferEnd - d->outputPointer < 4*1152 ) {
      bOutputBufferFull = true;
    }
    else if( d->handle->decodeNextFrame() ) {
      //
      // Once decoded the frame is synthesized to PCM samples. No errors
      // are reported by mad_synth_frame();
      //
      mad_synth_frame( d->handle->madSynth, d->handle->madFrame );

      // this fills the output buffer
      if( !createPcmSamples( d->handle->madSynth ) ) {
	return -1;
      }
    }
    else if( d->handle->inputError() ) {
      return -1;
    }
  }

  // flush the output buffer
  size_t buffersize = d->outputPointer - d->outputBuffer;

  return buffersize;
}


unsigned short K3bMadDecoder::linearRound( mad_fixed_t fixed )
{
  // round
  fixed += (1L << ( MAD_F_FRACBITS - 16 ));

  // clip
  if( fixed >= MAD_F_ONE - 1 )
    fixed = MAD_F_ONE - 1;
  else if( fixed < -MAD_F_ONE )
    fixed = -MAD_F_ONE;

  // quatisize
  return fixed >> (MAD_F_FRACBITS + 1 - 16 );
}


bool K3bMadDecoder::createPcmSamples( mad_synth* synth )
{
  unsigned short nsamples = synth->pcm.length;

  // this should not happen since we only decode if the
  // output buffer has enough free space
  if( d->outputBufferEnd - d->outputPointer < nsamples*4 ) {
    kdDebug() <<  "(K3bMadDecoder) buffer overflow!" << endl;
    return false;
  }

  // now create the output
  for( int i = 0; i < nsamples; i++ ) {

    /* Left channel */
    unsigned short sample = linearRound( synth->pcm.samples[0][i] );
    *(d->outputPointer++) = (sample >> 8) & 0xff;
    *(d->outputPointer++) = sample & 0xff;

    /* Right channel. If the decoded stream is monophonic then
     * the right output channel is the same as the left one.
     */
    if( synth->pcm.channels == 2 )
      sample = linearRound( synth->pcm.samples[1][i] );

    *(d->outputPointer++) = (sample >> 8) & 0xff;
    *(d->outputPointer++) = sample & 0xff;
  } // pcm conversion

  return true;
}


void K3bMadDecoder::cleanup()
{
  d->handle->cleanup();
}


bool K3bMadDecoder::seekInternal( const K3b::Msf& pos )
{
  //
  // we need to reset the complete mad stuff
  //
  if( !initDecoderInternal() )
    return false;

  //
  // search a position
  // This is all hacking, I don't really know what I am doing here... ;)
  //
  double mp3FrameSecs = static_cast<double>(d->firstHeader.duration.seconds)
    + static_cast<double>(d->firstHeader.duration.fraction) / static_cast<double>(MAD_TIMER_RESOLUTION);

  double posSecs = static_cast<double>(pos.totalFrames()) / 75.0;

  // seekPosition to seek after frame i
  unsigned int frame = static_cast<unsigned int>( posSecs / mp3FrameSecs );

  // Rob said: 29 frames is the theoretically max frame reservoir limit (whatever that means...)
  // it seems that mad needs at most 29 frames to get ready
  unsigned int frameReservoirProtect = ( frame > 29 ? 29 : frame );

  frame -= frameReservoirProtect;

  // seek in the input file behind the already decoded data
  d->handle->inputSeek( d->seekPositions[frame] );

  kdDebug() << "(K3bMadDecoder) Seeking to frame " << frame << " with "
	    << frameReservoirProtect << " reservoir frames." << endl;

  // decode some frames ignoring MAD_ERROR_BADDATAPTR errors
  unsigned int i = 1;
  while( i <= frameReservoirProtect ) {
    d->handle->fillStreamBuffer();
    if( mad_frame_decode( d->handle->madFrame, d->handle->madStream ) ) {
      if( MAD_RECOVERABLE( d->handle->madStream->error ) ) {
	if( d->handle->madStream->error == MAD_ERROR_BUFLEN )
	  continue;
	else if( d->handle->madStream->error != MAD_ERROR_BADDATAPTR ) {
	  kdDebug() << "(K3bMadDecoder) Seeking: recoverable mad error ("
		    << mad_stream_errorstr(d->handle->madStream) << ")" << endl;
	  continue;
	}
	else {
	  kdDebug() << "(K3bMadDecoder) Seeking: ignoring ("
		    << mad_stream_errorstr(d->handle->madStream) << ")" << endl;
	}
      }
      else
	return false;
    }

    if( i == frameReservoirProtect )  // synth only the last frame (Rob said so ;)
      mad_synth_frame( d->handle->madSynth, d->handle->madFrame );

    ++i;
  }

  return true;
}


QString K3bMadDecoder::fileType() const
{
  switch( d->firstHeader.layer ) {
  case MAD_LAYER_I:
    return "MPEG1 Layer I";
  case MAD_LAYER_II:
    return "MPEG1 Layer II";
  case MAD_LAYER_III:
    return "MPEG1 Layer III";
  default:
    return "Mp3";
  }
}

QStringList K3bMadDecoder::supportedTechnicalInfos() const
{
  return QStringList::split( ";",
			     i18n("Channels") + ";" +
			     i18n("Sampling Rate") + ";" +
			     i18n("Bitrate") + ";" +
			     i18n("Layer") + ";" +
			     i18n("Emphasis") + ";" +
			     i18n("Copyright") + ";" +
			     i18n("Original") + ";" +
			     i18n("CRC") );
}


QString K3bMadDecoder::technicalInfo( const QString& name ) const
{
  if( name == i18n("Channels") ) {
    switch( d->firstHeader.mode ) {
    case MAD_MODE_SINGLE_CHANNEL:
      return i18n("Mono");
    case MAD_MODE_DUAL_CHANNEL:
      return i18n("Dual");
    case MAD_MODE_JOINT_STEREO:
      return i18n("Joint Stereo");
    case MAD_MODE_STEREO:
      return i18n("Stereo");
    default:
      return "?";
    }
  }
  else if( name == i18n("Sampling Rate") )
    return i18n("%1 Hz").arg(d->firstHeader.samplerate);
  else if( name == i18n("Bitrate") ) {
    if( d->vbr )
      return i18n("VBR");
    else
      return i18n("%1 bps").arg(d->firstHeader.bitrate);
  }
  else if(  name == i18n("Layer") ){
    switch( d->firstHeader.layer ) {
    case MAD_LAYER_I:
      return "I";
    case MAD_LAYER_II:
      return "II";
    case MAD_LAYER_III:
      return "III";
    default:
      return "?";
    }
  }
  else if( name == i18n("Emphasis") ) {
    switch( d->firstHeader.emphasis ) {
    case MAD_EMPHASIS_NONE:
      return i18n("None");
    case MAD_EMPHASIS_50_15_US:
      return i18n("50/15 ms");
    case MAD_EMPHASIS_CCITT_J_17:
      return i18n("CCITT J.17");
    default:
      return i18n("Unknown");
    }
  }
  else if( name == i18n("Copyright") )
    return ( d->firstHeader.flags & MAD_FLAG_COPYRIGHT ? i18n("Yes") : i18n("No") );
  else if( name == i18n("Original") )
    return ( d->firstHeader.flags & MAD_FLAG_ORIGINAL ? i18n("Yes") : i18n("No") );
  else if( name == i18n("CRC") )
    return ( d->firstHeader.flags & MAD_FLAG_PROTECTION ? i18n("Yes") : i18n("No") );
  else
    return QString::null;
}


K3bMadDecoderFactory::K3bMadDecoderFactory( QObject* parent, const char* name )
  : K3bAudioDecoderFactory( parent, name )
{
}


K3bMadDecoderFactory::~K3bMadDecoderFactory()
{
}


K3bAudioDecoder* K3bMadDecoderFactory::createDecoder( QObject* parent,
						      const char* name ) const
{
  return new K3bMadDecoder( parent, name );
}


bool K3bMadDecoderFactory::canDecode( const KURL& url )
{
  //
  // HACK:
  //
  // I am simply no good at this and this detection code is no good as well
  // It always takes waves for mp3 files so we introduce this hack to
  // filter out wave files. :(
  //
  QFile f( url.path() );
  if( !f.open( IO_ReadOnly ) )
    return false;
  char buffer[12];
  if( f.readBlock( buffer, 12 ) != 12 )
    return false;
  if( !qstrncmp( buffer, "RIFF", 4 ) &&
      !qstrncmp( buffer + 8, "WAVE", 4 ) )
    return false;
  f.close();


  K3bMad handle;
  if( !handle.open( url.path() ) )
    return false;

  handle.skipTag();
  if( !handle.seekFirstHeader() )
    return false;

  if( handle.findNextHeader() ) {
    int c = MAD_NCHANNELS( &handle.madFrame->header );
    int layer = handle.madFrame->header.layer;
    unsigned int s = handle.madFrame->header.samplerate;

    //
    // find 4 more mp3 headers (random value since 2 was not enough)
    // This way we get most of the mp3 files while sorting out
    // for example wave files.
    //
    int cnt = 1;
    while( handle.findNextHeader() ) {
      // compare the found headers
      if( MAD_NCHANNELS( &handle.madFrame->header ) == c &&
	  handle.madFrame->header.layer == layer &&
	  handle.madFrame->header.samplerate == s ) {
	// only support layer III for now since otherwise some wave files
	// are taken for layer I
	if( ++cnt >= 5 ) {
	  kdDebug() << "(K3bMadDecoder) valid mpeg 1 layer " << layer
		    << " file with " << c << " channels and a samplerate of "
		    << s << endl;
	  return ( layer == MAD_LAYER_III );
	}
      }
      else
	break;
    }
  }

  kdDebug() << "(K3bMadDecoder) unsupported format: " << url.path() << endl;

  return false;
}

#include "k3bmaddecoder.moc"
