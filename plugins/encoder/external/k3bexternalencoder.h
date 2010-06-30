/*
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
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
    ~K3bExternalEncoder();

    virtual QStringList extensions() const;

    virtual QString fileTypeComment( const QString& ) const;

    virtual int pluginSystemVersion() const { return K3B_PLUGIN_SYSTEM_VERSION; }

    /**
     * reimplemented since the external program is intended to write the file
     * TODO: allow writing to stdout.
     */
    virtual bool openFile( const QString& ext, const QString& filename, const K3b::Msf& length, const MetaData& metaData );
    virtual bool isOpen() const;
    virtual void closeFile();

private Q_SLOTS:
    void slotExternalProgramFinished( int, QProcess::ExitStatus );
    void slotExternalProgramOutput( const QString& line );

private:
    virtual bool initEncoderInternal( const QString& extension, const K3b::Msf& length, const MetaData& metaData );
    virtual void finishEncoderInternal();
    virtual qint64 encodeInternal( const char* data, qint64 len );
    bool writeWaveHeader();

    class Private;
    Private* d;
};

#endif
