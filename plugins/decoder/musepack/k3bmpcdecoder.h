/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_MPC_DECODER_H_
#define _K3B_MPC_DECODER_H_

#include "k3baudiodecoder.h"

class K3bMpcWrapper;


class K3bMpcDecoderFactory : public K3b::AudioDecoderFactory
{
    Q_OBJECT

public:
    K3bMpcDecoderFactory( QObject* parent, const QVariantList& );
    ~K3bMpcDecoderFactory() override;

    bool canDecode( const QUrl& filename ) override;

    int pluginSystemVersion() const override { return K3B_PLUGIN_SYSTEM_VERSION; }

    K3b::AudioDecoder* createDecoder( QObject* parent = 0 ) const override;
};


class K3bMpcDecoder : public K3b::AudioDecoder
{
    Q_OBJECT

public:
    explicit K3bMpcDecoder( QObject* parent = 0 );
    ~K3bMpcDecoder() override;

    QString fileType() const override;

protected:
    bool analyseFileInternal( K3b::Msf& frames, int& samplerate, int& ch ) override;
    bool initDecoderInternal() override;
    bool seekInternal( const K3b::Msf& ) override;

    int decodeInternal( char* _data, int maxLen ) override;

private:
    K3bMpcWrapper* m_mpc;
};

#endif
