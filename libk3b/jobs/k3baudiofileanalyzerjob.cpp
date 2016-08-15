/*
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baudiofileanalyzerjob.h"
#include "k3baudiodecoder.h"

class K3b::AudioFileAnalyzerJob::Private
{
public:
    K3b::AudioDecoder* decoder;
};


K3b::AudioFileAnalyzerJob::AudioFileAnalyzerJob( K3b::JobHandler* hdl, QObject* parent )
    : K3b::ThreadJob( hdl, parent ),
      d( new Private() )
{
    d->decoder = 0;
}


K3b::AudioFileAnalyzerJob::~AudioFileAnalyzerJob()
{
    delete d;
}


void K3b::AudioFileAnalyzerJob::setDecoder( K3b::AudioDecoder* decoder )
{
    d->decoder = decoder;
}


K3b::AudioDecoder* K3b::AudioFileAnalyzerJob::decoder() const
{
    return d->decoder;
}


bool K3b::AudioFileAnalyzerJob::run()
{
    if ( !d->decoder ) {
        emit infoMessage( "Internal error: no decoder set. This is a bug.", MessageError );
        return false;
    }

    return d->decoder->analyseFile();
}


