/*
    SPDX-FileCopyrightText: 2004 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
