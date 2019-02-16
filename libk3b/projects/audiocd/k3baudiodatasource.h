/*
 *
 * Copyright (C) 2004-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_AUDIO_DATA_SOURCE_H_
#define _K3B_AUDIO_DATA_SOURCE_H_

#include "k3bmsf.h"
#include "k3b_export.h"

#include <QIODevice>
#include <QObject>

namespace K3b {
    class AudioTrack;
    class AudioDoc;


    /**
     * An AudioDataSource has an original length which represents the maximum amount of audio
     * sectors this source can provide (in special cases this is not true, see AudioZeroData).
     *
     * It is possible to just use a portion of that data by changing the startOffset and endOffset.
     * This will change the actual length of the data provided by this source through the read method.
     *
     * Sources are part of a list which can be traversed via the prev() and next() methods. This list
     * is part of a AudioTrack which in turn is part of a list which is owned by a AudioDoc.
     *
     * The list may be modified with the take(), moveAfter(), and moveAhead() methods. The source takes
     * care of fixing the list and notifying the track about the change (It is also possible to move sources
     * from one track to the other).
     *
     * When a source is deleted it automatically removes itself from it's list.
     */
    class LIBK3B_EXPORT AudioDataSource : public QObject
    {
        Q_OBJECT

        friend class AudioTrack;

    public:
        AudioDataSource();

        /**
         * Create en identical copy except that the copy will not be in any list.
         */
        AudioDataSource( const AudioDataSource& );
        ~AudioDataSource() override;

        /**
         * The original length of the source is the maximum data which is available
         * when startOffset is 0 this is the max for endOffset
         *
         * Be aware that this may change (see AudioZeroData)
         */
        virtual Msf originalLength() const = 0;

        /**
         * The default implementation returns the originalLength modified by startOffset and endOffset
         */
        virtual Msf length() const;

        /**
         * @return The index of this track (counting from 0)
         */
        int sourceIndex() const;

        /**
         * @return The raw size in pcm samples (16bit, 44800 kHz, stereo)
         */
        KIO::filesize_t size() const { return length().audioBytes(); }

        /**
         * Type of the data in readable form.
         */
        virtual QString type() const = 0;

        /**
         * The source in readable form (this is the filename for files)
         */
        virtual QString sourceComment() const = 0;

        /**
         * Used in case an error occurred. For now this is used if the
         * decoder was not able to decode an audiofile
         */
        virtual bool isValid() const { return true; }

        /**
         * The doc the source is currently a part of or null.
         */
        AudioDoc* doc() const;
        AudioTrack* track() const { return m_track; }

        AudioDataSource* prev() const { return m_prev; }
        AudioDataSource* next() const { return m_next; }

        AudioDataSource* take();

        void moveAfter( AudioDataSource* track );
        void moveAhead( AudioDataSource* track );

        /**
         * Set the start offset from the beginning of the source's originalLength.
         */
        virtual void setStartOffset( const Msf& );

        /**
         * Set the end offset from the beginning of the file. The endOffset sector
         * is not included in the data.
         * The maximum value is originalLength() which means to use all data.
         * 0 means the same as originalLength().
         * This has to be bigger than the start offset.
         */
        virtual void setEndOffset( const Msf& );

        virtual const Msf& startOffset() const { return m_startOffset; }

        /**
         * The end offset. It is the first sector not included in the data.
         * If 0 the last sector is determined by the originalLength
         */
        virtual const Msf& endOffset() const { return m_endOffset; }

        /**
         * Get the last used sector in the source.
         * The default implementation uses originalLength() and endOffset()
         */
        virtual Msf lastSector() const;

        /**
         * Create a copy of this source which is not part of a list
         */
        virtual AudioDataSource* copy() const = 0;

        /**
         * Split the source at position pos and return the split source
         * on success.
         * The new source will be moved after this source.
         *
         * The default implementation uses copy() to create a new source instance
         */
        virtual AudioDataSource* split( const Msf& pos );

        /**
         * Create reader associated with the source
         */
        virtual QIODevice* createReader( QObject* parent = 0 ) = 0;

    Q_SIGNALS:
        void changed();

    protected:
        /**
         * Informs the parent track about changes.
         */
        void emitChange();

    private:
        void fixupOffsets();

        AudioTrack* m_track;
        AudioDataSource* m_prev;
        AudioDataSource* m_next;

        Msf m_startOffset;
        Msf m_endOffset;
    };
}

#endif
