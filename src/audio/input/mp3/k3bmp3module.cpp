/* 
 *
 * $Id: $
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

#include "k3bmp3module.h"
#include "../../k3baudiotrack.h"

#include <kurl.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kprocess.h>
#include <klocale.h>
#include <kfilemetainfo.h>
#include <kdebug.h>
#include <kmimetype.h>

#include <qstring.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qtimer.h>

#include <stdlib.h>
#include <cmath>
#include <cstdlib>


typedef unsigned char k3b_mad_char;

int K3bMp3Module::MaxAllowedRecoverableErrors = 10;


K3bMp3Module::K3bMp3Module( QObject* parent, const char* name )
  : K3bAudioModule( parent, name )
{
  // at the beginning the timer is used for counting the frames
  m_decodingTimer = new QTimer( this );
  m_analysingTimer = new QTimer( this );
  connect( m_analysingTimer, SIGNAL(timeout()), this, SLOT(slotCountFrames()) );
  connect( m_decodingTimer, SIGNAL(timeout()), this, SLOT(slotDecodeNextFrame()) );


  m_bCountingFramesInProgress = false;
  m_bDecodingInProgress = false;
  m_bEndOfInput = false;
  m_bFrameNeedsResampling = false;

  m_inputBuffer  = new k3b_mad_char[INPUT_BUFFER_SIZE];
  m_outputBuffer = new k3b_mad_char[OUTPUT_BUFFER_SIZE];

  m_outputPointer   = m_outputBuffer;
  m_outputBufferEnd = m_outputBuffer + OUTPUT_BUFFER_SIZE;

  m_madStream = new mad_stream;
  m_madFrame  = new mad_frame;
  m_madHeader = new mad_header;
  m_madSynth  = new mad_synth;
  m_madTimer  = new mad_timer_t;


  // resampling
  // a 32 kHz frame has 1152 bytes
  // so to resample it we need about 1587 bytes for resampling
  // all other resampling needs at most 1152 bytes
  m_madResampledLeftChannel = new mad_fixed_t[1600];
  m_madResampledRightChannel = new mad_fixed_t[1600];
}


K3bMp3Module::~K3bMp3Module()
{
  delete [] m_inputBuffer;
  delete [] m_outputBuffer;

  delete [] m_madResampledLeftChannel;
  delete [] m_madResampledRightChannel;
}


void K3bMp3Module::initializeDecoding()
{
  m_inputFile.setName( audioTrack()->absPath() );
  m_inputFile.open( IO_ReadOnly );

  memset( m_inputBuffer, 0, INPUT_BUFFER_SIZE );
  memset( m_outputBuffer, 0, OUTPUT_BUFFER_SIZE );

  mad_stream_init( m_madStream );
  mad_timer_reset( m_madTimer );

  m_outputPointer = m_outputBuffer;
  
  m_frameCount = 0;
  m_rawDataAlreadyStreamed = 0;
  m_rawDataLengthToStream = audioTrack()->size();
  m_bEndOfInput = false;

  // reset the resampling status structures
  m_madResampledStepLeft = 0;
  m_madResampledLastLeft = 0;
  m_madResampledStepRight = 0;
  m_madResampledLastRight = 0;
}


void K3bMp3Module::startDecoding()
{
  if( !m_bCountingFramesInProgress && !m_bDecodingInProgress ) {
    m_bDecodingInProgress = true;
    m_bEndOfInput = false;
    m_bOutputFinished = false;
    
    initializeDecoding();

    mad_frame_init( m_madFrame );
    mad_synth_init( m_madSynth );

    m_decodingTimer->start(0);

    kdDebug() << "(K3bMp3Module) length of track: " << audioTrack()->length() << " frames." << endl;
    kdDebug() << "(K3bMp3Module) data to decode: " << m_rawDataLengthToStream << " bytes." << endl;
  }
}


void K3bMp3Module::fillInputBuffer()
{
  if( m_bEndOfInput )
    return;

  /* The input bucket must be filled if it becomes empty or if
   * it's the first execution of the loop.
   */
  if( m_madStream->buffer == 0 || m_madStream->error == MAD_ERROR_BUFLEN )
    {
      long readSize, remaining;
      unsigned char* readStart;

      if( m_madStream->next_frame != 0 )
	{
	  remaining = m_madStream->bufend - m_madStream->next_frame;
	  memmove( m_inputBuffer, m_madStream->next_frame, remaining );
	  readStart = m_inputBuffer + remaining;
	  readSize = INPUT_BUFFER_SIZE - remaining;
	}
      else
	{
	  readSize  = INPUT_BUFFER_SIZE;
	  readStart = m_inputBuffer;
	  remaining = 0;
	}
			
      // Fill-in the buffer. 
      Q_LONG result = m_inputFile.readBlock( (char*)readStart, readSize );
      if( result <= 0 ) {
	if( result < 0 )
	  kdDebug() << "(K3bMp3Module) read error on bitstream)" << endl;
	else
	  kdDebug() << "(K3bMp3Module) end of input stream" << endl;


	m_bEndOfInput = true;
      }
      else 
	{
	  // Pipe the new buffer content to libmad's stream decoder facility.
	  mad_stream_buffer( m_madStream, m_inputBuffer, result + remaining );
	  m_madStream->error = MAD_ERROR_NONE;
	}
    }
}


