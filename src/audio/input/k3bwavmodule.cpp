#include "k3bwavmodule.h"
#include "../k3baudiotrack.h"

#include <kapp.h>
#include <kconfig.h>
#include <kurl.h>
#include <kprocess.h>
#include <qtimer.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <cmath>

#include "../../device/util.h"
#include "../../device/Sample.h"



K3bWavModule::K3bWavModule( K3bAudioTrack* track )
  : K3bAudioModule( track )
{
  m_convertingProcess = new KShellProcess();
  m_streamingTimer = new QTimer( this );
  m_rawData = 0;

  track->setBufferFile( track->absPath() );

  // try to determine the track length
  unsigned long dataLength;
  long buffy;
  if( waveLength( track->absPath().latin1(),
		  0,
		  &buffy,
		  &dataLength ) == 0 )
    {
      qDebug( "(K3bWavModule) Could determine length of track: %ld samples, %f frames", 
	      dataLength, (float)dataLength / 588.0 );
      qDebug( "(K3bWavModule) Setting track's length to %f frames", ceil( (double)dataLength / 588.0 ) );

      audioTrack()->setLength( (int)ceil( (double)dataLength / 588.0 ), true );
    }
  else
    qDebug( "(K3bWavModule) Could not determine length of track!" );
		  

  // test stuff
  m_testRawData = 0;
  connect( this, SIGNAL(output(char*, int)), this, SLOT(slotTestCountOutput(char*,int)) );
  connect( this, SIGNAL(finished()), this, SLOT(slotTestOutputFinished()) );
  getStream();
}


K3bWavModule::~K3bWavModule()
{
  delete m_convertingProcess;
}


KURL K3bWavModule::writeToWav( const KURL& url )
{
  // we need an asynronous finished signal
  m_streamingTimer->disconnect();
  connect( m_streamingTimer, SIGNAL(timeout()), this, SIGNAL(finished()) );
  m_streamingTimer->start(0, true);

  return url;
}


void K3bWavModule::getStream()
{
  if( m_convertingProcess->isRunning() )
    return;

  m_convertingProcess->clearArguments();
  m_convertingProcess->disconnect();
  connect( m_convertingProcess, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotOutputData(KProcess*,char*, int)) );
//   connect( m_convertingProcess, SIGNAL(receivedStdout(KProcess*, char*, int)), 
// 	   this, SLOT(slotCountRawData(KProcess*, char*, int)) );
//   connect( m_convertingProcess, SIGNAL(receivedStderr(KProcess*, char*, int)), 
// 	   this, SLOT(slotParseStdErrOutput(KProcess*, char*, int)) );
  connect( m_convertingProcess, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotConvertingFinished()) );


  kapp->config()->setGroup( "External Programs");
  
  // convert the file to big endian with sox
  *m_convertingProcess << kapp->config()->readEntry( "sox path", "/usr/bin/sox" );

  *m_convertingProcess << "-V";
  // input
  *m_convertingProcess << QString( "\"%1\"" ).arg( audioTrack()->absPath() );

  // output options
  *m_convertingProcess << "-t" << "raw";    // filetype raw 
  *m_convertingProcess << "-r" << "44100";  // samplerate
  *m_convertingProcess << "-c" << "2";      // channels
  *m_convertingProcess << "-s";             // signed linear data
  *m_convertingProcess << "-w";             // 16-bit words
  *m_convertingProcess << "-x";             // swap byte order
  *m_convertingProcess << "-";              // output to stdout


  if( !m_convertingProcess->start( KProcess::NotifyOnExit, KProcess::Stdout ) )
    qDebug( "(K3bWavModule) could not start sox process." );
  else
    qDebug( "started sox" );
}


void K3bWavModule::slotOutputData(KProcess*, char* data, int len)
{
  m_rawData += len;
  emit output(data, len);
}


void K3bWavModule::slotConvertingFinished()
{
  // sox does not ensure it's output to be a multible of blocks (2352 bytes)
  // since we round up the number of frames in slotCountRawDataFinished
  // we need to pad by ourself to ensure cdrdao splits our data stream
  // correcty
  int diff = m_rawData % 2352;

 if( diff > 0 )
   {
     int bytesToPad = 2352 - diff;
     char data[bytesToPad];
     for( int i = 0; i < bytesToPad; i++ )
       data[i] = 0;
     
     emit output( data, bytesToPad );
   }

  emit finished();
}


void K3bWavModule::slotParseStdErrOutput(KProcess*, char* output, int len)
{
  qDebug( QString::fromLatin1( output, len ) );
}


void K3bWavModule::slotTestCountOutput(char*, int len )
{
  m_testRawData += len;
}

void K3bWavModule::slotTestOutputFinished()
{
  qDebug( "(K3bWavModule) Test: got %ld bytes from module. That are %f frames.", 
	  m_testRawData, (float)m_testRawData / 2352.0 );
}


