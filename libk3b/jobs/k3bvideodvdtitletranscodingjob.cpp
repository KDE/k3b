/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bvideodvdtitletranscodingjob.h"

#include <k3bexternalbinmanager.h>
#include <k3bprocess.h>
#include <k3bcore.h>
#include <k3bglobals.h>

#include <klocale.h>
#include <kdebug.h>
#include <kstandarddirs.h>

#include <qfile.h>
#include <qfileinfo.h>


class K3bVideoDVDTitleTranscodingJob::Private
{
public:
  const K3bExternalBin* usedTranscodeBin;

  K3bProcess* process;

  QString twoPassEncodingLogFile;

  int currentEncodingPass;

  bool canceled;

  int lastProgress;
  int lastSubProgress;
};



K3bVideoDVDTitleTranscodingJob::K3bVideoDVDTitleTranscodingJob( K3bJobHandler* hdl, QObject* parent )
  : K3bJob( hdl, parent ),
    m_clippingTop( 0 ),
    m_clippingBottom( 0 ),
    m_clippingLeft( 0 ),
    m_clippingRight( 0 ),
    m_width( 0 ),
    m_height( 0 ),
    m_titleNumber( 1 ),
    m_audioStreamIndex( 0 ),
    m_videoCodec( VIDEO_CODEC_FFMPEG_MPEG4 ),
    m_audioCodec( AUDIO_CODEC_MP3 ),
    m_videoBitrate( 1800 ),
    m_audioBitrate( 128 ),
    m_audioVBR( false ),
    m_resampleAudio( false ),
    m_twoPassEncoding( false ),
    m_lowPriority( true )
{
  d = new Private;
  d->process = 0;
}


K3bVideoDVDTitleTranscodingJob::~K3bVideoDVDTitleTranscodingJob()
{
  delete d->process;
  delete d;
}


void K3bVideoDVDTitleTranscodingJob::start()
{
  jobStarted();

  d->canceled = false;
  d->lastProgress = 0;

  d->usedTranscodeBin = k3bcore->externalBinManager()->binObject("transcode");
  if( !d->usedTranscodeBin ) {
    emit infoMessage( i18n("%1 executable could not be found.").arg("transcode"), ERROR );
    jobFinished( false );
    return;
  }

  if( d->usedTranscodeBin->version < K3bVersion( 1, 0, 0 ) ){
    emit infoMessage( i18n("%1 version %2 is too old.")
		      .arg("transcode")
		      .arg(d->usedTranscodeBin->version), ERROR );
    jobFinished( false );
    return;
  }

  emit debuggingOutput( "Used versions", "transcode: " + d->usedTranscodeBin->version );

  if( !d->usedTranscodeBin->copyright.isEmpty() )
    emit infoMessage( i18n("Using %1 %2 - Copyright (C) %3")
		      .arg(d->usedTranscodeBin->name())
		      .arg(d->usedTranscodeBin->version)
		      .arg(d->usedTranscodeBin->copyright), INFO );

  //
  // Let's take a look at the filename
  //
  if( m_filename.isEmpty() ) {
    m_filename = K3b::findTempFile( "avi" );
  }
  else {
    // let's see if the directory exists and we can write to it
    QFileInfo fileInfo( m_filename );
    QFileInfo dirInfo( fileInfo.dirPath() );
    if( !dirInfo.exists() && !KStandardDirs::makeDir( dirInfo.absFilePath() ) ) {
      emit infoMessage( i18n("Unable to create folder '%1'").arg(dirInfo.filePath()), ERROR );
      return;
    }
    else if( !dirInfo.isDir() || !dirInfo.isWritable() ) {
      emit infoMessage( i18n("Invalid filename: '%1'").arg(m_filename), ERROR );
      jobFinished( false );
      return;
    }
  }

  //
  // Determine a log file for two-pass encoding
  //
  d->twoPassEncodingLogFile = K3b::findTempFile( "log" );

  emit newTask( i18n("Transcoding title %1 from Video DVD %2").arg(m_titleNumber).arg(m_dvd.volumeIdentifier()) );

  //
  // Ok then, let's begin
  //
  startTranscode( m_twoPassEncoding ? 1 : 0 );
}