void K3bMp3Module::slotCountFrames()
{
  if( m_bEndOfInput ) {
    // nothing to do anymore (but perhaps there are some zombie timer events)
    return;
  }


  // always count 500 frames at a time
  for( int i = 0; i < 500; i++ ) {
    fillInputBuffer();
    
    if( m_bEndOfInput ) {
      // we need the length of the track to be multible of frames (1/75 second)
      float seconds = (float)m_madTimer->seconds + (float)m_madTimer->fraction/(float)MAD_TIMER_RESOLUTION;
      unsigned long frames = (unsigned long)ceil(seconds * 75.0);
      audioTrack()->setLength( frames );
      kdDebug() << "(K3bMp3Module) setting length of track " << audioTrack()->index() << " to ceil(" << seconds
		<< ") seconds = " << frames << " frames" << endl;
      
      m_analysingTimer->stop();

      m_bCountingFramesInProgress = false;
      mad_header_finish( m_madHeader );

      clearingUp();

      emit trackAnalysed( audioTrack() );
      break;
    }
    else { // input ready

      if( mad_header_decode( m_madHeader, m_madStream ) ) {
	if( MAD_RECOVERABLE( m_madStream->error ) ) {
	  kdDebug() << "(K3bMp3Module) recoverable frame level error ("
		    << mad_stream_errorstr(m_madStream) << ")" << endl;

	  m_recoverableErrorCount++;
	  if( m_recoverableErrorCount > MaxAllowedRecoverableErrors ) {
	    kdDebug() << "(K3bMp3Module) found " << MaxAllowedRecoverableErrors 
		      << " recoverable errors in a row. Stopping decoding." << endl;
	    audioTrack()->setStatus( K3bAudioTrack::CORRUPT );

	    m_bCountingFramesInProgress = false;
	    m_analysingTimer->stop();
	    mad_header_finish( m_madHeader );

	    clearingUp();

	    emit trackAnalysed( audioTrack() );
	    break;
	  }

	  audioTrack()->setStatus( K3bAudioTrack::RECOVERABLE );
	}
	else {
	  if( m_madStream->error != MAD_ERROR_BUFLEN ) {
	    kdDebug() << "(K3bMp3Module) unrecoverable frame level error ("
		      << mad_stream_errorstr(m_madStream) << endl;
	    audioTrack()->setStatus( K3bAudioTrack::CORRUPT );
	    
	    m_bCountingFramesInProgress = false;
	    m_analysingTimer->stop();
	    mad_header_finish( m_madHeader );

	    clearingUp();

	    emit trackAnalysed( audioTrack() );
	    break;
	  }
	}
      } // if( header not decoded )

      else {
	m_recoverableErrorCount = 0;

	m_frameCount++;

	mad_timer_add( m_madTimer, m_madHeader->duration );
      }
    } // if( input ready )

  } // for( 1000x )
}


