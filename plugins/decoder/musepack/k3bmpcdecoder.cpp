/*

    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bmpcdecoder.h"
#include "k3bmpcwrapper.h"
#include "k3bplugin_i18n.h"

#include <config-k3b.h>

K_PLUGIN_CLASS_WITH_JSON(K3bMpcDecoderFactory, "k3bmpcdecoder.json")

K3bMpcDecoderFactory::K3bMpcDecoderFactory( QObject* parent, const QVariantList& )
    : K3b::AudioDecoderFactory( parent )
{
}


K3bMpcDecoderFactory::~K3bMpcDecoderFactory()
{
}


K3b::AudioDecoder* K3bMpcDecoderFactory::createDecoder( QObject* parent
    ) const
{
    return new K3bMpcDecoder( parent );
}


bool K3bMpcDecoderFactory::canDecode( const QUrl& url )
{
    K3bMpcWrapper w;
    return w.open( url.toLocalFile() );
}






K3bMpcDecoder::K3bMpcDecoder( QObject* parent  )
    : K3b::AudioDecoder( parent )
{
    m_mpc = new K3bMpcWrapper();
}


K3bMpcDecoder::~K3bMpcDecoder()
{
    delete m_mpc;
}


QString K3bMpcDecoder::fileType() const
{
    return i18n("Musepack");
}


bool K3bMpcDecoder::analyseFileInternal( K3b::Msf& frames, int& samplerate, int& ch )
{
    if( m_mpc->open( filename() ) ) {
        frames = m_mpc->length();
        samplerate = m_mpc->samplerate();
        ch = m_mpc->channels();

        // call addTechnicalInfo and addMetaInfo here

        return true;
    }
    else
        return false;
}


bool K3bMpcDecoder::initDecoderInternal()
{
    return m_mpc->open( filename() );
}


bool K3bMpcDecoder::seekInternal( const K3b::Msf& msf )
{
    return m_mpc->seek( msf );
}


int K3bMpcDecoder::decodeInternal( char* _data, int maxLen )
{
    return m_mpc->decode( _data, maxLen );
}


#include "k3bmpcdecoder.moc"