void K3bVideoDVDTitleTranscodingJob::startTranscode( int pass )
{
  d->currentEncodingPass = pass;
  d->lastSubProgress = 0;

  QString videoCodecString;
  switch( m_videoCodec ) {
  case VIDEO_CODEC_XVID:
    videoCodecString = "xvid";
    break;

  case VIDEO_CODEC_FFMPEG_MPEG4:
    videoCodecString = "ffmpeg";
    break;

  default:
    emit infoMessage( i18n("Invalid Video codec set: %1").arg(m_videoCodec), ERROR );
    jobFinished( false );
    return;
  }

  QString audioCodecString;
  switch( m_audioCodec ) {
  case AUDIO_CODEC_MP3:
    audioCodecString = "0x55";
    break;

    // ogg only works (as in: transcode does something) with .y <codec>,ogg
    // but then the video is garbage (at least to xine and mplayer on my system)
    //     case AUDIO_CODEC_OGG_VORBIS:
    //       audioCodecString = "0xfffe";
    //       break;

  case AUDIO_CODEC_AC3_STEREO:
  case AUDIO_CODEC_AC3_PASSTHROUGH:
    audioCodecString = "0x2000";
    break;

  default:
    emit infoMessage( i18n("Invalid Audio codec set: %1").arg(m_audioCodec), ERROR );
    jobFinished( false );
    return;
  }

  //
  // prepare the process
  //
  delete d->process;
  d->process = new K3bProcess();
  d->process->setSuppressEmptyLines(true);
  d->process->setSplitStdout(true);
  connect( d->process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotTranscodeStderr(const QString&)) );
  connect( d->process, SIGNAL(stdoutLine(const QString&)), this, SLOT(slotTranscodeStderr(const QString&)) );
  connect( d->process, SIGNAL(processExited(KProcess*)), this, SLOT(slotTranscodeExited(KProcess*)) );

  // the executable
  *d->process << d->usedTranscodeBin;

  // low priority
  if( m_lowPriority )
    *d->process << "--nice" << "19";

  // we only need 100 steps, but to make sure we use 150
  if ( d->usedTranscodeBin->version.simplify() >= K3bVersion( 1, 1, 0 ) )
      *d->process << "--progress_meter" << "2" << "--progress_rate" << QString::number(m_dvd[m_titleNumber-1].playbackTime().totalFrames()/150);
  else
      *d->process << "--print_status" << QString::number(m_dvd[m_titleNumber-1].playbackTime().totalFrames()/150);

  // the input
  *d->process << "-i" << m_dvd.device()->blockDeviceName();

  // just to make sure
  *d->process << "-x" << "dvd";

  // select the title number
  *d->process << "-T" << QString("%1,-1,1").arg( m_titleNumber );

  // select the audio stream to extract
  if ( m_dvd[m_titleNumber-1].numAudioStreams() > 0 )
      *d->process << "-a" << QString::number( m_audioStreamIndex );

  // clipping
  *d->process << "-j" << QString("%1,%2,%3,%4")
                         .arg(m_clippingTop)
                         .arg(m_clippingLeft)
                         .arg(m_clippingBottom)
                         .arg(m_clippingRight);

  // select the encoding type (single pass or two-pass) and the log file for two-pass encoding
  // the latter is unused for pass = 0
  *d->process << "-R" << QString("%1,%2").arg( pass ).arg( d->twoPassEncodingLogFile );

  // depending on the pass we use different options
  if( pass != 1 ) {
    // select video codec
    *d->process << "-y" << videoCodecString;

    // select the audio codec to use
    *d->process << "-N" << audioCodecString;

    if( m_audioCodec == AUDIO_CODEC_AC3_PASSTHROUGH ) {
      // keep 5.1 sound
      *d->process << "-A";
    }
    else {
      // audio quality settings
      *d->process << "-b" << QString("%1,%2").arg(m_audioBitrate).arg(m_audioVBR ? 1 : 0);

      // resample audio stream to 44.1 khz
      if( m_resampleAudio )
	*d->process << "-E" << "44100";
    }

    // the output filename
    *d->process << "-o" << m_filename;
  }
  else {
    // gather information about the video stream, ignore audio
    *d->process << "-y" << QString("%1,null").arg( videoCodecString );

    // we ignore the output from the first pass
    *d->process << "-o" << "/dev/null";
  }

  // choose the ffmpeg codec
  if( m_videoCodec == VIDEO_CODEC_FFMPEG_MPEG4 ) {
    *d->process << "-F" << "mpeg4";
  }

  // video bitrate
  *d->process << "-w" << QString::number( m_videoBitrate );

  // video resizing
  int usedWidth = m_width;
  int usedHeight = m_height;
  if( m_width == 0 || m_height == 0 ) {
    //
    // The "real" size of the video, considering anamorph encoding
    //
    int realHeight = m_dvd[m_titleNumber-1].videoStream().realPictureHeight();
    int readWidth = m_dvd[m_titleNumber-1].videoStream().realPictureWidth();

    //
    // The clipped size with the correct aspect ratio
    //
    int clippedHeight = realHeight - m_clippingTop - m_clippingBottom;
    int clippedWidth = readWidth - m_clippingLeft - m_clippingRight;

    //
    // Now simply resize the clipped video to the wanted size
    //
    if( usedWidth > 0 ) {
      usedHeight = clippedHeight * usedWidth / clippedWidth;
    }
    else {
      if( usedHeight == 0 ) {
	//
	// This is the default case in which both m_width and m_height are 0.
	// The result will be a size of clippedWidth x clippedHeight
	//
	usedHeight = clippedHeight;
      }
      usedWidth = clippedWidth * usedHeight / clippedHeight;
    }
  }

  //
  // Now make sure both width and height are multiple of 16 the simple way
  //
  usedWidth -= usedWidth%16;
  usedHeight -= usedHeight%16;

  // we only give information about the resizing of the video once
  if( pass < 2 )
    emit infoMessage( i18n("Resizing picture of title %1 to %2x%3").arg(m_titleNumber).arg(usedWidth).arg(usedHeight), INFO );
  *d->process << "-Z" << QString("%1x%2").arg(usedWidth).arg(usedHeight);

  // additional user parameters from config
  const QStringList& params = d->usedTranscodeBin->userParameters();
  for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
    *d->process << *it;

  // produce some debugging output
  kdDebug() << "***** transcode parameters:\n";
  const QValueList<QCString>& args = d->process->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    s += *it + " ";
  }
  kdDebug() << s << flush << endl;
  emit debuggingOutput( d->usedTranscodeBin->name() + " command:", s);

  // start the process
  if( !d->process->start( KProcess::NotifyOnExit, KProcess::All ) ) {
    // something went wrong when starting the program
    // it "should" be the executable
    emit infoMessage( i18n("Could not start %1.").arg(d->usedTranscodeBin->name()), K3bJob::ERROR );
    jobFinished(false);
  }
  else {
    if( pass == 0 )
      emit newSubTask( i18n("Single-pass Encoding") );
    else if( pass == 1 )
      emit newSubTask( i18n("Two-pass Encoding: First Pass") );
    else
      emit newSubTask( i18n("Two-pass Encoding: Second Pass") );

    emit subPercent( 0 );
  }
}