void K3bMp3Module::slotConsumerReady()
{
  if( m_bDecodingInProgress )
    m_decodingTimer->start(0);
}


void K3bMp3Module::slotDecodeNextFrame()
{
  if( !m_bDecodingInProgress || m_bOutputFinished ) {
    // nothing to do anymore (but perhaps there are some zombie timer events)
    return;
  }

  bool bOutputBufferFull = false;

  while( !bOutputBufferFull ) {

    // a mad_synth contains of the data of one mad_frame
    // one mad_frame represents a mp3-frame which is always 1152 samples
    // for us that means we need 4*1152 bytes of output buffer for every frame
    // since one sample has 16 bit
    // special case: resampling from 32000 Hz: this results 1587 samples per channel
    // so to always have enough free buffer we use 4*1600 (This is bad, this needs some redesign!)
    if( m_outputBufferEnd - m_outputPointer < 4*1600 ) {
      bOutputBufferFull = true;
    }
    else {
      fillInputBuffer();

      if( m_bEndOfInput ) {
	// now check if we need to pad with zeros
	size_t bufferSize = m_outputPointer - m_outputBuffer;
	if( m_rawDataLengthToStream > m_rawDataAlreadyStreamed + bufferSize ) {
	  // pad as much as possible
	  size_t freeBuffer = OUTPUT_BUFFER_SIZE - bufferSize;
	  unsigned long dataToPad = m_rawDataLengthToStream - m_rawDataAlreadyStreamed - bufferSize;
	  memset( m_outputPointer, 0, freeBuffer );
	  m_outputPointer += ( freeBuffer > dataToPad ? dataToPad : freeBuffer );

	  kdDebug() << "(K3bMp3Module) padding data with " 
		    << ( freeBuffer > dataToPad ? dataToPad : freeBuffer ) << " zeros." << endl;
	}
	else {
	  break;
	}
      }
      else {
	if( mad_frame_decode( m_madFrame, m_madStream ) ) {
	  if( MAD_RECOVERABLE( m_madStream->error ) ) {
	    kdDebug() << "(K3bMp3Module) recoverable frame level error ("
		      << mad_stream_errorstr(m_madStream) << ")" << endl;
	    audioTrack()->setStatus( K3bAudioTrack::RECOVERABLE );
	  }
	  else {
	    if( m_madStream->error != MAD_ERROR_BUFLEN ) {
	      kdDebug() << "(K3bMp3Module) unrecoverable frame level error ("
			<< mad_stream_errorstr(m_madStream) << endl;
	      audioTrack()->setStatus( K3bAudioTrack::CORRUPT );
	
	      m_bDecodingInProgress = false;
	      m_decodingTimer->stop();

	      clearingUp();

	      emit finished( false );
	      return;
	    }
	  }
	} // mad frame could not be decoded

	else {
  
	  m_frameCount++;

	  mad_timer_add( m_madTimer, m_madFrame->header.duration );

	  /* Once decoded the frame is synthesized to PCM samples. No errors
	   * are reported by mad_synth_frame();
	   */
	  mad_synth_frame( m_madSynth, m_madFrame );

	  // this does the resampling if needed
	  // and fills the output buffer
	  createPcmSamples( m_madSynth );

	} // mad frame successfully decoded

      } // input ready

    } // output buffer not full yet

  } // while loop

  // now the buffer is full and needs to be streamed to
  // the consumer
  // if no consumer is set decoding will continue after
  // the output signal has been emitted
  // otherwise it will be stopped until the consumer
  // emitted the ready signal

  // flush the output buffer
  size_t buffersize = m_outputPointer - m_outputBuffer;
  size_t bytesToOutput = buffersize;
	    
  if( m_rawDataAlreadyStreamed > m_rawDataLengthToStream ) {
    kdDebug() << "(K3bMp3Module) to much data streamed" << endl;
    exit(1);
  }


  // make sure we don't stream to much
  if( m_rawDataAlreadyStreamed + buffersize > m_rawDataLengthToStream ) {
    bytesToOutput = m_rawDataLengthToStream - m_rawDataAlreadyStreamed;
    kdDebug() << "(K3bMp3Module) decoded data was longer than calculated length. Cutting data." << endl;
    kdDebug() << "(K3bMp3Module) bytes to stream: "
	      << m_rawDataLengthToStream 
	      << "; bytes already streamed: "
	      << m_rawDataAlreadyStreamed
	      << "; bytes in buffer: " 
	      << buffersize << "." << endl;
  }
  
  m_rawDataAlreadyStreamed += bytesToOutput;

  if( bytesToOutput > 0 ) {
    emit output( m_outputBuffer, bytesToOutput );
  }
  else {
    m_bOutputFinished = true;
  }

  m_outputPointer = m_outputBuffer;

  if( m_rawDataAlreadyStreamed < m_rawDataLengthToStream )
    emit percent( (int)(100.0* (double)m_rawDataAlreadyStreamed / (double)m_rawDataLengthToStream) );
  else
    emit percent( 100 );

  if( m_rawDataLengthToStream == m_rawDataAlreadyStreamed )
    m_bOutputFinished = true;

  if( m_bOutputFinished ) {

    kdDebug() << "(K3bMp3Module) end of output." << endl;

    m_decodingTimer->stop();

    m_bDecodingInProgress = false;

    mad_synth_finish( m_madSynth );
    mad_frame_finish( m_madFrame );

    clearingUp();

    emit finished( true );

    kdDebug() << "(K3bMp3Module) finished." << endl;
  }
  else {
    m_decodingTimer->stop();
  }
}


