/*

    SPDX-FileCopyrightText: 2004 Matthieu Bedouet <mbedouet@no-log.org>
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "k3blibsndfiledecoder.h"
#include "k3bplugin_i18n.h"

#include <config-k3b.h>

#include <QDebug>
#include <QFile>
#include <QStringList>

#include <math.h>
#include <stdio.h>
#include <sndfile.h>

K_PLUGIN_CLASS_WITH_JSON(K3bLibsndfileDecoderFactory, "k3blibsndfiledecoder.json")

class K3bLibsndfileDecoder::Private
{
public:
    Private():
        isOpen(false),
        buffer(0),
        bufferSize(0) {
        format_info.name = 0;
    }

    SNDFILE *sndfile;
    SF_INFO sndinfo;
    SF_FORMAT_INFO format_info;
    bool isOpen;
    float* buffer;
    int bufferSize;
};



K3bLibsndfileDecoder::K3bLibsndfileDecoder( QObject* parent )
    : K3b::AudioDecoder( parent )
{
    d = new Private();
}


K3bLibsndfileDecoder::~K3bLibsndfileDecoder()
{
    delete d;
}


QString K3bLibsndfileDecoder::fileType() const
{
    if( d->format_info.name )
        return QString::fromLocal8Bit(d->format_info.name);
    else
        return "-";
}



bool K3bLibsndfileDecoder::openFile()
{
    if( !d->isOpen ) {

        cleanup();

        d->sndinfo.format = 0;
        d->sndfile = sf_open (QFile::encodeName(filename()), SFM_READ, &d->sndinfo);
        if ( !d->sndfile ) {
            qDebug() << "(K3bLibsndfileDecoder::openLibsndfileFile) : " << sf_strerror(d->sndfile);
            return false;
        }
        else {
            //retrieve infos (name, extension) about the format:
            d->format_info.format = d->sndinfo.format & SF_FORMAT_TYPEMASK ;
            sf_command (d->sndfile, SFC_GET_FORMAT_INFO, &d->format_info, sizeof (SF_FORMAT_INFO)) ;

            d->isOpen = true;
            qDebug() << "(K3bLibsndfileDecoder::openLibsndfileFile) " << d->format_info.name << " file opened ";
            return true;
        }
    }

    return d->isOpen;
}


bool K3bLibsndfileDecoder::analyseFileInternal( K3b::Msf& frames, int& samplerate, int& ch )
{
    cleanup();

    if( openFile() ) {
        // check length of track
        if ( d->sndinfo.frames <= 0 ) {
            qDebug() << "(K3bLibsndfileDecoder::analyseFileInternal) Could not determine length of file "
                     << filename() << Qt::endl;
            cleanup();
            return false;
        }
        else {
            addMetaInfo( META_TITLE, sf_get_string(d->sndfile, SF_STR_TITLE) );
            addMetaInfo( META_ARTIST, sf_get_string(d->sndfile, SF_STR_ARTIST) );
            addMetaInfo( META_COMMENT, sf_get_string(d->sndfile, SF_STR_COMMENT) );

            addTechnicalInfo( i18n("Channels"), QString::number(d->sndinfo.channels) );
            addTechnicalInfo( i18n("Sampling Rate"), i18n("%1 Hz",d->sndinfo.samplerate) );

            frames = static_cast<unsigned long>(ceil(75.0 * d->sndinfo.frames / d->sndinfo.samplerate));
            samplerate = d->sndinfo.samplerate;
            ch = d->sndinfo.channels;

            qDebug() << "(K3bLibsndfileDecoder) successfully analysed file: " << frames << " frames.";

            cleanup();
            return true;
        }
    }
    else
        return false;
}



bool K3bLibsndfileDecoder::initDecoderInternal()
{
    cleanup();
    return openFile();
}



int K3bLibsndfileDecoder::decodeInternal( char* data, int maxLen )
{
    if( !d->buffer ) {
        d->buffer = new float[maxLen];
        d->bufferSize = maxLen/2;
    }

    int read = int(sf_read_float(d->sndfile, d->buffer,d->bufferSize));
    fromFloatTo16BitBeSigned( d->buffer, data, read );
    read = read * 2;

    if( read < 0 ) {
        qDebug() << "(K3bLibsndfileDecoder::decodeInternal) Error: " << read;
        return -1;
    }
    else if( read == 0 ) {
        qDebug() << "(K3bLibsndfileDecoder::decodeInternal) successfully finished decoding.";
        return 0;
    }
    else
        return read;
}



bool K3bLibsndfileDecoder::seekInternal( const K3b::Msf& pos)
{
    return ( sf_seek( d->sndfile, pos.pcmSamples(), SEEK_SET ) == 0 );
}




void K3bLibsndfileDecoder::cleanup()
{
    if( d->isOpen ) {
        qDebug() << "(K3bLibsndfileDecoder) cleaning up.";
        sf_close( d->sndfile );
        d->isOpen = false;
    }
}



/********************************************************/


K3bLibsndfileDecoderFactory::K3bLibsndfileDecoderFactory( QObject* parent, const QVariantList&  )
    : K3b::AudioDecoderFactory( parent)
{
}


K3bLibsndfileDecoderFactory::~K3bLibsndfileDecoderFactory()
{
}


K3b::AudioDecoder* K3bLibsndfileDecoderFactory::createDecoder( QObject* parent ) const
{
    return new K3bLibsndfileDecoder( parent );
}


bool K3bLibsndfileDecoderFactory::canDecode( const QUrl& url )
{
    SF_INFO infos;
    infos.format = 0;
    SNDFILE* sndfile = sf_open (QFile::encodeName(url.toLocalFile()), SFM_READ, &infos);

    //is it supported by libsndfile?
    if ( !sndfile ) {
        qDebug() << "(K3bLibsndfileDecoder) " << sf_strerror(sndfile);
        return false;
    }
    else if ( infos.format )  {

        //retrieve infos (name) about the format:
        SF_FORMAT_INFO format_info;
        format_info.format = infos.format & SF_FORMAT_TYPEMASK ;
        sf_command (sndfile, SFC_GET_FORMAT_INFO, &format_info, sizeof (format_info)) ;

        qDebug() << "(K3bLibsndfileDecoder) " << format_info.name << " file === OK === ";
        sf_close( sndfile );
        return true;
    }
    else {
        qDebug() << "(K3bLibsndfileDecoder) " << url.toLocalFile() << "not supported";
        sf_close( sndfile );
        return false;
    }
    return false;
}

#include "k3blibsndfiledecoder.moc"

#include "moc_k3blibsndfiledecoder.cpp"
