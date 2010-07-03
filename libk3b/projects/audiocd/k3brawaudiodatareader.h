/*
 *
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_RAW_AUDIO_DATA_READER_H_
#define _K3B_RAW_AUDIO_DATA_READER_H_

#include "k3b_export.h"

#include <QtCore/QIODevice>
#include <QtCore/QScopedPointer>

namespace K3b {

    class RawAudioDataSource;

    class LIBK3B_EXPORT RawAudioDataReader : public QIODevice
    {
    public:
        RawAudioDataReader( RawAudioDataSource& source, QObject* parent = 0 );
        ~RawAudioDataReader();

        virtual bool open( OpenMode mode );
        virtual void close();
        virtual bool isSequential() const;
        virtual qint64 size() const;
        virtual bool seek( qint64 pos );

    protected:
        virtual qint64 writeData( const char* data, qint64 len );
        virtual qint64 readData( char* data, qint64 maxlen );

    private:
        class Private;
        QScopedPointer<Private> d;
        Q_DISABLE_COPY(RawAudioDataReader)
    };

} // namespace K3b

#endif // _K3B_RAW_AUDIO_DATA_READER_H_
