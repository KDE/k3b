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

#include <kprocess.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cmath>
#include <iostream>




K3bMp3Module::K3bMp3Module( K3bAudioTrack* track )
  : K3bExternalBinModule( track )
{
  m_decodingProcess = new KShellProcess();
}


K3bMp3Module::~K3bMp3Module()
{
  delete m_decodingProcess;
}


void K3bMp3Module::addArguments()
{
  kapp->config()->setGroup( "External Programs");
  
  // start a new mpg123 process for this track
  *m_process << kapp->config()->readEntry( "mpg123 path" );

  // needed for percent output
  // *m_process << "-v";

  // switch on buffer
  // TODO: let the user specify the buffer-size
  *m_process << "-b" << "10000";  // 2 Mb

  *m_process << "-s";
  *m_process << QString("\"%1\"").arg(QFile::encodeName( audioTrack()->absPath() ));

  *m_process << "|";
  
  // convert the stream to big endian with sox
  *m_process << kapp->config()->readEntry( "sox path", "/usr/bin/sox" );
  // input options
  *m_process << "-t" << "raw";    // filetype raw
  *m_process << "-r" << "44100";  // samplerate
  *m_process << "-c" << "2";      // channels
  *m_process << "-s";             // signed linear data
  *m_process << "-w";             // 16-bit words
  *m_process << "-";              // input from stdin
  
  // output options
  *m_process << "-t" << "raw";    // filetype raw 
  *m_process << "-r" << "44100";  // samplerate
  *m_process << "-c" << "2";      // channels
  *m_process << "-s";             // signed linear data
  *m_process << "-w";             // 16-bit words
  *m_process << "-x";             // swap byte order
  *m_process << "-";              // output to stdout
}


KURL K3bMp3Module::writeToWav( const KURL& url )
{
  if( m_decodingProcess->isRunning() ) {
    m_decodingProcess->kill();
  }

  m_decodingProcess->clearArguments();
  m_decodingProcess->disconnect();
  connect( m_decodingProcess, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotWriteToWavFinished()) );
  connect( m_decodingProcess, SIGNAL(receivedStderr(KProcess*, char*, int)), 
	   this, SLOT(slotParseStdErrOutput(KProcess*, char*, int)) );
  // TODO: we need some error shit: parse some error messages like "disc full"

  kapp->config()->setGroup( "External Programs");
  
  // start a new mpg123 process for this track
  m_decodingProcess->clearArguments();
  *m_decodingProcess << kapp->config()->readEntry( "mpg123 path" );

  *m_decodingProcess << "-v" << "-w";
  *m_decodingProcess << url.path();
  *m_decodingProcess << QString("\"%1\"").arg(QFile::encodeName( audioTrack()->absPath() ));

  if( !m_decodingProcess->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
    qDebug( "(K3bMp3Module) could not start mpg123 process." );
    return KURL();
  }
  else {
    m_currentToWavUrl = url;
  }
  
  return url;
}


void K3bMp3Module::slotWriteToWavFinished()
{
  m_currentToWavUrl = QString::null;
  if( m_decodingProcess->normalExit() && m_decodingProcess->exitStatus() == 0 )
    emit finished( true );
  else
    emit finished( false );
}


void K3bMp3Module::init()
{
  ID3_Tag _tag( audioTrack()->absPath().latin1() );
  ID3_Frame* _frame = _tag.Find( ID3FID_TITLE );
  if( _frame )
    audioTrack()->setTitle( QString(ID3_GetString(_frame, ID3FN_TEXT )) );
		
  _frame = _tag.Find( ID3FID_LEADARTIST );
  if( _frame )
    audioTrack()->setArtist( QString(ID3_GetString(_frame, ID3FN_TEXT )) );
}


