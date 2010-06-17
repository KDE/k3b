/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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

#ifndef _K3B_LAME_ENCODER_H_
#define _K3B_LAME_ENCODER_H_

#include "k3baudioencoder.h"


class K3bLameEncoder : public K3b::AudioEncoder
{
    Q_OBJECT

public:
    K3bLameEncoder( QObject* parent, const QVariantList& );
    ~K3bLameEncoder();

    bool openFile( const QString& extension, const QString& filename, const K3b::Msf& length, const MetaData& metaData );
    bool isOpen() const;
    void closeFile();
    QString filename() const;

    QStringList extensions() const;

    QString fileTypeComment( const QString& ) const;

    long long fileSize( const QString&, const K3b::Msf& msf ) const;

    int pluginSystemVersion() const { return K3B_PLUGIN_SYSTEM_VERSION; }

private:
    void finishEncoderInternal();
    bool initEncoderInternal( const QString& extension, const K3b::Msf& length, const MetaData& metaData );
    qint64 encodeInternal( const char* data, qint64 len );

    class Private;
    Private* d;
};

K3B_EXPORT_PLUGIN(k3blameencoder, K3bLameEncoder)

#endif
