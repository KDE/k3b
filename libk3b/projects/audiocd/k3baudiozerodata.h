/*
 *
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
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
