/*


    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#include <config-k3b.h>

#include "k3b<name>decoder.h"

K3B_EXPORT_PLUGIN(k3b<name>decoder, K3b<name>DecoderFactory)

K3b<name>DecoderFactory::K3b<name>DecoderFactory( QObject* parent, const QVariantList& )
    : K3b::AudioDecoderFactory( parent )
{
}


K3b<name>DecoderFactory::~K3b<name>DecoderFactory()
{
}


K3b::AudioDecoder* K3b<name>DecoderFactory::createDecoder( QObject* parent,
    ) const
{
    return new K3b<name>Decoder( parent, name );
}


bool K3b<name>DecoderFactory::canDecode( const QUrl& url )
{
    // PUT YOUR CODE HERE
    return false;
}






K3b<name>Decoder::K3b<name>Decoder( QObject* parent,  )
    : K3b::AudioDecoder( parent, name )
{
}


K3b<name>Decoder::~K3b<name>Decoder()
{
}


QString K3b<name>Decoder::fileType() const
{
    // PUT YOUR CODE HERE
}


bool K3b<name>Decoder::analyseFileInternal( K3b::Msf& frames, int& samplerate, int& ch )
{
    // PUT YOUR CODE HERE
    // call addTechnicalInfo and addMetaInfo here
    return false;
}


bool K3b<name>Decoder::initDecoderInternal()
{
    // PUT YOUR CODE HERE
    return false;
}


bool K3b<name>Decoder::seekInternal( const K3b::Msf& )
{
    // PUT YOUR CODE HERE
    return false;
}


int K3b<name>Decoder::decodeInternal( char* _data, int maxLen )
{
    // PUT YOUR CODE HERE
    return -1;
}


#include "k3b<name>decoder.moc"
