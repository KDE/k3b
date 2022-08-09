/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "k3bffmpegwrapper.h"
#include "k3bplugin_i18n.h"

#include <config-k3b.h>

extern "C" {
/*
 Recent versions of FFmpeg uses C99 constant macros which are not present in C++ standard.
 The macro __STDC_CONSTANT_MACROS allow C++ to use these macros. Although it's not defined by C++ standard
 it's supported by many implementations.
 See bug 236036 and discussion: https://lists.ffmpeg.org/pipermail/ffmpeg-devel/2010-May/095488.html
 */
#define __STDC_CONSTANT_MACROS
#ifdef NEWFFMPEGAVCODECPATH
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#else
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>
#endif
}

#include <string.h>
#include <math.h>


#define FFMPEG_CODEC(s) (s->codec)

#ifndef HAVE_FFMPEG_AVFORMAT_OPEN_INPUT
//      this works because the parameters/options are not used
#  define avformat_open_input(c,s,f,o) av_open_input_file(c,s,f,0,o)
#endif
#ifndef HAVE_FFMPEG_AV_DUMP_FORMAT
#  define av_dump_format(c,x,f,y) dump_format(c,x,f,y)
#endif
#ifndef HAVE_FFMPEG_AVFORMAT_FIND_STREAM_INFO
#  define avformat_find_stream_info(c,o) av_find_stream_info(c)
#endif
#ifndef HAVE_FFMPEG_AVFORMAT_CLOSE_INPUT
#  define avformat_close_input(c) av_close_input_file(*c)
#endif
#ifndef HAVE_FFMPEG_AVCODEC_OPEN2
#  define avcodec_open2(a,c,o) avcodec_open(a,c)
#endif
#ifndef HAVE_FFMPEG_AVMEDIA_TYPE
#  define AVMEDIA_TYPE_AUDIO CODEC_TYPE_AUDIO
#endif
#ifndef HAVE_FFMPEG_CODEC_MP3
#  define CODEC_ID_MP3 CODEC_ID_MP3LAME
#endif

K3bFFMpegWrapper* K3bFFMpegWrapper::s_instance = 0;


class K3bFFMpegFile::Private
{
public:
    ::AVFormatContext* formatContext;
    ::AVCodec* codec;
    ::AVStream *audio_stream;

    K3b::Msf length;

    // for decoding. ffmpeg requires 16-byte alignment.
    ::AVFrame* frame;
    char* outputBufferPos;
    int outputBufferSize;
    ::AVPacket packet;
    quint8* packetData;
    int packetSize;
    bool isSpacious;
    int sampleFormat;
};


K3bFFMpegFile::K3bFFMpegFile( const QString& filename )
    : m_filename(filename)
{
    d = new Private;
    d->formatContext = 0;
    d->codec = 0;
    d->audio_stream = nullptr;
    d->frame = av_frame_alloc();
}


K3bFFMpegFile::~K3bFFMpegFile()
{
    close();
    av_frame_free(&d->frame);
    delete d;
}


bool K3bFFMpegFile::open()
{
    close();

    // open the file
    int err = ::avformat_open_input( &d->formatContext, m_filename.toLocal8Bit(), 0, 0 );
    if( err < 0 ) {
        qDebug() << "(K3bFFMpegFile) unable to open " << m_filename << " with error " << err;
        return false;
    }

    // analyze the streams
    ::avformat_find_stream_info( d->formatContext, 0 );

    // we only handle files containing one audio stream
    if( d->formatContext->nb_streams == 1 ) {
        d->audio_stream = d->formatContext->streams[0];
    } else  {
        for (uint i = 0; i < d->formatContext->nb_streams; ++i) {
            if (d->formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                if (!d->audio_stream) {
                    d->audio_stream = d->formatContext->streams[i];
                } else {
                    d->audio_stream = nullptr;
                    qDebug() << "(K3bFFMpegFile) more than one audio stream in " << m_filename;
                    return false;
                }
            }
        }
    }

    // urgh... ugly
    ::AVCodecContext* codecContext =  FFMPEG_CODEC(d->audio_stream);
    if( codecContext->codec_type != AVMEDIA_TYPE_AUDIO)
    {
        qDebug() << "(K3bFFMpegFile) not a simple audio stream: " << m_filename;
        return false;
    }

    // get the codec
    d->codec = ::avcodec_find_decoder(codecContext->codec_id);
    if( !d->codec ) {
        qDebug() << "(K3bFFMpegFile) no codec found for " << m_filename;
        return false;
    }

    // open the codec on our context
    qDebug() << "(K3bFFMpegFile) found codec for " << m_filename;
    if( ::avcodec_open2( codecContext, d->codec, 0 ) < 0 ) {
        qDebug() << "(K3bFFMpegDecoderFactory) could not open codec.";
        return false;
    }

    // determine the length of the stream
    d->length = K3b::Msf::fromSeconds( (double)d->formatContext->duration / (double)AV_TIME_BASE );

    if( d->length == 0 ) {
        qDebug() << "(K3bFFMpegDecoderFactory) invalid length.";
        return false;
    }

    d->sampleFormat = d->audio_stream->codecpar->format;
    d->isSpacious = ::av_sample_fmt_is_planar((AVSampleFormat)d->sampleFormat) && d->audio_stream->codecpar->channels > 1;

    // dump some debugging info
    ::av_dump_format( d->formatContext, 0, m_filename.toLocal8Bit(), 0 );

    return true;
}