void K3bMp3Module::slotGatherInformation()
{
  ID3_Tag _tag( audioTrack()->absPath().latin1() );
  int _id3TagSize = _tag.Size();


  // find frame-header
  unsigned char data[1024];
  unsigned char* datapointer;


  FILE *fd = fopen( QFile::encodeName(audioTrack()->absPath()),"r");
  if( fd == NULL )
    qDebug( "(K3bMp3Module) could not open file " + audioTrack()->absPath() );
  else
    {
      fseek(fd, 0, SEEK_SET);
      int readbytes = fread(&data, 1, 1024, fd);
      fclose(fd);
      
      // some hacking
      if( data[0] == 'I' && data[1] == 'D' && data[2] == '3' ) {
	fd = fopen( QFile::encodeName(audioTrack()->absPath()),"r");
	fseek(fd, _id3TagSize, SEEK_SET);
	readbytes = fread(&data, 1, 1024, fd);
	fclose(fd);
      }

      bool found = false;
      
      unsigned int _header = data[0]<<24 | data[1]<<16 | data[2]<<8 | data[3];
      datapointer = data+4;
      readbytes -= 4;
      
      while( !found && (readbytes > 0) ) 
	{
	  if( mp3HeaderCheck(_header) ) {
	    //	    qDebug( "*header found: %x", _header );
	    found = true;
	    break;
	  }
	  
	  _header = _header<<8 | *datapointer;
	  datapointer++;
	  readbytes--;
	}
      if( found ) 
	{
	  // calculate length
	  if( mp3SampleRate(_header) ) {
	    
	    double tpf;
	    
	    tpf = (double)1152;
	    tpf /= mp3SampleRate(_header) << 1;
	    
	    double _frameSize = 144000.0 * (double)mp3Bitrate(_header);
	    _frameSize /= (double)( mp3SampleRate(_header) - mp3Padding(_header) );
	    
	    if( mp3Protection( _header ) )
	      _frameSize += 16;

	    //	    qDebug( "*framesize calculated: %f", _frameSize);
	    if( _frameSize > 0 ) {
	      int _frameNumber = (int)( (double)(QFileInfo(audioTrack()->absPath()).size() - _id3TagSize ) / _frameSize );
	      //	      qDebug( " #frames: %i", _frameNumber );

	      // cdrdao needs the length in frames where 75 frames are 1 second 
	      double _length = _frameNumber * compute_tpf( _header );
	      //	  setLength(  _frameNumber * 26 * 75 / 1000 );
	      audioTrack()->setLength( _length * 75 );
	    }
	  }
// 	  else
// 	    qDebug("Samplerate is 0");
	}
      else
	{
	  qDebug( "(K3bMp3Module) Warning: No mp3-frame-header found!!!" );
	}
    }

}

void K3bMp3Module::recalcLength()
{
  if( m_decodingProcess->isRunning() ) {
    m_decodingProcess->kill();
  }

  // better start a timer and not call it direct because of the signals
  QTimer::singleShot( 0, this, SLOT(slotStartCountRawData()) );
}


void K3bMp3Module::slotStartCountRawData()
{
  // ===================
  // here we start a mpg123 process to decode the complete file and count it's output
  // in the background. This is neccessary since we need the perfect size of the input
  // for burning on-the-fly!
  // ===================

  m_rawData = 0;

  qDebug( "(K3bMp3Module) start counting raw data for track: " + audioTrack()->fileName() );
      
  m_decodingProcess->clearArguments();
  m_decodingProcess->disconnect();
  kapp->config()->setGroup( "External Programs" );
      
  *m_decodingProcess << kapp->config()->readEntry( "mpg123 path" );
      
  // switch on buffer
  // TODO: let the user specify the buffer-size
  *m_decodingProcess << "-b" << "2048";  // 2 Mb
      
  *m_decodingProcess << "-s";
  *m_decodingProcess << QString("\"%1\"").arg(QFile::encodeName( audioTrack()->absPath() ));

  //       cerr << endl;
  //       for( char* it = m_decodingProcess->args()->first(); it != 0; 
  // 	   it = m_decodingProcess->args()->next() )
  // 	cerr << it << " ";
  //       cerr << endl;
      
  connect( m_decodingProcess, SIGNAL(receivedStdout(KProcess*, char*, int)), 
	   this, SLOT(slotCountRawData(KProcess*, char*, int)) );
  //       connect( m_decodingProcess, SIGNAL(receivedStderr(KProcess*, char*, int)), 
  // 	    this, SLOT(slotParseStdErrOutput(KProcess*, char*, int)) );
  connect( m_decodingProcess, SIGNAL(processExited(KProcess*)), this, SLOT(slotCountRawDataFinished()) );
      
  if( !m_decodingProcess->start( KProcess::NotifyOnExit, KProcess::Stdout ) ) {
    qDebug( "(K3bMp3Module) could not start mpg123 process." );

    emit finished( false );
  }
}


void K3bMp3Module::slotCountRawData(KProcess*, char*, int len)
{
  m_rawData += len;
}


void K3bMp3Module::slotCountRawDataFinished()
{
  m_decodingProcess->disconnect();
  m_decodingProcess->clearArguments();

  if( m_decodingProcess->normalExit() && m_decodingProcess->exitStatus() == 0 ) {
    audioTrack()->setLength( (int)(ceil((float)m_rawData / 2352.0) ) /* here we need bytesPerFrame */ );
    
    qDebug( "(K3bMp3Module) bytes in raw data: %ld,  frames: %f, ceil: %d", 
	    m_rawData, (float)m_rawData / 2352.0, (int)(ceil((float)m_rawData / 2352.0) ) );
    
    emit finished( true );
  }
  else {
    emit finished( false );
  }
}



