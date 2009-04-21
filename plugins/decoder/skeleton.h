/* 
 *
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_<name>_DECODER_H_
#define _K3B_<name>_DECODER_H_

#include "k3baudiodecoder.h"


class K3b<name>DecoderFactory : public K3b::AudioDecoderFactory
{
    Q_OBJECT

public:
    K3b<name>DecoderFactory( QObject* parent, const QVariantList& );
    ~K3b<name>DecoderFactory();

    bool canDecode( const KUrl& filename );

    int pluginSystemVersion() const { return K3B_PLUGIN_SYSTEM_VERSION; }

    K3b::AudioDecoder* createDecoder( QObject* parent = 0 ) const;
};


class K3b<name>Decoder : public K3b::AudioDecoder
{
    Q_OBJECT

public:
    K3b<name>Decoder( QObject* parent = 0,  );
    ~K3b<name>Decoder();

    QString fileType() const;

protected:
    bool analyseFileInternal( K3b::Msf& frames, int& samplerate, int& ch );
    bool initDecoderInternal();
    bool seekInternal( const K3b::Msf& );

    int decodeInternal( char* _data, int maxLen );
};

K3B_EXPORT_PLUGIN(k3b<name>decoder, K3b<name>DecoderFactory)

#endif