void K3bFFMpegFile::close()
{
    d->outputBufferSize = 0;
    d->packetSize = 0;
    d->packetData = 0;

    if( d->codec ) {
        ::avcodec_close( FFMPEG_CODEC(d->audio_stream) );
        d->codec = 0;
    }

    if( d->formatContext ) {
        ::avformat_close_input( &d->formatContext );
        d->formatContext = 0;
    }

    d->audio_stream = nullptr;
}


K3b::Msf K3bFFMpegFile::length() const
{
    return d->length;
}


int K3bFFMpegFile::sampleRate() const
{
    return d->audio_stream->codecpar->sample_rate;
}


int K3bFFMpegFile::channels() const
{
    return d->audio_stream->codecpar->channels;
}


int K3bFFMpegFile::type() const
{
    return d->audio_stream->codecpar->codec_id;
}


QString K3bFFMpegFile::typeComment() const
{
    switch( type() ) {
    case AV_CODEC_ID_WMAV1:
        return i18n("Windows Media v1");
    case AV_CODEC_ID_WMAV2:
        return i18n("Windows Media v2");
    case AV_CODEC_ID_WAVPACK:
        return i18n("WavPack");
    case AV_CODEC_ID_APE:
        return i18n("Monkey's Audio (APE)");
    case AV_CODEC_ID_AAC:
        return i18n("Advanced Audio Coding (AAC)");
    default:
        return QString::fromLocal8Bit( d->codec->name );
    }
}


QString K3bFFMpegFile::title() const
{
    // FIXME: is this UTF8 or something??
    AVDictionaryEntry *ade = av_dict_get( d->formatContext->metadata, "TITLE", NULL, 0 );
    return ade && ade->value && ade->value[0] != '\0' ? QString::fromLocal8Bit( ade->value ) : QString();
}


QString K3bFFMpegFile::author() const
{
    // FIXME: is this UTF8 or something??
    AVDictionaryEntry *ade = av_dict_get( d->formatContext->metadata, "ARTIST", NULL, 0 );
    return ade && ade->value && ade->value[0] != '\0' ? QString::fromLocal8Bit( ade->value ) : QString();
}


QString K3bFFMpegFile::comment() const
{
    // FIXME: is this UTF8 or something??
    AVDictionaryEntry *ade = av_dict_get( d->formatContext->metadata, "COMMENT", NULL, 0 );
    return ade && ade->value && ade->value[0] != '\0' ? QString::fromLocal8Bit( ade->value ) : QString();
}


int K3bFFMpegFile::read(char* buf, int bufLen)
{
    if (!buf || !d->outputBufferPos)
        return -1;

    int ret = fillOutputBuffer();
    if (ret <= 0) {
        return ret;
    }

    int len = qMin(bufLen, d->outputBufferSize);
    ::memcpy(buf, d->outputBufferPos, len);

    if(d->isSpacious && bufLen > d->outputBufferSize)
        delete[] d->outputBufferPos; // clean up allocated space

    // TODO: only swap if needed
    for(int i=0; i<len-1; i+=2)
        qSwap(buf[i], buf[i+1]); // BE -> LE

    d->outputBufferSize -= len;
    if(d->outputBufferSize > 0)
        d->outputBufferPos += len;
    return len;
}


