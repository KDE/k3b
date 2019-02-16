/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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


#ifndef _K3B_MSF_H_
#define _K3B_MSF_H_

#include "k3bdevice_export.h"
#include <KIO/Global>
#include <QDebug>
#include <QSharedDataPointer>

namespace K3b
{
    /**
     * int values are always treated as frames
     * except in the set methods
     * A MSF is never < 0.
     */
    class LIBK3BDEVICE_EXPORT Msf
    {
    public:
        Msf();
        Msf( const Msf& );
        Msf( int, int, int );
        Msf( int );
        ~Msf();

        Msf& operator=( const Msf& );
        Msf& operator=( int );
        Msf& operator+=( const Msf& );
        Msf& operator+=( int );
        Msf& operator-=( const Msf& );
        Msf& operator-=( int );
        const Msf operator++( int );
        Msf& operator++();
        const Msf operator--( int );
        Msf& operator--();

        int minutes() const;
        int seconds() const;
        int frames() const;

        int totalFrames() const;
        int lba() const;

        //      operator int () const { return lba(); }

        void setValue( int m, int s, int f );

        void addMinutes( int m );
        void addSeconds( int s );
        void addFrames( int f );

        QString toString( bool showFrames = true ) const;

        KIO::filesize_t mode1Bytes() const;
        KIO::filesize_t mode2Form1Bytes() const;
        KIO::filesize_t mode2Form2Bytes() const;
        KIO::filesize_t audioBytes() const;
        KIO::filesize_t rawBytes() const;
        unsigned long long pcmSamples() const;

        /**
         * Convert a string representation into an Msf object.
         *
         * Valid strings include:
         * \li 100       - treated as 100 frames
         * \li 100:23    - treated as 100 minutes and 23 seconds
         * \li 100:23:57 - treated as 100 minutes, 23 seconds, and 57 frames
         * \li 100:23.57 - treated as 100 minutes, 23 seconds, and 57 frames
         */
        static Msf fromString( const QString&, bool* ok = 0 );

        /**
         * @param ms seconds
         * frames will be rounded up
         */
        static Msf fromSeconds( double ms );

        static Msf fromAudioBytes( qint64 bytes );

        static QRegExp regExp();

    private:
        class Private;
        QSharedDataPointer<Private> d;
    };

    LIBK3BDEVICE_EXPORT Msf operator+( const Msf&, const Msf& );
    LIBK3BDEVICE_EXPORT Msf operator+( const Msf&, int );
    LIBK3BDEVICE_EXPORT Msf operator-( const Msf&, const Msf& );
    LIBK3BDEVICE_EXPORT Msf operator-( const Msf&, int );
    LIBK3BDEVICE_EXPORT bool operator==( const Msf&, const Msf& );
    LIBK3BDEVICE_EXPORT bool operator!=( const Msf&, const Msf& );
    LIBK3BDEVICE_EXPORT bool operator<( const Msf&, const Msf& );
    LIBK3BDEVICE_EXPORT bool operator>( const Msf&, const Msf& );
    LIBK3BDEVICE_EXPORT bool operator<=( const Msf&, const Msf& );
    LIBK3BDEVICE_EXPORT bool operator>=( const Msf&, const Msf& );

    LIBK3BDEVICE_EXPORT QDebug& operator<<( QDebug&, const Msf& );
}

#endif
