/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_AUDIO_ZERO_DATA_H_
#define _K3B_AUDIO_ZERO_DATA_H_

#include "k3baudiodatasource.h"
#include "k3b_export.h"

namespace K3b {
    class LIBK3B_EXPORT AudioZeroData : public AudioDataSource
    {
    public:
        explicit AudioZeroData( const Msf& msf = 150 );
        AudioZeroData( const AudioZeroData& );
        ~AudioZeroData() override;

        Msf originalLength() const override { return m_length; }
        void setLength( const Msf& msf );

        QString type() const override;
        QString sourceComment() const override;

        AudioDataSource* copy() const override;
        QIODevice* createReader( QObject* parent = 0 ) override;

        /**
         * Only changes the length
         */
        void setStartOffset( const Msf& ) override;

        /**
         * Only changes the length
         */
        void setEndOffset( const Msf& ) override;

    private:
        Msf m_length;
    };
}

#endif
