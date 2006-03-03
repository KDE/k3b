/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_VIDEODVD_TITLE_TRANSCODING_JOB_H_
#define _K3B_VIDEODVD_TITLE_TRANSCODING_JOB_H_

#include <k3bjob.h>
#include <k3bvideodvd.h>

class KProcess;


/**
 * The K3bVideoDVDTitleTranscodingJob rips a Video DVD title directly
 * from the medium and transcodes it on-the-fly to, for example, an XviD video
 *
 * For now only one audio stream is supported.
 */
class K3bVideoDVDTitleTranscodingJob : public K3bJob
{
  Q_OBJECT

 public:
  K3bVideoDVDTitleTranscodingJob( K3bJobHandler* hdl, QObject* parent );
  ~K3bVideoDVDTitleTranscodingJob();

  const K3bVideoDVD::VideoDVD& videoDVD() const { return m_dvd; }
  int title() const { return m_titleNumber; }
  int audioStream() const { return m_audioStreamIndex; }
  int clippingTop() const { return m_clippingTop; }
  int clippingLeft() const { return m_clippingLeft; }
  int clippingBottom() const { return m_clippingBottom; }
  int clippingRight() const { return m_clippingRight; }
  int height() const { return m_height; }
  int width() const { return m_width; }
  const QString& filename() { return m_filename; }
  int videoCodec() const { return m_videoCodec; }
  int videoBitrate() const { return m_videoBitrate; }
  bool twoPassEncoding() const { return m_twoPassEncoding; }
  int audioCodec() const { return m_audioCodec; }
  int audioBitrate() const { return m_audioBitrate; }
  bool audioVBR() const { return m_audioVBR; }
  bool resampleAudioTo44100() const { return m_resampleAudio; }
  bool lowPriority() const { return m_lowPriority; }

 public slots:
  void start();
  void cancel();

  /**
   * The device containing the Video DVD
   */
  void setVideoDVD( const K3bVideoDVD::VideoDVD& dvd ) { m_dvd = dvd; }

  /**
   * Set the title number to be transcoded
   *
   * The default value is 1, denoting the first title.
   */
  void setTitle( int t ) { m_titleNumber = t; }

  /**
   * Set the audio stream to use.
   *
   * For now K3b does not support encoding multiple audio streams
   * in one video file.
   *
   * The default value is 0, meaning that the first audio stream will
   * be encoded.
   */
  void setAudioStream( int i ) { m_audioStreamIndex = i; }

  /**
   * Set the clipping values for the Video title.
   * The clipping will be applied before the transcoding.
   *
   * The default is to not clip the video.
   */
  void setClipping( int top, int left, int bottom, int right );

  /**
   * The size of the resulting transcoded video.
   *
   * The default is to not resize the video at all (width=height=0)
   */
  void setSize( int width, int height );

  /**
   * The filename to write the resulting video to.
   *
   * The default is some automatically generated filename
   * in the default K3b temp directory.
   */
  void setFilename( const QString& name ) { m_filename = name; }

  /**
   * The video codecs supported by this job.
   */
  enum VideoCodec {
    VIDEO_CODEC_XVID,
    VIDEO_CODEC_FFMPEG_MPEG4
  };

  /**
   * Set the video codec used to encode the video title.
   *
   * The default is VIDEO_CODEC_FFMPEG_MPEG4
   */
  void setVideoCodec( int codec ) { m_videoCodec = codec; }

  /**
   * Set the bitrate used to encode the video.
   *
   * The default is 1800
   */
  void setVideoBitrate( int bitrate ) { m_videoBitrate = bitrate; }

  /**
   * Set if the job should use two-pass encoding to improve
   * the quality of the resulting video.
   *
   * The default is false.
   */
  void setTwoPassEncoding( bool b ) { m_twoPassEncoding = b; }

  /**
   * The audio codecs supported by this job.
   */
  enum AudioCodec {
    AUDIO_CODEC_MP3,
    /*    AUDIO_CODEC_OGG_VORBIS,*/
    AUDIO_CODEC_AC3
  };

  /**
   * Set the audio codec used to encode the audio stream
   * in the video title.
   *
   * The default is AUDIO_CODEC_MP3
   */
  void setAudioCodec( int codec ) { m_audioCodec = codec; }

  /**
   * Set the bitrate used to encode the audio stream.
   *
   * The default is 128
   */
  void setAudioBitrate( int bitrate ) { m_audioBitrate = bitrate; }

  /**
   * Set if the audio stream should be encoded with a variable bitrate.
   *
   * The default is false.
   */
  void setAudioVBR( bool vbr ) { m_audioVBR = vbr; }

  /**
   * Set if the audio data should be resampled to 44100 Hz/s
   *
   * The default is true.
   */
  void setResampleAudioTo44100( bool b ) { m_resampleAudio = b; }

  /**
   * If true the transcode processes will be run with a very low scheduling 
   * priority.
   *
   * The default is true.
   */
  void setLowPriority( bool b ) { m_lowPriority = b; }

  static QString audioCodecString( int );
  static QString videoCodecString( int );

 private slots:
  void slotTranscodeStderr( const QString& );
  void slotTranscodeExited( KProcess* );

 private:
  /**
   * \param 0 - single pass encoding
   *        1 - two pass encoding/first pass
   *        2 - two pass encoding/second pass
   */
  void startTranscode( int pass );

  void cleanup( bool success );

  K3bVideoDVD::VideoDVD m_dvd;

  QString m_filename;

  int m_clippingTop;
  int m_clippingBottom;
  int m_clippingLeft;
  int m_clippingRight;

  int m_width;
  int m_height;

  int m_titleNumber;
  int m_audioStreamIndex;  

  int m_videoCodec;
  int m_audioCodec;

  int m_videoBitrate;
  int m_audioBitrate;
  bool m_audioVBR;

  bool m_resampleAudio;
  bool m_twoPassEncoding;

  bool m_lowPriority;

  class Private;
  Private* d;
};

#endif