unsigned short K3bMp3Module::linearRound( mad_fixed_t fixed )
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


void K3bMp3Module::createPcmSamples( mad_synth* synth )
{
  static mad_fixed_t const resample_table[9] =
    {
      MAD_F(0x116a3b36) /* 1.088435374 */,
      MAD_F(0x10000000) /* 1.000000000 */,
      MAD_F(0x0b9c2779) /* 0.725623583 */,
      MAD_F(0x08b51d9b) /* 0.544217687 */,
      MAD_F(0x08000000) /* 0.500000000 */,
      MAD_F(0x05ce13bd) /* 0.362811791 */,
      MAD_F(0x045a8ecd) /* 0.272108844 */,
      MAD_F(0x04000000) /* 0.250000000 */,
      MAD_F(0x02e709de) /* 0.181405896 */, 
    };



  mad_fixed_t* leftChannel = synth->pcm.samples[0];
  mad_fixed_t* rightChannel = synth->pcm.samples[1];
  unsigned short nsamples = synth->pcm.length;

  // check if we need to resample
  if( synth->pcm.samplerate != 44100 ) {

    switch ( synth->pcm.samplerate ) {
    case 48000:
      m_madResampledRatio = resample_table[0];
      break;
    case 44100:
      m_madResampledRatio = resample_table[1];
      break;
    case 32000:
      m_madResampledRatio = resample_table[2];
      break;
    case 24000:
      m_madResampledRatio = resample_table[3];
      break;
    case 22050:
      m_madResampledRatio = resample_table[4];
      break;
    case 16000:
      m_madResampledRatio = resample_table[5];
      break;
    case 12000:
      m_madResampledRatio = resample_table[6];
      break;
    case 11025:
      m_madResampledRatio = resample_table[7];
      break;
    case  8000:
      m_madResampledRatio = resample_table[8];
      break;
    default:
      kdDebug() << "(K3bMp3Module) Error: not a supported samplerate: " << synth->pcm.samplerate << endl;
      m_bDecodingInProgress = false;
      m_decodingTimer->stop();
      
      clearingUp();
      
      emit finished( false );
      return;
    }


    resampleBlock( leftChannel, synth->pcm.length, m_madResampledLeftChannel, m_madResampledLastLeft, 
		   m_madResampledStepLeft );
    nsamples = resampleBlock( rightChannel, synth->pcm.length, m_madResampledRightChannel, 
			      m_madResampledLastRight, m_madResampledStepRight );

    leftChannel = m_madResampledLeftChannel;
    rightChannel = m_madResampledRightChannel;
  }



  // now create the output
  for( int i = 0; i < nsamples; i++ )
    {
      unsigned short	sample;
      
      /* Left channel */
      sample = linearRound( leftChannel[i] );
      *(m_outputPointer++) = (sample >> 8) & 0xff;
      *(m_outputPointer++) = sample & 0xff;
      
      /* Right channel. If the decoded stream is monophonic then
       * the right output channel is the same as the left one.
       */
      if( synth->pcm.channels == 2 )
	sample = linearRound( rightChannel[i] );
      
      *(m_outputPointer++) = (sample >> 8) & 0xff;
      *(m_outputPointer++) = sample & 0xff;
      
      // this should not happen since we only decode if the
      // output buffer has enough free space
      if( m_outputPointer == m_outputBufferEnd && i+1 < nsamples ) {
	kdDebug() <<  "(K3bMp3Module) buffer overflow!" << endl;
	exit(1);
      }
    } // pcm conversion
}