// Determines length of header and audio data for WAVE files. 'hdrlen' is
// filled with length of WAVE header in bytes. 'datalen' is filled with
// length of audio data in samples (if != NULL).
// return: 0: OK
//         1: error occured
//         2: illegal WAVE file
int K3bWavModule::waveLength(const char *filename, long offset,
			  long *hdrlen, unsigned long *datalen)
{
  FILE *fp;
  char magic[4];
  long headerLen = 0;
  long len;
  short waveFormat;
  short waveChannels;
  long waveRate;
  short waveBits;
  struct stat sbuf;

#ifdef __CYGWIN__
  if ((fp = fopen(filename, "rb")) == NULL)
#else
  if ((fp = fopen(filename, "r")) == NULL)
#endif
  {
    qDebug( "Cannot open audio file \"%s\" for reading: %s",
	    filename, strerror(errno));
    return 1;
  }

  if (offset != 0) {
    if (fseek(fp, offset, SEEK_SET) != 0) {
      qDebug( "Cannot seek to offset %ld in file \"%s\": %s",
	      offset, filename, strerror(errno));
      return 1;
    }
  }

  if (fread(magic, sizeof(char), 4, fp) != 4 ||
      strncmp("RIFF", magic, 4) != 0) {
    qDebug( "%s: not a WAVE file.", filename);
    fclose(fp);
    return 2;
  }

  readLong(fp);

  if (fread(magic, sizeof(char), 4, fp) != 4 ||
      strncmp("WAVE", magic, 4) != 0) {
    qDebug( "%s: not a WAVE file.", filename);
    fclose(fp);
    return 2;
  }

  // search for format chunk
  for (;;) {
    if (fread(magic, sizeof(char), 4, fp) != 4) {
      qDebug( "%s: corrupted WAVE file.", filename);
      fclose(fp);
      return 1;
    }

    len = readLong(fp);
    len += len & 1; // round to multiple of 2

    if (strncmp("fmt ", magic, 4) == 0) {
      // format chunk found
      break;
    }

    // skip chunk data
    if (fseek(fp, len, SEEK_CUR) != 0) {
      qDebug( "%s: corrupted WAVE file.", filename);
      fclose(fp);
      return 1;
    }
  }

  if (len < 16) {
    qDebug( "%s: corrupted WAVE file.", filename);
    fclose(fp);
    return 1;
  }

  waveFormat = readShort(fp);

  if (waveFormat != 1) {
    // not PCM format
    qDebug( "%s: not in PCM format.", filename);
    fclose(fp);
    return 2;
  }

  waveChannels = readShort(fp);
  if (waveChannels != 2) {
    qDebug( "%s: found %d channel(s), require 2 channels.",
	    filename, waveChannels);
    fclose(fp);
    return 2;
  }

  waveRate = readLong(fp);
  if (waveRate != 44100) {
     qDebug( "%s: found sampling rate %ld, require 44100.",
	    filename, waveRate);
     fclose(fp);
     return 2;
  }

  readLong(fp); // average bytes/second
  readShort(fp); // block align

  waveBits = readShort(fp);
  if (waveBits != 16) {
    qDebug( "%s: found %d bits per sample, require 16.",
	    filename, waveBits);
    fclose(fp);
    return 2;
  }

  len -= 16;

  // skip chunk data
  if (fseek(fp, len, SEEK_CUR) != 0) {
    qDebug( "%s: corrupted WAVE file.", filename);
    fclose(fp);
    return 1;
  }

  // search wave data chunk
  for (;;) {
    if (fread(magic, sizeof(char), 4, fp) != 4) {
      qDebug( "%s: corrupted WAVE file.", filename);
      fclose(fp);
      return 1;
    }

    len = readLong(fp);

    if (strncmp("data", magic, 4) == 0) {
      // found data chunk
      break;
    }

    len += len & 1; // round to multiple of 2

    // skip chunk data
    if (fseek(fp, len, SEEK_CUR) != 0) {
      qDebug( "%s: corrupted WAVE file.", filename);
      fclose(fp);
      return 1;
    }
  }

  if ((headerLen = ftell(fp)) < 0) {
    qDebug( "%s: cannot determine file position: %s",
	    filename, strerror(errno));
    fclose(fp);
    return 1;
  }

  headerLen -= offset;

  if (fstat(fileno(fp), &sbuf) != 0) {
    qDebug( "Cannot fstat audio file \"%s\": %s", filename,
	    strerror(errno));
    fclose(fp);
    return 1;
  }

  fclose(fp);

  if (len + headerLen + offset > sbuf.st_size) {
    qDebug(	"%s: file length does not match length from WAVE header - using actual length.", filename);
    len = sbuf.st_size - offset - headerLen;
  }

  if (len % sizeof(Sample) != 0) {
    qDebug(
	    "%s: length of data chunk is not a multiple of sample size (4).",
	    filename);
  }

  *hdrlen = headerLen;

  if (datalen != NULL)
    *datalen = len / sizeof(Sample);

  return 0;
}
