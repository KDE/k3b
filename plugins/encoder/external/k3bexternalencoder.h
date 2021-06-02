/*
    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef _K3B_EXTERNAL_ENCODER_H_
#define _K3B_EXTERNAL_ENCODER_H_

#include "k3baudioencoder.h"

#include "k3bprocess.h"

class K3bExternalEncoder : public K3b::AudioEncoder
{
    Q_OBJECT

public:
    K3bExternalEncoder( QObject* parent, const QVariantList& );
    ~K3bExternalEncoder() override;

    QStringList extensions() const override;

    QString fileTypeComment( const QString& ) const override;

    int pluginSystemVersion() const override { return K3B_PLUGIN_SYSTEM_VERSION; }

    /**
     * reimplemented since the external program is intended to write the file
     * TODO: allow writing to stdout.
     */
    bool openFile( const QString& ext, const QString& filename, const K3b::Msf& length, const MetaData& metaData ) override;
    bool isOpen() const override;
    void closeFile() override;

private Q_SLOTS:
    void slotExternalProgramFinished( int, QProcess::ExitStatus );
    void slotExternalProgramOutput( const QString& line );

private:
    bool initEncoderInternal( const QString& extension, const K3b::Msf& length, const MetaData& metaData ) override;
    void finishEncoderInternal() override;
    qint64 encodeInternal( const char* data, qint64 len ) override;
    bool writeWaveHeader();

    class Private;
    Private* d;
};

#endif
