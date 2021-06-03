/*
    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_OGG_VORBIS_ENCODER_H_
#define _K3B_OGG_VORBIS_ENCODER_H_


#include "k3baudioencoder.h"

class K3bOggVorbisEncoder : public K3b::AudioEncoder
{
    Q_OBJECT

public:
    K3bOggVorbisEncoder( QObject* parent, const QVariantList& );
    ~K3bOggVorbisEncoder() override;

    QStringList extensions() const override { return QStringList("ogg"); }

    QString fileTypeComment( const QString& ) const override;

    long long fileSize( const QString&, const K3b::Msf& msf ) const override;

    int pluginSystemVersion() const override { return K3B_PLUGIN_SYSTEM_VERSION; }

private:
    void loadConfig();
    void finishEncoderInternal() override;
    bool initEncoderInternal( const QString& extension, const K3b::Msf& length, const MetaData& metaData ) override;
    qint64 encodeInternal( const char* data, qint64 len ) override;

    bool writeOggHeaders();
    void cleanup();
    long flushVorbis();

    class Private;
    Private* d;
};

#endif