void K3bVideoDVDTitleTranscodingJob::cancel()
{
  // FIXME: do not cancel before one frame has been encoded. transcode seems to hang then
  //        find a way to determine all subprocess ids to kill all of them
  d->canceled = true;
  if( d->process && d->process->isRunning() )
    d->process->kill();
}


void K3bVideoDVDTitleTranscodingJob::cleanup( bool success )
{
  if( QFile::exists( d->twoPassEncodingLogFile ) ) {
    QFile::remove( d->twoPassEncodingLogFile );
  }

  if( !success && QFile::exists( m_filename ) ) {
    emit infoMessage( i18n("Removing incomplete video file '%1'").arg(m_filename), INFO );
    QFile::remove( m_filename );
  }
}


void K3bVideoDVDTitleTranscodingJob::slotTranscodeStderr( const QString& line )
{
  emit debuggingOutput( "transcode", line );

  // parse progress
  // encoding frames [000000-000144],  27.58 fps, EMT: 0:00:05, ( 0| 0| 0)
  if( line.startsWith( "encoding frame" ) ) {
    int pos1 = line.find( '-', 15 );
    int pos2 = line.find( ']', pos1+1 );
    if( pos1 > 0 && pos2 > 0 ) {
      bool ok;
      int encodedFrames = line.mid( pos1+1, pos2-pos1-1 ).toInt( &ok );
      if( ok ) {
	int progress = 100 * encodedFrames / m_dvd[m_titleNumber-1].playbackTime().totalFrames();

	if( progress > d->lastSubProgress ) {
	  d->lastSubProgress = progress;
	  emit subPercent( progress );
	}

	if( m_twoPassEncoding ) {
	  progress /= 2;
	  if( d->currentEncodingPass == 2 )
	    progress += 50;
	}

	if( progress > d->lastProgress ) {
	  d->lastProgress = progress;
	  emit percent( progress );
	}
      }
    }
  }
}


