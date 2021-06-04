/*
    SPDX-FileCopyrightText: 2005-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3baudiofilereader.h"
#include "k3baudiofile.h"
#include "k3baudiodecoder.h"


namespace K3b {

class AudioFileReader::Private
{
public:
    Private( AudioFile& s )
    :
        source( s )
    {
    }

    AudioFile& source;
};


AudioFileReader::AudioFileReader( AudioFile& source, QObject* parent )
    : QIODevice( parent ),
      d( new Private( source ) )
{
}


AudioFileReader::~AudioFileReader()
{
    close();
}


bool AudioFileReader::open( OpenMode mode )
{
    if( !mode.testFlag( QIODevice::WriteOnly ) ) {
        return QIODevice::open( mode );
    }
    else {
        return false;
    }
}


void AudioFileReader::close()
{
    QIODevice::close();
}


bool AudioFileReader::isSequential() const
{
    return false;
}


qint64 AudioFileReader::size() const
{
    return d->source.length().audioBytes();
}


bool AudioFileReader::seek( qint64 pos )
{
    Msf msf = Msf::fromAudioBytes( pos );
    // this is valid once the decoder has been initialized.
    if( d->source.startOffset() + msf <= d->source.lastSector() &&
        d->source.decoder()->seek( d->source.startOffset() + msf ) ) {
        return QIODevice::seek( pos );
    }
    else {
        return false;
    }
}


qint64 AudioFileReader::writeData( const char* /*data*/, qint64 /*len*/ )
{
    return -1;
}


qint64 AudioFileReader::readData( char* data, qint64 maxlen )
{
    // here we can trust on the decoder to always provide enough data
    // see if we decode too much
    if( maxlen + pos() > size() )
        maxlen = size() - pos();

    qint64 read = d->source.decoder()->decode( data, maxlen );

    if( read > 0 )
        return read;
    else
        return -1;
}

} // namespace K3b
