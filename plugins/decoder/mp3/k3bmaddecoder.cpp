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


//
// Some notes on mp3:
// A mp3 Frame is always samples/samplerate seconds in length
//
//
//
// What we need are raw 16 bit stereo samples at 44100 Hz which results in 588 samples
// per block (2352 bytes: 32*588 bit). 1 second are 75 blocks.
//

#include "k3bmaddecoder.h"
#include "k3bmad.h"

#include <kurl.h>
#include <kdebug.h>
#include <kinstance.h>
#include <klocale.h>

#include <qstring.h>
#include <qfile.h>
#include <qvaluevector.h>

#include <stdlib.h>
#include <cmath>
#include <cstdlib>

#include <config.h>

#ifdef HAVE_LIBID3
#include <id3/misc_support.h>
#endif


int K3bMadDecoder::MaxAllowedRecoverableErrors = 10;



class K3bMadDecoder::Private
{
public:
  Private() {
#ifdef HAVE_LIBID3
    id3Tag = 0;
#endif
  }

  K3bMad* handle;

  QValueVector<unsigned long long> seekPositions;

  bool bOutputFinished;

  char* outputBuffer;
  char* outputPointer;
  char* outputBufferEnd;

#ifdef HAVE_LIBID3
  ID3_Tag* id3Tag;
#endif

  // the first frame header for technical info
  mad_header firstHeader;
  bool vbr;
};




K3bMadDecoder::K3bMadDecoder( QObject* parent, const char* name )
  : K3bAudioDecoder( parent, name )
{
  d = new Private();
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
#ifdef HAVE_LIBID3
  // use id3 stuff
  if( !d->id3Tag ) {
    d->id3Tag = new ID3_Tag( QFile::encodeName(filename()) );
  }

  char* str = 0;

  switch( f ) {
  case META_TITLE:
    str = ID3_GetTitle( d->id3Tag );
    break;
  case META_ARTIST:
    str = ID3_GetArtist( d->id3Tag );
    break;
  case META_SONGWRITER:
    str = ID3_GetLyricist( d->id3Tag );
    break;
  case META_COMMENT:
    str = ID3_GetComment( d->id3Tag );
    break;
  default:
    break;
  }

  if( str != 0 ) {
    QString s(str);
    delete [] str;
    return s;
  }

  return QString::null;

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

  if( !d->handle->skipJunk() )
    return false;

  return true;
}