void K3bVideoDVDTitleTranscodingJob::slotTranscodeExited( KProcess* p )
{
  if( d->canceled ) {
    emit canceled();
    cleanup( false );
    jobFinished( false );
  }
  else if( p->normalExit() ) {
    switch( p->exitStatus() ) {
    case 0:
      if( d->currentEncodingPass == 1 ) {
	emit percent( 50 );
	// start second encoding pass
	startTranscode( 2 );
      }
      else {
	emit percent( 100 );
	cleanup( true );
	jobFinished( true );
      }
      break;

    default:
      // FIXME: error handling

      emit infoMessage( i18n("%1 returned an unknown error (code %2).")
			.arg(d->usedTranscodeBin->name()).arg(p->exitStatus()),
			K3bJob::ERROR );
      emit infoMessage( i18n("Please send me an email with the last output."), K3bJob::ERROR );

      cleanup( false );
      jobFinished( false );
    }
  }
  else {
    cleanup( false );
    emit infoMessage( i18n("Execution of %1 failed.").arg("transcode"), ERROR );
    emit infoMessage( i18n("Please consult the debugging output for details."), ERROR );
    jobFinished( false );
  }
}


void K3bVideoDVDTitleTranscodingJob::setClipping( int top, int left, int bottom, int right )
{
  m_clippingTop = top;
  m_clippingLeft = left;
  m_clippingBottom = bottom;
  m_clippingRight = right;

  //
  // transcode seems unable to handle different clipping values for left and right
  //
  m_clippingLeft = m_clippingRight = QMIN( m_clippingRight, m_clippingLeft );
}


void K3bVideoDVDTitleTranscodingJob::setSize( int width, int height )
{
  m_width = width;
  m_height = height;
}


QString K3bVideoDVDTitleTranscodingJob::audioCodecString( K3bVideoDVDTitleTranscodingJob::AudioCodec codec )
{
  switch( codec ) {
  case AUDIO_CODEC_AC3_STEREO:
    return i18n("AC3 (Stereo)");
  case AUDIO_CODEC_AC3_PASSTHROUGH:
    return i18n("AC3 (Pass-through)");
  case AUDIO_CODEC_MP3:
    return i18n("MPEG1 Layer III");
  default:
    return "unknown audio codec";
  }
}