unsigned int K3bMp3Module::resampleBlock( mad_fixed_t const *source, 
					  unsigned int nsamples,
					  mad_fixed_t* target,
					  mad_fixed_t& last,
					  mad_fixed_t& step )
{
  /*
   * This resampling algorithm is based on a linear interpolation, which is
   * not at all the best sounding but is relatively fast and efficient.
   *
   * A better algorithm would be one that implements a bandlimited
   * interpolation.
   *
   * taken from madplay
   */


  mad_fixed_t const *end, *begin;
  end   = source + nsamples;
  begin = target;

  if (step < 0) {
    step = mad_f_fracpart(-step);

    while (step < MAD_F_ONE) {
       *target++ = step ?
 	last + mad_f_mul(*source - last, step) 
 	: last;

       step += m_madResampledRatio;
      if (((step + 0x00000080L) & 0x0fffff00L) == 0)
	step = (step + 0x00000080L) & ~0x0fffffffL;
    }

    step -= MAD_F_ONE;
  }


  while (end - source > 1 + mad_f_intpart(step)) {
    source += mad_f_intpart(step);
    step = mad_f_fracpart(step);

    *target++ = step ?
      *source + mad_f_mul(source[1] - source[0], step) 
      : *source;

    step += m_madResampledRatio;
    if (((step + 0x00000080L) & 0x0fffff00L) == 0)
      step = (step + 0x00000080L) & ~0x0fffffffL;
  }

  if (end - source == 1 + mad_f_intpart(step)) {
    last = end[-1];
    step = -step;
  }
  else
    step -= mad_f_fromint(end - source);
  
  return target - begin;
}


void K3bMp3Module::cancel()
{
  if( m_bCountingFramesInProgress ) {
    kdDebug() << "(K3bMp3Module) length checking cannot be canceled." << endl;
  }
  else if( m_bDecodingInProgress ) {
    m_bDecodingInProgress = false;
    m_decodingTimer->stop();

    clearingUp();

    mad_synth_finish( m_madSynth );
    mad_frame_finish( m_madFrame );

    emit canceled();
    emit finished( false );
  }
}


void K3bMp3Module::clearingUp()
{
  m_inputFile.close();

  mad_stream_finish( m_madStream );
}


