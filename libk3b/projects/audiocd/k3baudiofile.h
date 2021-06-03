/*
    SPDX-FileCopyrightText: 2004-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_AUDIO_FILE_H_
#define _K3B_AUDIO_FILE_H_

#include "k3baudiodatasource.h"

#include "k3b_export.h"

#include <QScopedPointer>

namespace K3b {
    class AudioDecoder;

    /**
     * The AudioFile is the most important audio data source. It gets its data
     * from an audio file and uses a AudioDecoder to decode this data.
     *
     * Be aware that it is currently not possible to change the doc of an AudioFile.
     * The reason for this is the decoder sharing which is in place to allow gapless
     * splitting of audio files into several tracks.
     *
     * \see AudioDoc::createDecoderForUrl
     */
    class LIBK3B_EXPORT AudioFile : public AudioDataSource
    {
    public:
        /**
         * The AudioFile registers itself with the doc. This is part of the
         * decoder handling facility in AudioDoc which reuses the same decoder
         * for sources with the same url.
         *
         * Use AudioDoc::getDecoderForUrl to create a decoder.
         */
        AudioFile( AudioDecoder* decoder, AudioDoc* doc );
        AudioFile( const AudioFile& file );

        /**
         * The AudioFile deregisters itself from the doc. If it was the last file
         * to use the decoder the doc will take care of deleting it.
         */
        ~AudioFile() override;

        AudioDecoder* decoder() const;

        AudioDoc* doc() const;

        QString filename() const;

        /**
         * The complete length of the file used by this source.
         */
        Msf originalLength() const override;

        QString type() const override;
        QString sourceComment() const override;

        bool isValid() const override;

        AudioDataSource* copy() const override;

        QIODevice* createReader( QObject* parent = 0 ) override;

    private:
        class Private;
        QScopedPointer<Private> d;
    };
}

#endif
