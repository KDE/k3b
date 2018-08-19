/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BTOC_H
#define K3BTOC_H

#include "k3bdevice_export.h"
#include "k3bmsf.h"
#include "k3btrack.h"

#include <QList>

namespace K3b {
    namespace Device
    {
        enum ContentsType {
            DATA,
            AUDIO,
            MIXED,
            NONE // no tracks
        };

        /**
         * A list of Tracks that represents the contents
         * of a cd.
         * The Toc deletes all its tracks when it is deleted and
         * deletes removed tracks.
         */
        class LIBK3BDEVICE_EXPORT Toc : public QList<Track>
        {
        public:
            Toc();
            /** deep copy */
            Toc( const Toc& );
            /** deletes all tracks */
            ~Toc();
            /** deep copy */
            Toc& operator=( const Toc& );

            /**
             * CDDB disc Id
             */
            unsigned int discId() const;

            QByteArray mcn() const;

            /**
             * determine the contents type based on the tracks' types.
             * Audio, Data, or Mixed
             */
            ContentsType contentType() const;

            /**
             * \return the number of sessions in this TOC.
             */
            int sessions() const;

            /**
             * The first track's first sector could differ from the disc's
             * first sector if there is a pregap before index 1
             */
            K3b::Msf firstSector() const;
            K3b::Msf lastSector() const;
            K3b::Msf length() const;

            void setMcn( const QByteArray& mcn );

            void clear();

            bool operator==( const Toc& ) const;
            bool operator!=( const Toc& ) const;

        private:
            QByteArray m_mcn;
        };
    }
}

LIBK3BDEVICE_EXPORT QDebug operator<<( QDebug s, const K3b::Device::Toc& );

#endif
