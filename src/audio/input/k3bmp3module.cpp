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




K3bMp3Module::K3bMp3Module( K3bAudioTrack* track )
  : K3bAudioModule( track ), m_inputFile( track->absPath() )
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
}


void K3bMp3Module::initializeDecoding()
{
  if( !m_inputFile.isOpen() )
    m_inputFile.open( IO_ReadOnly );

  mad_stream_init( m_madStream );
  mad_timer_reset( m_madTimer );

  m_outputPointer = m_outputBuffer;
  
  m_frameCount = 0;
  m_rawDataAlreadyStreamed = 0;
}


void K3bMp3Module::start()
{
  if( !m_bCountingFramesInProgress && !m_bDecodingInProgress ) {
    m_bDecodingInProgress = true;
    m_bEndOfInput = false;
    m_bOutputFinished = false;
    
    initializeDecoding();

    m_inputFile.at(0);

    mad_frame_init( m_madFrame );
    mad_synth_init( m_madSynth );    
    
    m_decodingTimer->start(0);

    qDebug("(K3bMp3Module) length of track: %li frames.", audioTrack()->length() );
    qDebug("(K3bMp3Module) data to decode:  %li bytes.", m_rawDataLengthToStream );
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
      readSize = m_inputFile.readBlock( (char*)readStart, readSize );
      if( readSize <= 0 ) {
	if( readSize < 0 )
	  qDebug("(K3bMp3Module) read error on bitstream)" );
	else
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

      clearingUp();

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

	    clearingUp();

	    emit finished( false );
	    break;
	  }
	}
      } // if( header not decoded )

      else {
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
    // a mad_synth can consist of at most 1152 frames per channel
    // so we need to make sure to have always 2*1152 byte of output buffer free
    // otherwise we would need to flush the buffer while decoding of a frame is in 
    // process which will end in a lot of checking overhead since we cannot go on immediately
    if( m_outputBufferEnd - m_outputPointer < 4*1152 ) {
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

	  qDebug("(K3bMp3Module) padding data with %i zeros.", ( freeBuffer > dataToPad ? dataToPad : freeBuffer ) );
	}
	else {
	  break;
	}
      }
      else {
	if( mad_frame_decode( m_madFrame, m_madStream ) ) {
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
	      
	      // this should not happen since we only decode if the
	      // output buffer has enough free space
	      if( m_outputPointer == m_outputBufferEnd && i+1 < m_madSynth->pcm.length ) {
		qDebug( "(K3bMp3Module) buffer overflow!" );
		exit(1);
	      }
	    } // pcm conversion

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
    qDebug("to much data streamed");
    exit(1);
  }


  // make sure we don't stream to much
  if( m_rawDataAlreadyStreamed + buffersize > m_rawDataLengthToStream ) {
    bytesToOutput = m_rawDataLengthToStream - m_rawDataAlreadyStreamed;
    qDebug("(K3bMp3Module) decoded data was longer than calculated length. Cutting data." );
    qDebug("(K3bMp3Module) bytes to stream: %li; bytes already streamed: %li; bytes in buffer: %li.",
	   m_rawDataLengthToStream, m_rawDataAlreadyStreamed, buffersize );
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

    qDebug("(K3bMp3Module) end of output.");

    m_decodingTimer->stop();

    m_bDecodingInProgress = false;

    mad_synth_finish( m_madSynth );
    mad_frame_finish( m_madFrame );

    clearingUp();

    emit finished( true );

    qDebug("(K3bMp3Module) finished.");
  }
  else if( m_consumer ) {
    // the timer will be restarted when the consumer
    // emits the corresponding signal (s.a.)
    m_decodingTimer->stop();
  }
}


unsigned short K3bMp3Module::madFixedToUshort( mad_fixed_t fixed )
{
  // round
  fixed += (1L << ( MAD_F_FRACBITS - 16 ));

  // clip
  if( fixed >= MAD_F_ONE )
    fixed = MAD_F_ONE - 1;
  else if( fixed < -MAD_F_ONE )
    fixed = -MAD_F_ONE;

  // quatisize
  return fixed >> (MAD_F_FRACBITS + 1 - 16 );
}


void K3bMp3Module::cancel()
{
  if( m_bCountingFramesInProgress ) {
    qDebug( "(K3bMp3Module) length checking cannot be canceled." );
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

  if( m_consumer )
    m_consumer->disconnect(this);

  m_consumer = 0;
}


#include "k3bmp3module.moc"