void K3bMp3Module::slotParseStdErrOutput(KProcess*, char* output, int len)
{
  QString buffer = QString::fromLatin1( output, len );

  // split to lines
  QStringList lines = QStringList::split( "\n", buffer );
	
  for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ )
    {
      if( (*str).left(10).contains("Frame#") ) {
	int made, togo;
	bool ok;
	int pos1 = (*str).find('#');
	int pos2 = (*str).find('[');
	int pos3 = (*str).find(']');

	if( pos1 == -1 || pos2 == -1 || pos3 == -1 ) {
	  qDebug("parsing did not work for " + *str );
	  return;
	}

	made = (*str).mid(pos1+1,pos2-pos1-1).toInt(&ok);
	if( !ok ) {
	  qDebug("parsing did not work for " + (*str).mid(pos1+1,pos2-pos1-1) );
	  return;
	}
	togo = (*str).mid(pos2+1,pos3-pos2-1).toInt(&ok);
	if( !ok ) {
	    qDebug("parsing did not work for " + (*str).mid(pos2+1,pos3-pos2-1) );
	    return;
	}

	emit percent( 100 * made / (made+togo) );
      }
      else
	qDebug( *str );
    }
}


void K3bMp3Module::cancel()
{
  // cancel whatever in process
  m_decodingProcess->disconnect();
  m_decodingProcess->kill();

  if( !m_currentToWavUrl.isEmpty() ) {
    if( QFile::exists( m_currentToWavUrl.path() ) ) {
      QFile::remove( m_currentToWavUrl.path() );
    }
    m_currentToWavUrl = QString::null;
    
  K3bExternalBinModule::cancel();
  }
}


int K3bMp3Module::mp3VersionNumber(unsigned int header) 
{
  int d = (header & 0x00180000) >> 19;

  switch (d) 
    {
    case 3:
      return 0;
    case 2:
      return 1;
    case 0:	
      return 2;
    }

  return -1;
}



int K3bMp3Module::mp3LayerNumber(unsigned int header) 
{
  int d = (header & 0x00060000) >> 17;
  return 4-d;
}

bool K3bMp3Module::mp3Protection(unsigned int header) 
{
  return ((header>>16 & 0x00000001)==1);
}

int K3bMp3Module::mp3Bitrate(unsigned int header) 
{
  const unsigned int bitrates[3][3][15] =
    {
      {
	{0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},
     	{0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},
	{0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}
      },
      {
     	{0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
	{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
	{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
      },
      {
     	{0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
	{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
	{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
      }
    };
  int version = mp3VersionNumber(header);
  int layer = mp3LayerNumber(header)-1;
  int bitrate = (header & 0x0000f000) >> 12;

//   if( bitrate == 0 )
//     qDebug(" Bitrate is 0");

//   printf(" Version:%x\n",version);
//   printf(" Layer:%d\n",layer);
//   printf(" Bitrate:%d\n",bitrates[version][layer][bitrate]);

  return bitrates[version][layer][bitrate];
}

int K3bMp3Module::mp3SampleRate(unsigned int header)
{
  const unsigned int s_freq[3][4] =
    {
      {44100, 48000, 32000, 0},
      {22050, 24000, 16000, 0},
      {11025, 8000, 8000, 0}
    };

  int version = mp3VersionNumber(header);
  int srate = (header & 0x00000c00) >> 10;

  //  qDebug(" samplerate: %i", s_freq[version][srate] );

  return s_freq[version][srate];
}


bool K3bMp3Module::mp3HeaderCheck(unsigned int head)
{
  if( (head & 0xffe00000) != 0xffe00000)
    return false;

  if(!((head>>17)&3))
    return false;

  if( ((head>>12)&0xf) == 0xf)
    return false;

  if( ((head>>10)&0x3) == 0x3 )
    return false;

  if ((head & 0xffff0000) == 0xfffe0000)
    return false;

  return true;
}


int K3bMp3Module::mp3Padding(unsigned int header)
{
  if( header & 0x00000200 == 0x00000200 ) {
    // calculate padding
    //    qDebug( " padding: 1" );
    return 1;
  }
  else {
    //    qDebug( " padding: 0" );
    return 0;
  }
}


double K3bMp3Module::compute_tpf( unsigned int header )
{
  static int bs[4] = { 0,384,1152,1152 };
  double tpf;
  
  tpf = (double) bs[mp3LayerNumber(header)];
  tpf /= mp3SampleRate(header);// << (fr->lsf);
  return tpf;
}


#include "k3bmp3module.moc"
