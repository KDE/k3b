/*
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_LAME_ENCODER_H_
#define _K3B_LAME_ENCODER_H_

#include "k3baudioencoder.h"


class K3bLameEncoder : public K3b::AudioEncoder
{
    Q_OBJECT

public:
    K3bLameEncoder( QObject* parent, const QVariantList& );
    ~K3bLameEncoder() override;

    bool openFile( const QString& extension, const QString& filename, const K3b::Msf& length, const MetaData& metaData ) override;
    bool isOpen() const override;
    void closeFile() override;
    QString filename() const override;

    QStringList extensions() const override;

    QString fileTypeComment( const QString& ) const override;

    long long fileSize( const QString&, const K3b::Msf& msf ) const override;

    int pluginSystemVersion() const override { return K3B_PLUGIN_SYSTEM_VERSION; }

private:
    void finishEncoderInternal() override;
    bool initEncoderInternal( const QString& extension, const K3b::Msf& length, const MetaData& metaData ) override;
    qint64 encodeInternal( const char* data, qint64 len ) override;

    class Private;
    Private* d;
};

#endif