unsigned long K3bMadDecoder::countFrames()
{
  kdDebug() << "(K3bMadDecoder::countFrames)" << endl << flush;

  unsigned long frames = 0;
  bool error = false;
  d->vbr = false;
  bool bFirstHeaderSaved = false;

  d->seekPositions.clear();

  while( !error && d->handle->findNextHeader() ) {

    mad_timer_add( d->handle->madTimer, d->handle->madFrame->header.duration );

    if( !bFirstHeaderSaved ) {
      bFirstHeaderSaved = true;
      d->firstHeader = d->handle->madFrame->header;
    }
    else {
      if( d->handle->madFrame->header.bitrate != d->firstHeader.bitrate )
	d->vbr = true;

      if( 0 && mad_timer_compare( d->firstHeader.duration, d->handle->madFrame->header.duration ) ) {
	// The Mp3 standard needs every frame to have the same duration
	kdDebug() << "(K3bMadDecoder) frame len differs: old: " 
		  << d->firstHeader.duration.seconds << ":" << d->firstHeader.duration.fraction
		  << " new: " << d->handle->madFrame->header.duration.seconds << ":" << d->handle->madFrame->header.duration.fraction << endl;
	error = true;
      }
    }

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
    // we need the length of the track to be multible of frames (1/75 second)
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
    else if( madDecodeNextFrame() ) {
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


bool K3bMadDecoder::madDecodeNextFrame()
{
  if( d->handle->inputError() || !d->handle->fillStreamBuffer() )
    return false;

  if( mad_frame_decode( d->handle->madFrame, d->handle->madStream ) ) {
    if( d->handle->madStream->error == MAD_ERROR_BUFLEN ) {
      return madDecodeNextFrame();
    }
    if( MAD_RECOVERABLE( d->handle->madStream->error )  ) {
      kdDebug() << "(K3bMadDecoder) recoverable frame level error ("
		<< mad_stream_errorstr(d->handle->madStream) << ")" << endl;

      return madDecodeNextFrame();
    }
    else {
      kdDebug() << "(K3bMadDecoder) unrecoverable frame level error ("
		<< mad_stream_errorstr(d->handle->madStream) << endl;
      
      return false;
    }
  }
  else {
    mad_timer_add( d->handle->madTimer, d->handle->madFrame->header.duration );
    return true;
  }
}


bool K3bMadDecoder::decodeNextHeader()
{
  if( d->handle->inputError() || !d->handle->fillStreamBuffer() )
    return false;

  if( mad_header_decode( &d->handle->madFrame->header, d->handle->madStream ) ) {
    if( d->handle->madStream->error == MAD_ERROR_BUFLEN ) {
      return decodeNextHeader();
    }
    else if( MAD_RECOVERABLE( d->handle->madStream->error ) ) {
      kdDebug() << "(K3bMadDecoder) recoverable frame level error ("
		<< mad_stream_errorstr(d->handle->madStream) << ")" << endl;
      
      return decodeNextHeader();
    }
    else {
      kdDebug() << "(K3bMadDecoder) unrecoverable frame level error ("
		<< mad_stream_errorstr(d->handle->madStream) << endl;
      
      return false;
    }
  }
  else {
    mad_timer_add( d->handle->madTimer, d->handle->madFrame->header.duration );
    return true;
  }
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
#ifdef HAVE_LIBID3
  delete d->id3Tag;
  d->id3Tag = 0;
#endif

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
  s_instance = new KInstance( "k3bmaddecoder" );
}


K3bMadDecoderFactory::~K3bMadDecoderFactory()
{
}


K3bPlugin* K3bMadDecoderFactory::createPluginObject( QObject* parent, 
						     const char* name,
						     const QStringList& )
{
  return new K3bMadDecoder( parent, name );
}

bool K3bMadDecoderFactory::canDecode( const KURL& url )
{
  K3bMad handle;
  if( !handle.open( url.path() ) )
    return false;

  return handle.findNextHeader();

  QFile f(url.path());
  if( !f.open(IO_ReadOnly) ) {
    kdDebug() << "(K3bMadDecoder) could not open file " << url.path() << endl;
    return false;
  }

  // there seem to be mp3 files starting with a lot of zeros
  // we try to skip these.
  // there might still be files with more than bufLen zeros...
  const int bufLen = 4096;
  char buf[bufLen];
  if( f.readBlock( buf, bufLen ) < bufLen ) {
    kdDebug() << "(K3bMadDecoder) unable to read " << bufLen << " bytes from " << url.path() << endl;
    return false;
  }

  // skip any 0
  int i = 0;
  while( i < bufLen && buf[i] == '\0' ) i++;
  if( i == bufLen ) {
    kdDebug() << "(K3bMadDecoder) only zeros found in the beginning of " << url.path() << endl;
    return false;
  }



  //
  // now skip any id3 tags
  //
  while( i < bufLen-10 && 
      ( buf[i] == 'I' && buf[i+1] == 'D' && buf[i+2] == '3' ) &&
      ( (unsigned short)buf[i+3] < 0xff && (unsigned short)buf[i+4] < 0xff ) ) {
    kdDebug() << "(K3bMadDecoder) found id3 magic: ID3 " 
	      << (unsigned short)buf[i+3] << "." << (unsigned short)buf[i+4] << endl;

    int pos = i + ((buf[i+6]<<21)|(buf[i+7]<<14)|(buf[i+8]<<7)|buf[i+9]) + 10;

    kdDebug() << "(K3bMadDecoder) skipping past ID3 tag to " << pos << endl;

    if( !f.at(pos) ) {
      kdDebug() << "(K3bMadDecoder) " << url.path() << ": couldn't seek to " << pos << endl;
      return false;
    }

    if( f.readBlock( buf, bufLen ) < bufLen ) {
      kdDebug() << "(K3bMadDecoder) unable to read " << bufLen << " bytes from " << url.path() << endl;
      return false;
    }
  
    // again skip any 0
    i = 0;
    while( i < bufLen && buf[i] == '\0' ) i++;
    if( i == bufLen ) {
      kdDebug() << "(K3bMadDecoder) only zeros found after the id3 tag in " << url.path() << endl;
      return false;
    }
  }


  // check if we have a RIFF MPEG header
  // libmad seems to be able to decode such files but not to decode these first bytes
  if( ( buf[i] == 'R' && buf[i+1] == 'I' && buf[i+2] == 'F' && buf[i+3] == 'F' &&
	buf[i+8] == 'W' && buf[i+9] == 'A' && buf[i+10] == 'V' && buf[i+11] == 'E' &&
	buf[i+12] == 'f' && buf[i+13] == 'm' &&	buf[i+14] == 't' ) ) {
    kdDebug() << "(K3bMadDecoder) found RIFF, WAVE, and fmt." << endl;
    short m = (short)( buf[i+20] | (buf[i+21]<<8) );
    if( m == 80 ) {
      kdDebug() << "(K3bMadDecoder) found RIFF MPEG magic." << endl;
      return true;
    }
    else if( m == 85 ) {
      kdDebug() << "(K3bMadDecoder) found RIFF MPEG III magic." << endl;
      return true;
    }
    else
      return false;
  }
      


  // let libmad try to decode one frame header
  mad_stream stream;;
  mad_header header;
  mad_stream_init( &stream );
  mad_header_init( &header );

  mad_stream_buffer( &stream, (unsigned char*)&buf[i], bufLen-i );
  stream.error = MAD_ERROR_NONE;
  bool success = true;
  if( mad_header_decode( &header, &stream ) ) {
    kdDebug() << "(K3bMadDecoder) could not find mpeg header." << endl;
    success = false;
  }

  mad_header_finish( &header );
  mad_stream_finish( &stream );
  
  return success;
}

#include "k3bmaddecoder.moc"
