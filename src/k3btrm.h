/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_TRM_H_
#define _K3B_TRM_H_

#include <config-k3b.h>

#ifdef ENABLE_MUSICBRAINZ

#include "k3bmsf.h"
#include <QByteArray>

/**
 * This class is a wrapper around the trm part of libmusicbrainz.
 * It handles proxy settings automatically through KDE.
 *
 * TRM always treats audio data as 44100, 2 channel, 16 bit data.
 */
namespace K3b {
class TRM
{
public:
    TRM();
    ~TRM();

    void start( const K3b::Msf& length );

    /**
     * \return true if no more data is needed
     */
    bool generate( char* data, int len );

    /**
     * \return true on success, false on error.
     */
    bool finalize();

    QByteArray rawSignature() const;
    QByteArray signature() const;

private:
    class Private;
    Private* d;
};
}

#endif
#endif