bool K3bMp3Module::canDecode( const KURL& url )
{
//   static const QString mime_types[] = {
//     "audio/mp3", "audio/x-mp3", "audio/mpg3", "audio/x-mpg3", "audio/mpeg3", "audio/x-mpeg3",
//     "audio/mp2", "audio/x-mp2", "audio/mpg2", "audio/x-mpg2", "audio/mpeg2", "audio/x-mpeg2",
//     "audio/mp1", "audio/x-mp1", "audio/mpg1", "audio/x-mpg1", "audio/mpeg1", "audio/x-mpeg1",
//     "audio/mpeg", "audio/x-mpeg",
//     QString::null,
//   };

//   QString mimetype = KMimeType::findByFileContent( url.path(), 0 )->name();
//   kdDebug() << "(K3bMp3Module) mimetype: " << mimetype << endl;

//   for( int i = 0; !mime_types[i].isNull(); ++i )
//     if( mime_types[i] == mimetype )
//       return true;

  // no success with the mimetype
  // since sometimes the mimetype system does not work we try it on our own


  QFile f(url.path());
  if( !f.open(IO_ReadOnly) ) {
    kdDebug() << "(K3bMp3Module) could not open file " << url.path() << endl;
    return false;
  }

  // there seem to be mp3 files starting with a lot of zeros
  // we try to skip these.
  // there might still be files with more than bufLen zeros...
  const int bufLen = 4096;
  char buf[bufLen];
  if( f.readBlock( buf, bufLen ) < bufLen ) {
    kdDebug() << "(K3bMp3Module) unable to read " << bufLen << " bytes from " << url.path() << endl;
    f.close();
    return false;
  }
  f.close();

  // skip any 0
  int i = 0;
  while( i < bufLen && buf[i] == '\0' ) i++;
  if( i == bufLen ) {
    kdDebug() << "(K3bMp3Module) only zeros found in the beginning of " << url.path() << endl;
    return false;
  }



  // now check if the file starts with an id3 tag
  if( i < bufLen-5 && 
      ( buf[i] == 'I' && buf[i+1] == 'D' && buf[i+2] == '3' ) &&
      ( (unsigned short)buf[i+3] < 0xff && (unsigned short)buf[i+4] < 0xff ) ) {
    kdDebug() << "(K3bMp3Module) found id3 magic: ID3 " 
	      << (unsigned short)buf[i+3] << "." << (unsigned short)buf[i+4] << endl;
    return true;
  }

  // check if we have a RIFF MPEG header
  // libmad seems to be able to decode such files but not to decode these first bytes
  if( ( buf[i] == 'R' && buf[i+1] == 'I' && buf[i+2] == 'F' && buf[i+3] == 'F' &&
	buf[i+8] == 'W' && buf[i+9] == 'A' && buf[i+10] == 'V' && buf[i+11] == 'E' &&
	buf[i+12] == 'f' && buf[i+13] == 'm' &&	buf[i+14] == 't' ) ) {
    kdDebug() << "(K3bMp3Module) found RIFF, WAVE, and fmt." << endl;
    short m = (short)( buf[i+20] | (buf[i+21]<<8) );
    if( m == 80 ) {
      kdDebug() << "(K3bMp3Module) found RIFF MPEG magic." << endl;
      return true;
    }
    else if( m == 85 ) {
      kdDebug() << "(K3bMp3Module) found RIFF MPEG III magic." << endl;
      return true;
    }
    else
      return false;
  }
      


  // here no id3 tag could be found
  // so let libmad try to decode one frame header
  mad_stream stream;;
  mad_header header;
  mad_stream_init( &stream );
  mad_header_init( &header );

  mad_stream_buffer( &stream, (unsigned char*)&buf[i], bufLen-i );
  stream.error = MAD_ERROR_NONE;
  bool success = true;
  if( mad_header_decode( &header, &stream ) ) {
    kdDebug() << "(K3bMp3Module) could not find mpeg header." << endl;
    success = false;
  }

  mad_header_finish( &header );
  mad_stream_finish( &stream );
  
  return success;
}


void K3bMp3Module::analyseTrack()
{
  KFileMetaInfo metaInfo( audioTrack()->absPath() );
  if( !metaInfo.isEmpty() && metaInfo.isValid() ) {
    
    KFileMetaInfoItem artistItem = metaInfo.item( "Artist" );
    KFileMetaInfoItem titleItem = metaInfo.item( "Title" );
    
    if( artistItem.isValid() )
      audioTrack()->setArtist( artistItem.string() );

    if( titleItem.isValid() )
      audioTrack()->setTitle( titleItem.string() );
  }


  audioTrack()->setStatus( K3bAudioTrack::OK );

  // check track length
  initializeDecoding();
  mad_header_init( m_madHeader );
  m_bCountingFramesInProgress = true;
  m_bDecodingInProgress = false;
  m_recoverableErrorCount = 0;

  m_analysingTimer->start(0);
}


void K3bMp3Module::stopAnalysingTrack()
{
  if( m_bCountingFramesInProgress ) {
    m_analysingTimer->stop();
    mad_header_finish( m_madHeader );
    clearingUp();
  }
}

#include "k3bmp3module.moc"
