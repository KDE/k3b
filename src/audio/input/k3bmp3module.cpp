#include "k3bmp3module.h"
#include "../k3baudiotrack.h"

#include <kurl.h>
#include <kapp.h>
#include <kconfig.h>
#include <kprocess.h>
#include <klocale.h>


// ID3lib-includes
#include <id3/tag.h>
#include <id3/misc_support.h>

#include <qstring.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qtimer.h>

#include <cmath>
#include <iostream>
#include <errno.h>

//#include <mad.h>



K3bMp3Module::K3bMp3Module( K3bAudioTrack* track )
  : K3bAudioModule( track )
{
  // at the beginning the timer is used for counting the frames
  m_decodingTimer = new QTimer( this );
  connect( m_decodingTimer, SIGNAL(timeout()), this, SLOT(slotCountFrames()) );


  m_bCountingFramesInProgress = true;
  m_bDecodingInProgress = false;
  m_bEndOfInput = false;

  m_inputBuffer  = new (unsigned char)[INPUT_BUFFER_SIZE];
  m_outputBuffer = new (unsigned char)[OUTPUT_BUFFER_SIZE];

  m_outputPointer   = m_outputBuffer;
  m_outputBufferEnd = m_outputBuffer + OUTPUT_BUFFER_SIZE;

  memset( m_inputBuffer, 0, INPUT_BUFFER_SIZE );
  memset( m_outputBuffer, 0, OUTPUT_BUFFER_SIZE );

  m_madStream = new mad_stream;
  m_madFrame  = new mad_frame;
  m_madHeader = new mad_header;
  m_madSynth  = new mad_synth;
  m_madTimer  = new mad_timer_t;

  m_inputFile = 0;

  // read id3 tag
  // -----------------------------------------------
  ID3_Tag tag( audioTrack()->absPath().latin1() );
  ID3_Frame* frame = tag.Find( ID3FID_TITLE );
  if( frame )
    audioTrack()->setTitle( QString(ID3_GetString(frame, ID3FN_TEXT )) );
		
  frame = tag.Find( ID3FID_LEADARTIST );
  if( frame )
    audioTrack()->setArtist( QString(ID3_GetString(frame, ID3FN_TEXT )) );

  audioTrack()->setStatus( K3bAudioTrack::OK );

  // check track length
  initializeDecoding();
  mad_header_init( m_madHeader );
  m_decodingTimer->start(0);
}


K3bMp3Module::~K3bMp3Module()
{
  delete [] m_inputBuffer;
  delete [] m_outputBuffer;

  mad_stream_finish( m_madStream );

  if( m_inputFile != 0 )
    fclose( m_inputFile );
}


void K3bMp3Module::initializeDecoding()
{
  if( m_inputFile == 0 )
    m_inputFile = fopen( audioTrack()->absPath().latin1(), "r" );

  mad_stream_init( m_madStream );
  mad_timer_reset( m_madTimer );
  
  m_frameCount = 0;
  m_rawDataAlreadyStreamed = 0;
}


void K3bMp3Module::start( double offset )
{
  if( !m_bCountingFramesInProgress && !m_bDecodingInProgress ) {
    m_bDecodingInProgress = true;
    m_bEndOfInput = false;
    
    initializeDecoding();

    if( offset > 0 && offset < 1 ) {
      fseek( m_inputFile, (long)(offset*(double)audioTrack()->size()), SEEK_SET );
    }
    else
      rewind( m_inputFile );

    mad_frame_init( m_madFrame );
    mad_synth_init( m_madSynth );    
    
    m_decodingTimer->start(0);
  }
}


