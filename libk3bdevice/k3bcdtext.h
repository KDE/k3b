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


#ifndef _K3B_CDTEXT_H_
#define _K3B_CDTEXT_H_

#include "k3bdevice_export.h"

#include <QSharedDataPointer>
#include <QString>

namespace K3b {
    namespace Device {
        class LIBK3BDEVICE_EXPORT TrackCdText
        {
        public:
            TrackCdText();
            TrackCdText( const TrackCdText& );
            ~TrackCdText();

            TrackCdText& operator=( const TrackCdText& );

            void clear();

            QString title() const;
            QString performer() const;
            QString songwriter() const;
            QString composer() const;
            QString arranger() const;
            QString message() const;
            QString isrc() const;

            // TODO: use the real CD-TEXT charset (a modified ISO8859-1)
            void setTitle( const QString& s );
            void setPerformer( const QString& s );
            void setSongwriter( const QString& s );
            void setComposer( const QString& s );
            void setArranger( const QString& s );
            void setMessage( const QString& s );
            void setIsrc( const QString& s );

            bool isEmpty() const;

            bool operator==( const TrackCdText& ) const;
            bool operator!=( const TrackCdText& ) const;

        private:
            class Private;
            QSharedDataPointer<Private> d;

            friend class CdText;
        };


        class LIBK3BDEVICE_EXPORT CdText
        {
        public:
            CdText();
            CdText( const unsigned char* data, int len );

            /**
             * \sa setRawPackData
             */
            explicit CdText( const QByteArray& );
            CdText( const CdText& );
            ~CdText();

            CdText& operator=( const CdText& );

            int count() const;

            /**
             * If i < count() behaviour is undefined.
             */
            TrackCdText operator[]( int i ) const;
            TrackCdText& operator[]( int i );

            void insert( int index, const TrackCdText& );

            /**
             * Will create a new empty TrackCdText if i is out of scope.
             */
            TrackCdText& track( int i );

            TrackCdText track( int i ) const;

            /**
             * Parse cd text from raw pack data. The data as provided is cached
             * and will be returned by rawPackData until the cdtext object is changed.
             */
            void setRawPackData( const unsigned char*, int );
            void setRawPackData( const QByteArray& );

            QByteArray rawPackData() const;

            bool empty() const;
            bool isEmpty() const;

            void clear();

            QString title() const;
            QString performer() const;
            QString songwriter() const;
            QString composer() const;
            QString arranger() const;
            QString message() const;
            QString discId() const;
            QString upcEan() const;

            // TODO: use the real CD-TEXT charset (a modified ISO8859-1)
            void setTitle( const QString& s );
            void setPerformer( const QString& s );
            void setSongwriter( const QString& s );
            void setComposer( const QString& s );
            void setArranger( const QString& s );
            void setMessage( const QString& s );
            void setDiscId( const QString& s );
            void setUpcEan( const QString& s );

            void debug() const;

            /**
             * Returns false if found a crc error in the raw cdtext block or it has a
             * wrong length.
             */
            static bool checkCrc( const unsigned char*, int );
            static bool checkCrc( const QByteArray& );

            bool operator==( const CdText& ) const;
            bool operator!=( const CdText& ) const;

        private:
            class Private;
            QSharedDataPointer<Private> d;
        };
    }
}

#endif
