/*
    SPDX-FileCopyrightText: 2004 Matthieu Bedouet <mbedouet@no-log.org>
    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_LIBSNDFILE_DECODER_H_
#define _K3B_LIBSNDFILE_DECODER_H_

#include "k3baudiodecoder.h"

class K3bLibsndfileDecoderFactory : public K3b::AudioDecoderFactory
{
    Q_OBJECT

public:
    K3bLibsndfileDecoderFactory( QObject* parent, const QVariantList& args  );
    ~K3bLibsndfileDecoderFactory() override;

    bool canDecode( const QUrl& filename ) override;

    int pluginSystemVersion() const override { return K3B_PLUGIN_SYSTEM_VERSION; }

    bool multiFormatDecoder() const override { return true; }

    K3b::AudioDecoder* createDecoder( QObject* parent = 0 ) const override;
};


class K3bLibsndfileDecoder : public K3b::AudioDecoder
{
    Q_OBJECT

public:
    explicit K3bLibsndfileDecoder( QObject* parent = 0  );
    ~K3bLibsndfileDecoder() override;
    void cleanup() override;
    QString fileType() const override;

protected:
    bool analyseFileInternal( K3b::Msf& frames, int& samplerate, int& ch ) override;
    bool initDecoderInternal() override;
    bool seekInternal( const K3b::Msf& ) override;

    int decodeInternal( char* _data, int maxLen ) override;
 
private:
    bool openFile();

    class Private;
    Private* d;
  
};

#endif