QString K3bVideoDVDTitleTranscodingJob::videoCodecString( K3bVideoDVDTitleTranscodingJob::VideoCodec codec )
{
  switch( codec ) {
  case VIDEO_CODEC_FFMPEG_MPEG4:
    return i18n("MPEG4 (FFMPEG)");
  case VIDEO_CODEC_XVID:
    return i18n("XviD");
  default:
    return "unknown video codec";
  }
}


QString K3bVideoDVDTitleTranscodingJob::videoCodecDescription( K3bVideoDVDTitleTranscodingJob::VideoCodec codec )
{
  switch( codec ) {
  case VIDEO_CODEC_FFMPEG_MPEG4:
    return i18n("FFmpeg is an open-source project trying to support most video and audio codecs used "
		"these days. Its subproject libavcodec forms the basis for multimedia players such as "
		"xine or mplayer.")
      + "<br>"
      + i18n("FFmpeg contains an implementation of the MPEG-4 video encoding standard which produces "
	     "high quality results.");
  case VIDEO_CODEC_XVID:
    return i18n("XviD is a free and open source MPEG-4 video codec. XviD was created by a group of "
		"volunteer programmers after the OpenDivX source was closed in July 2001.")
      + "<br>"
      + i18n("XviD features MPEG-4 Advanced Profile settings such as b-frames, global "
	     "and quarter pixel motion compensation, lumi masking, trellis quantization, and "
	     "H.263, MPEG and custom quantization matrices.")
      + "<br>"
      + i18n("XviD is a primary competitor of DivX (XviD being DivX spelled backwards). "
	     "While DivX is closed source and may only run on Windows, Mac OS and Linux, "
	     "XviD is open source and can potentially run on any platform.")
      + "<br><em>"
      + i18n("(Description taken from the Wikipedia article)")
      + "</em>";
  default:
    return "unknown video codec";
  }
}


QString K3bVideoDVDTitleTranscodingJob::audioCodecDescription( K3bVideoDVDTitleTranscodingJob::AudioCodec codec )
{
  static QString s_ac3General = i18n("AC3, better known as Dolby Digital is standardized as ATSC A/52. "
				     "It contains up to 6 total channels of sound.");
  switch( codec ) {
  case AUDIO_CODEC_AC3_STEREO:
    return s_ac3General
      + "<br>" + i18n("With this setting K3b will create a two-channel stereo "
		      "Dolby Digital audio stream.");
  case AUDIO_CODEC_AC3_PASSTHROUGH:
    return s_ac3General
      + "<br>" + i18n("With this setting K3b will use the Dolby Digital audio stream "
		      "from the source DVD without changing it.")
      + "<br>" + i18n("Use this setting to preserve 5.1 channel sound from the DVD.");
  case AUDIO_CODEC_MP3:
    return i18n("MPEG1 Layer III is better known as MP3 and is the most used lossy audio format.")
      + "<br>" + i18n("With this setting K3b will create a two-channel stereo MPEG1 Layer III audio stream.");
  default:
    return "unknown audio codec";
  }
}


bool K3bVideoDVDTitleTranscodingJob::transcodeBinaryHasSupportFor( K3bVideoDVDTitleTranscodingJob::VideoCodec codec, const K3bExternalBin* bin )
{
  static char* s_codecFeatures[] = { "xvid", "ffmpeg" };
  if( !bin )
    bin = k3bcore->externalBinManager()->binObject("transcode");
  if( !bin )
    return false;
  return bin->hasFeature( QString::fromLatin1( s_codecFeatures[(int)codec] ) );
}


bool K3bVideoDVDTitleTranscodingJob::transcodeBinaryHasSupportFor( K3bVideoDVDTitleTranscodingJob::AudioCodec codec, const K3bExternalBin* bin )
{
  static char* s_codecFeatures[] = { "lame", "ac3", "ac3" };
  if( !bin )
    bin = k3bcore->externalBinManager()->binObject("transcode");
  if( !bin )
    return false;
  return bin->hasFeature( QString::fromLatin1( s_codecFeatures[(int)codec] ) );
}

#include "k3bvideodvdtitletranscodingjob.moc"
