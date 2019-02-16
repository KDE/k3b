/*
 *
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_AUDIO_DOC_READER_H_
#define _K3B_AUDIO_DOC_READER_H_

#include "k3b_export.h"

#include <QIODevice>
#include <QScopedPointer>

namespace K3b {

    class AudioDoc;
    class AudioTrack;
    class AudioTrackReader;

    class LIBK3B_EXPORT AudioDocReader : public QIODevice
    {
        Q_OBJECT

    public:
        explicit AudioDocReader( AudioDoc& doc, QObject* parent = 0 );
        ~AudioDocReader() override;

        AudioTrackReader* currentTrackReader() const;
        bool setCurrentTrack( const AudioTrack& track );

        bool open( OpenMode mode = ReadOnly ) override;
        void close() override;
        bool isSequential() const override;
        qint64 size() const override;
        bool seek( qint64 pos ) override;

    public Q_SLOTS:
        void nextTrack();
        void previousTrack();

    Q_SIGNALS:
        void currentTrackChanged( const K3b::AudioTrack& track );

    protected:
        qint64 writeData( const char* data, qint64 len ) override;
        qint64 readData( char* data, qint64 maxlen ) override;

    private:
        void updatePos();

    private:
        class Private;
        QScopedPointer<Private> d;
        Q_DISABLE_COPY(AudioDocReader)
        Q_PRIVATE_SLOT( d, void slotTrackAdded( int position ) )
        Q_PRIVATE_SLOT( d, void slotTrackAboutToBeRemoved( int position ) )
    };

} // namespace K3b

#endif // _K3B_AUDIO_DOC_READER_H_