// fill d->packetData with data to decode
int K3bFFMpegFile::readPacket()
{
    if( d->packetSize <= 0 ) {
        ::av_init_packet( &d->packet );

        if( ::av_read_frame( d->formatContext, &d->packet ) < 0 ) {
            return 0;
        }
        d->packetSize = d->packet.size;
        d->packetData = d->packet.data;
    }

    return d->packetSize;
}


// decode data in d->packetData and fill d->outputBuffer
int K3bFFMpegFile::fillOutputBuffer()
{
    // decode if the output buffer is empty
    while(d->outputBufferSize <= 0) {

        // make sure we have data to decode
        if( readPacket() == 0 ) {
            return 0;
        }

        int gotFrame = 0;
        int len = ::avcodec_decode_audio4(
            FFMPEG_CODEC(d->audio_stream),
            d->frame,
            &gotFrame,
            &d->packet );

        if( d->packetSize <= 0 || len < 0 )
            ::av_packet_unref( &d->packet );
        if( len < 0 ) {
            qDebug() << "(K3bFFMpegFile) decoding failed for " << m_filename;
            return -1;
        }

        if (gotFrame) {
            int nb_s = d->frame->nb_samples;
            int nb_ch = 2; // copy only two channels even if there're more
            d->outputBufferSize = nb_s * nb_ch * 2; // 2 means 2 bytes (16bit)
            d->outputBufferPos = reinterpret_cast<char*>(
                d->frame->extended_data[0]);
            if(d->isSpacious) {
                d->outputBufferPos = new char[d->outputBufferSize];
                if(d->sampleFormat == AV_SAMPLE_FMT_FLTP) {
                    int width = sizeof(float); // sample width of float audio
                    for(int sample=0; sample<nb_s; sample++) {
                        for(int ch=0; ch<nb_ch; ch++) {
                            float val = *(reinterpret_cast<float*>(
                                d->frame->extended_data[ch] + sample * width));
                            val = ::abs(val) > 1 ? ::copysign(1.0, val) : val;
                            int16_t result = static_cast<int16_t>(
                                val * 32767.0 + 32768.5) - 32768;
                            ::memcpy(d->outputBufferPos + (sample*nb_ch+ch) * 2,
                                     &result,
                                     2); // 2 is sample width of 16 bit audio
                        }
                    }
                } else {
                    for(int sample=0; sample<nb_s; sample++) {
                        for(int ch=0; ch<nb_ch; ch++) {
                            ::memcpy(d->outputBufferPos + (sample*nb_ch+ch) * 2,
                                     d->frame->extended_data[ch] + sample * 2,
                                     2); // 16 bit here as well
                        }
                    }
                }
            }
        }
        d->packetSize -= len;
        d->packetData += len;
    }

    return d->outputBufferSize;
}


bool K3bFFMpegFile::seek( const K3b::Msf& msf )
{
    d->outputBufferSize = 0;
    d->packetSize = 0;

    double seconds = (double)msf.totalFrames()/75.0;
    quint64 timestamp = (quint64)(seconds * (double)AV_TIME_BASE);

    // FIXME: do we really need the start_time and why?
    return ( ::av_seek_frame( d->formatContext, -1, timestamp + d->formatContext->start_time, 0 ) >= 0 );
}






K3bFFMpegWrapper::K3bFFMpegWrapper()
{
    ::av_register_all();
}


K3bFFMpegWrapper::~K3bFFMpegWrapper()
{
    s_instance = 0;
}


K3bFFMpegWrapper* K3bFFMpegWrapper::instance()
{
    if( !s_instance ) {
        s_instance = new K3bFFMpegWrapper();
    }

    return s_instance;
}


K3bFFMpegFile* K3bFFMpegWrapper::open( const QString& filename ) const
{
    K3bFFMpegFile* file = new K3bFFMpegFile( filename );
    if( file->open() ) {
#ifndef K3B_FFMPEG_ALL_CODECS
        //
        // only allow tested formats. ffmpeg seems not to be too reliable with every format.
        // mp3 being one of them sadly. Most importantly: allow the libsndfile decoder to do
        // its thing.
        //
        if( file->type() == AV_CODEC_ID_WMAV1 ||
            file->type() == AV_CODEC_ID_WMAV2 ||
            file->type() == AV_CODEC_ID_AAC ||
            file->type() == AV_CODEC_ID_APE ||
            file->type() == AV_CODEC_ID_WAVPACK )
#endif
            return file;
    }

    delete file;
    return 0;
}
