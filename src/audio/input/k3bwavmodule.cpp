#include "k3bwavmodule.h"
#include "../k3baudiotrack.h"

#include <kapp.h>
#include <kconfig.h>
#include <kurl.h>
#include <kprocess.h>

#include <qtimer.h>
#include <qfile.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <cmath>

#include "../../device/util.h"
#include "../../device/Sample.h"



K3bWavModule::K3bWavModule( K3bAudioTrack* track )
  : K3bExternalBinModule( track )
{
  m_streamingTimer = new QTimer( this );
}


K3bWavModule::~K3bWavModule()
{
}



void K3bWavModule::slotGatherInformation()
{
  audioTrack()->setBufferFile( audioTrack()->absPath() );

  // try to determine the track length
  unsigned long dataLength;
  long buffy;
  if( waveLength( QFile::encodeName(audioTrack()->absPath()),
		  0,
		  &buffy,
		  &dataLength ) == 0 )
    {
      qDebug( "(K3bWavModule) Could determine length of track: %ld samples, %f frames", 
	      dataLength, (float)dataLength / 588.0 );
      qDebug( "(K3bWavModule) Setting track's length to %f frames", ceil( (double)dataLength / 588.0 ) );

      audioTrack()->setLength( (int)ceil( (double)dataLength / 588.0 ) );
    }
  else
    qDebug( "(K3bWavModule) Could not determine length of track!" );
}


KURL K3bWavModule::writeToWav( const KURL& url )
{
  // we need an asynronous finished signal
  m_streamingTimer->disconnect();
  connect( m_streamingTimer, SIGNAL(timeout()), this, SLOT(slotWavFinished()) );
  m_streamingTimer->start(0, true);

  return url;
}


void K3bWavModule::slotWavFinished()
{
  emit finished( true );
}


void K3bWavModule::addArguments()
{
  kapp->config()->setGroup( "External Programs");
  
  // convert the file to big endian with sox
  *m_process << kapp->config()->readEntry( "sox path", "/usr/bin/sox" );

  *m_process << "-V";
  // input
  *m_process << QString("\"%1\"").arg(QFile::encodeName( audioTrack()->absPath() ));

  // output options
  *m_process << "-t" << "raw";    // filetype raw 
  *m_process << "-r" << "44100";  // samplerate
  *m_process << "-c" << "2";      // channels
  *m_process << "-s";             // signed linear data
  *m_process << "-w";             // 16-bit words
  *m_process << "-x";             // swap byte order
  *m_process << "-";              // output to stdout
}


void K3bWavModule::slotParseStdErrOutput(KProcess*, char* output, int len)
{
  qDebug( QString::fromLatin1( output, len ) );
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


#include "k3bwavmodule.moc"