void K3bMp3Module::fillInputBuffer()
{
  /* The input bucket must be filled if it becomes empty or if
   * it's the first execution of the loop.
   */
  if( m_madStream->buffer == 0 || m_madStream->error == MAD_ERROR_BUFLEN )
    {
      size_t readSize, remaining;
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
      readSize = fread( readStart, 1, readSize, m_inputFile );
      if( readSize <= 0 )
	{
	  if( ferror(m_inputFile) )
	    {
	      qDebug("(K3bMp3Module) read error on bitstream (%s)", strerror(errno) );
	    }
	  if( feof(m_inputFile) )
	    qDebug("(K3bMp3Module) end of input stream" );


	  m_bEndOfInput = true;
	}
      else 
	{
	  // Pipe the new buffer content to libmad's stream decoder facility.
	  mad_stream_buffer( m_madStream, m_inputBuffer, readSize + remaining );
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
      qDebug("(K3bMp3Module) setting length of track %i to ceil(%f) seconds = %li frames", 
	     audioTrack()->index(), seconds, frames );
      
      m_rawDataLengthToStream = frames * 2352;
      
      // finished. Prepare timer for decoding
      m_decodingTimer->stop();
      m_decodingTimer->disconnect();
      connect( m_decodingTimer, SIGNAL(timeout()), this, SLOT(slotDecodeNextFrame()) );

      m_bCountingFramesInProgress = false;
      mad_header_finish( m_madHeader );

      emit finished( true );
      break;
    }
    else { // input ready

      if( mad_header_decode( m_madHeader, m_madStream ) ) {
	if( MAD_RECOVERABLE( m_madStream->error ) ) {
	  qDebug( "(K3bMp3Module) recoverable frame level error (%s)",
		  mad_stream_errorstr(m_madStream) );
	  audioTrack()->setStatus( K3bAudioTrack::RECOVERABLE );
	}
	else {
	  if( m_madStream->error != MAD_ERROR_BUFLEN ) {
	    qDebug( "(K3bMp3Module) unrecoverable frame level error (%s).",
		    mad_stream_errorstr(m_madStream));
	    audioTrack()->setStatus( K3bAudioTrack::CORRUPT );
	    
	    m_bCountingFramesInProgress = false;
	    m_decodingTimer->stop();
	    mad_header_finish( m_madHeader );

	    emit finished( false );
	    break;
	  }
	}
      } // if( header decoded )
    } // if( input ready )

    m_frameCount++;

    mad_timer_add( m_madTimer, m_madHeader->duration );
  } // for( 1000x )
}


void K3bMp3Module::slotDecodeNextFrame()
{
  if( m_bEndOfInput ) {
    // nothing to do anymore (but perhaps there are some zombie timer events)
    return;
  }


  fillInputBuffer();

  // if no input is available anymore test if we need to pad
  if( m_bEndOfInput ) {
    if( m_rawDataLengthToStream > m_rawDataAlreadyStreamed ) {
      qDebug( "(K3bMp3Module) data needs to be padded by %li bytes.", 
	      m_rawDataLengthToStream - m_rawDataAlreadyStreamed );

    }

    finishDecoding();
  }
  else {
    if( mad_frame_decode( m_madFrame, m_madStream ) ) {
      if( MAD_RECOVERABLE( m_madStream->error ) ) {
	qDebug( "(K3bMp3Module) recoverable frame level error (%s)",
		mad_stream_errorstr(m_madStream) );
	audioTrack()->setStatus( K3bAudioTrack::RECOVERABLE );
	return;
      }
      else {
	if( m_madStream->error == MAD_ERROR_BUFLEN )
	  return;
	else {
	  qDebug( "(K3bMp3Module) unrecoverable frame level error (%s).",
		  mad_stream_errorstr(m_madStream));
	  audioTrack()->setStatus( K3bAudioTrack::CORRUPT );
	
	  m_bDecodingInProgress = false;
	  m_decodingTimer->stop();
	  emit finished( false );
	  return;
	}
      }
    }
  
    m_frameCount++;

    mad_timer_add( m_madTimer, m_madFrame->header.duration );

    /* Once decoded the frame is synthesized to PCM samples. No errors
     * are reported by mad_synth_frame();
     */
    mad_synth_frame( m_madSynth, m_madFrame );

    
    /* Synthesized samples must be converted from mad's fixed
     * point number to the consumer format. Here we use unsigned
     * 16 bit big endian integers on two channels. Integer samples
     * are temporarily stored in a buffer that is flushed when
     * full.
     */
    for( int i = 0; i < m_madSynth->pcm.length; i++ )
      {
	unsigned short	sample;
	  
	/* Left channel */
	sample = madFixedToUshort( m_madSynth->pcm.samples[0][i] );
	*(m_outputPointer++) = sample >> 8;
	*(m_outputPointer++) = sample & 0xff;
	  
	/* Right channel. If the decoded stream is monophonic then
	 * the right output channel is the same as the left one.
	 */
	if( MAD_NCHANNELS( &m_madFrame->header ) == 2 )
	  sample = madFixedToUshort( m_madSynth->pcm.samples[1][i] );
	  
	*(m_outputPointer++) = sample >> 8;
	*(m_outputPointer++) = sample & 0xff;
	
	/* Flush the buffer if it is full. */
	if( m_outputPointer == m_outputBufferEnd )
	  {
	    flushOutputBuffer();
	  }
      }
  } // output the synth
}



