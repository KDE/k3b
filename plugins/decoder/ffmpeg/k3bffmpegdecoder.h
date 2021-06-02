/* 


    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef _K3B_FFMPEG_DECODER_H_
#define _K3B_FFMPEG_DECODER_H_

#include "k3baudiodecoder.h"

class K3bFFMpegFile;


class K3bFFMpegDecoderFactory : public K3b::AudioDecoderFactory
{
    Q_OBJECT

public:
    K3bFFMpegDecoderFactory( QObject* parent, const QVariantList& args  );
    ~K3bFFMpegDecoderFactory() override;

    bool canDecode( const QUrl& filename ) override;

    int pluginSystemVersion() const override { return K3B_PLUGIN_SYSTEM_VERSION; }

    bool multiFormatDecoder() const override { return true; }

    K3b::AudioDecoder* createDecoder( QObject* parent = 0  ) const override;
};


class K3bFFMpegDecoder : public K3b::AudioDecoder
{
    Q_OBJECT

public:
    explicit K3bFFMpegDecoder( QObject* parent = 0  );
    ~K3bFFMpegDecoder() override;

    QString fileType() const override;

    void cleanup() override;

protected:
    bool analyseFileInternal( K3b::Msf& frames, int& samplerate, int& ch ) override;
    bool initDecoderInternal() override;
    bool seekInternal( const K3b::Msf& ) override;

    int decodeInternal( char* _data, int maxLen ) override;

private:
    K3bFFMpegFile* m_file;
    QString m_type;
};

#endif
