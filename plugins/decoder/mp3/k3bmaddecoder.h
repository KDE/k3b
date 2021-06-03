/* 
    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_MAD_DECODER_H_
#define _K3B_MAD_DECODER_H_


#include "k3baudiodecoder.h"

extern "C" {
#include <mad.h>
}


class K3bMadDecoderFactory : public K3b::AudioDecoderFactory
{
    Q_OBJECT

public:
    explicit K3bMadDecoderFactory( QObject* parent = 0, const QVariantList& args = QVariantList() );
    ~K3bMadDecoderFactory() override;

    bool canDecode( const QUrl& filename ) override;

    int pluginSystemVersion() const override { return K3B_PLUGIN_SYSTEM_VERSION; }

    K3b::AudioDecoder* createDecoder( QObject* parent = 0 ) const override;
};


class K3bMadDecoder : public K3b::AudioDecoder
{
    Q_OBJECT

public:
    explicit K3bMadDecoder( QObject* parent = 0 );
    ~K3bMadDecoder() override;

    QString metaInfo( MetaDataField ) override;

    void cleanup() override;

    bool seekInternal( const K3b::Msf& ) override;

    QString fileType() const override;
    QStringList supportedTechnicalInfos() const override;
    QString technicalInfo( const QString& ) const override;

protected:
    bool analyseFileInternal( K3b::Msf& frames, int& samplerate, int& ch ) override;
    bool initDecoderInternal() override;

    int decodeInternal( char* _data, int maxLen ) override;
 
private:
    unsigned long countFrames();
    inline unsigned short linearRound( mad_fixed_t fixed );
    bool createPcmSamples( mad_synth* );

    static int MaxAllowedRecoverableErrors;

    class MadDecoderPrivate;
    MadDecoderPrivate* d;
};

#endif