void K3bMp3Module::finishDecoding()
{
  flushOutputBuffer();

  m_bDecodingInProgress = false;

  mad_synth_finish( m_madSynth );
  mad_frame_finish( m_madFrame );

  emit finished( true );
}


void K3bMp3Module::flushOutputBuffer()
{
  // flush the output buffer
  size_t buffersize = m_outputPointer - m_outputBuffer;
  size_t bytesToOutput = buffersize;
	    
  // make sure we don't stream to much
  if( m_rawDataAlreadyStreamed + buffersize > m_rawDataLengthToStream ) {
    bytesToOutput = m_rawDataLengthToStream - m_rawDataAlreadyStreamed;
    qDebug("(K3bMp3Module) decoded data was longer than calculated length. diff: %i. Cutting data.", 
	   buffersize - bytesToOutput );
  }
  
  m_rawDataAlreadyStreamed += bytesToOutput;

  if( bytesToOutput > 0 )  
    emit output( m_outputBuffer, bytesToOutput );

  m_outputPointer = m_outputBuffer;
}


unsigned short K3bMp3Module::madFixedToUshort( mad_fixed_t fixed )
{
  /* A fixed point number is formed of the following bit pattern:
   *
   * SWWWFFFFFFFFFFFFFFFFFFFFFFFFFFFF
   * MSB                          LSB
   * S ==> Sign (0 is positive, 1 is negative)
   * W ==> Whole part bits
   * F ==> Fractional part bits
   *
   * This pattern contains MAD_F_FRACBITS fractional bits, one
   * should alway use this macro when working on the bits of a fixed
   * point number. It is not guaranteed to be constant over the
   * different platforms supported by libmad.
   *
   * The unsigned short value is formed by the least significant
   * whole part bit, followed by the 15 most significant fractional
   * part bits. Warning: this is a quick and dirty way to compute
   * the 16-bit number, madplay includes much better algorithms.
   */
  fixed = fixed >> ( MAD_F_FRACBITS - 15 );
  return (unsigned short)fixed;
}


void K3bMp3Module::cancel()
{
  if( m_bCountingFramesInProgress ) {
    qDebug( "(K3bMp3Module) length checking cannot be canceled." );
  }
  else if( m_bDecodingInProgress ) {
    m_bDecodingInProgress = false;
    m_decodingTimer->stop();

    emit canceled();
    emit finished( false );
  }
}


void K3bMp3Module::pause()
{
  if( m_bDecodingInProgress )
    m_decodingTimer->stop();
}

void K3bMp3Module::resume()
{
  if( m_bDecodingInProgress )
    m_decodingTimer->start(0);
  else
    qDebug("(K3bMp3Module) tried to resume without decoding process.");
}


#include "k3bmp3module.moc"
