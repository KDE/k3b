/*

    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_RAW_AUDIO_DATA_READER_H_
#define _K3B_RAW_AUDIO_DATA_READER_H_

#include "k3b_export.h"

#include <QIODevice>
#include <QScopedPointer>

namespace K3b {

    class RawAudioDataSource;

    class LIBK3B_EXPORT RawAudioDataReader : public QIODevice
    {
    public:
        explicit RawAudioDataReader( RawAudioDataSource& source, QObject* parent = 0 );
        ~RawAudioDataReader() override;

        bool open( OpenMode mode ) override;
        void close() override;
        bool isSequential() const override;
        qint64 size() const override;
        bool seek( qint64 pos ) override;

    protected:
        qint64 writeData( const char* data, qint64 len ) override;
        qint64 readData( char* data, qint64 maxlen ) override;

    private:
        class Private;
        QScopedPointer<Private> d;
        Q_DISABLE_COPY(RawAudioDataReader)
    };

} // namespace K3b

#endif // _K3B_RAW_AUDIO_DATA_READER_H_
