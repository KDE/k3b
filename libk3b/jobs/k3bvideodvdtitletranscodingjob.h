/*
 *
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

#ifndef _K3B_VIDEODVD_TITLE_TRANSCODING_JOB_H_
#define _K3B_VIDEODVD_TITLE_TRANSCODING_JOB_H_

#include "k3b_export.h"
#include "k3bjob.h"
#include "k3bvideodvd.h"
#include <QProcess>

namespace K3b {
    class ExternalBin;


    /**
     * The VideoDVDTitleTranscodingJob rips a Video DVD title directly
     * from the medium and transcodes it on-the-fly to, for example, an XviD video
     *
     * For now only one audio stream is supported.
     */
    class LIBK3B_EXPORT VideoDVDTitleTranscodingJob : public Job
    {
        Q_OBJECT

    public:
        VideoDVDTitleTranscodingJob( JobHandler* hdl, QObject* parent );
        ~VideoDVDTitleTranscodingJob() override;

        /**
         * The video codecs supported by this job.
         */
        enum VideoCodec {
            VIDEO_CODEC_XVID,
            VIDEO_CODEC_FFMPEG_MPEG4,
            VIDEO_CODEC_NUM_ENTRIES /**< Do not use this as a codec. */
        };

        /**
         * The audio codecs supported by this job.
         */
        enum AudioCodec {
            AUDIO_CODEC_MP3,
            /*    AUDIO_CODEC_OGG_VORBIS,*/
            AUDIO_CODEC_AC3_STEREO,
            AUDIO_CODEC_AC3_PASSTHROUGH,
            AUDIO_CODEC_NUM_ENTRIES /**< Do not use this as a codec. */
        };

        const VideoDVD::VideoDVD& videoDVD() const { return m_dvd; }
        int title() const { return m_titleNumber; }
        int audioStream() const { return m_audioStreamIndex; }
        int clippingTop() const { return m_clippingTop; }
        int clippingLeft() const { return m_clippingLeft; }
        int clippingBottom() const { return m_clippingBottom; }
        int clippingRight() const { return m_clippingRight; }
        int height() const { return m_height; }
        int width() const { return m_width; }
        const QString& filename() { return m_filename; }
        VideoCodec videoCodec() const { return m_videoCodec; }
        int videoBitrate() const { return m_videoBitrate; }
        bool twoPassEncoding() const { return m_twoPassEncoding; }
        AudioCodec audioCodec() const { return m_audioCodec; }
        int audioBitrate() const { return m_audioBitrate; }
        bool audioVBR() const { return m_audioVBR; }
        bool resampleAudioTo44100() const { return m_resampleAudio; }
        bool lowPriority() const { return m_lowPriority; }

        /**
         * \param bin If 0 the default binary from Core will be used
         */
        static bool transcodeBinaryHasSupportFor( VideoCodec codec, const ExternalBin* bin = 0 );

        /**
         * \param bin If 0 the default binary from Core will be used
         */
        static bool transcodeBinaryHasSupportFor( AudioCodec codec, const ExternalBin* bin = 0 );

        static QString videoCodecString( VideoCodec );
        static QString audioCodecString( AudioCodec );

        static QString videoCodecDescription( VideoCodec );
        static QString audioCodecDescription( AudioCodec );

    public Q_SLOTS:
        void start() override;
        void cancel() override;

        /**
         * The device containing the Video DVD
         */
        void setVideoDVD( const K3b::VideoDVD::VideoDVD& dvd ) { m_dvd = dvd; }

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
         * For now it is not possible to use different clipping values for left
         * and right as transcode cannot handle this. Thus, the job uses the
         * smaller value for both the left and right clipping.
         *
         * The default is to not clip the video.
         */
        void setClipping( int top, int left, int bottom, int right );

        /**
         * The size of the resulting transcoded video.
         *
         * The default is to automatically adjust the size (width=height=0), which
         * essentially means that anamorph encoded source material will be resized
         * according to its aspect ratio.
         *
         * It is also possible to set just the width or just the height and leave
         * the other value to 0 which will then be determined automatically.
         *
         * The clipping values will be taken into account if at least one value
         * is determined automatically.
         *
         * The width and height values have to be a multiple of 16. If it is not,
         * they will be changed accordingly.
         *
         * FIXME: GET MessageInfoRMATION: why a multiple of 16 and not 8 or 32?
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
         * Set the video codec used to encode the video title.
         *
         * The default is VIDEO_CODEC_FFMPEG_MPEG4
         */
        void setVideoCodec( VideoCodec codec ) { m_videoCodec = codec; }

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
         * Set the audio codec used to encode the audio stream
         * in the video title.
         *
         * The default is AUDIO_CODEC_MP3
         */
        void setAudioCodec( AudioCodec codec ) { m_audioCodec = codec; }

        /**
         * Set the bitrate used to encode the audio stream.
         *
         * The default is 128
         *
         * In case of the AC3 codec the bitrate can be some value between 32 and 640.
         *
         * For the AC3 passthrough mode the bitrate is ignored.
         */
        void setAudioBitrate( int bitrate ) { m_audioBitrate = bitrate; }

        /**
         * Set if the audio stream should be encoded with a variable bitrate.
         *
         * The default is false.
         *
         * For the AC3 passthrough mode the bitrate is ignored.
         */
        void setAudioVBR( bool vbr ) { m_audioVBR = vbr; }

        /**
         * Set if the audio data should be resampled to 44100 Hz/s
         *
         * The default is false.
         *
         * For the AC3 passthrough mode this is ignored.
         */
        void setResampleAudioTo44100( bool b ) { m_resampleAudio = b; }

        /**
         * If true the transcode processes will be run with a very low scheduling
         * priority.
         *
         * The default is true.
         */
        void setLowPriority( bool b ) { m_lowPriority = b; }

    private Q_SLOTS:
        void slotTranscodeStderr( const QString& );
        void slotTranscodeExited( int, QProcess::ExitStatus );

    private:
        /**
         * \param 0 - single pass encoding
         *        1 - two pass encoding/first pass
         *        2 - two pass encoding/second pass
         */
        void startTranscode( int pass );

        void cleanup( bool success );

        VideoDVD::VideoDVD m_dvd;

        QString m_filename;

        int m_clippingTop;
        int m_clippingBottom;
        int m_clippingLeft;
        int m_clippingRight;

        int m_width;
        int m_height;

        int m_titleNumber;
        int m_audioStreamIndex;

        VideoCodec m_videoCodec;
        AudioCodec m_audioCodec;

        int m_videoBitrate;
        int m_audioBitrate;
        bool m_audioVBR;

        bool m_resampleAudio;
        bool m_twoPassEncoding;

        bool m_lowPriority;

        class Private;
        Private* d;
    };
}

#endif
