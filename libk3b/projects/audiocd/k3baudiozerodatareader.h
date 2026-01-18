/*
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3B_AUDIOZERODATAREADER_H
#define K3B_AUDIOZERODATAREADER_H

#include "k3b_export.h"

#include <QIODevice>
#include <QScopedPointer>


namespace K3b {

    class AudioZeroData;

    class LIBK3B_EXPORT AudioZeroDataReader : public QIODevice
    {
    public:
        explicit AudioZeroDataReader( AudioZeroData& source, QObject* parent = nullptr );
        ~AudioZeroDataReader() override;

        bool open( OpenMode mode ) override;
        bool isSequential() const override;
        qint64 size() const override;

    protected:
        qint64 writeData(const char* data, qint64 len) override;
        qint64 readData(char* data, qint64 maxlen) override;

    private:
        class Private;
        QScopedPointer<Private> d;
        Q_DISABLE_COPY(AudioZeroDataReader)
    };

}

#endif // K3B_AUDIOZERODATAREADER_H
