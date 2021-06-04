/* 
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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

    bool canDecode( const QUrl& filename );

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

#endif
