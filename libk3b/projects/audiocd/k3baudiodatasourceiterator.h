/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_AUDIO_DATA_SOURCE_ITERATOR_H_
#define _K3B_AUDIO_DATA_SOURCE_ITERATOR_H_

#include "k3b_export.h"

namespace K3b {
    class AudioDataSource;
    class AudioTrack;
    class AudioDoc;

    /**
     * This Iterator iterates over the sources in an audio project
     *
     * Be aware that this iterator does not properly update when the doc
     * changes. A manual update can be issued with first(). This is because
     * an update would either involve slots (this being a QObject) which is
     * too much overhead or the AudioDoc would need to have knowledge of all
     * the iterators which is also overhead that would be overkill.
     */
    class LIBK3B_EXPORT AudioDataSourceIterator
    {
    public:
        /**
         * This will place the iterator on the first source just like first() does.
         */
        explicit AudioDataSourceIterator( AudioDoc* );

        AudioDataSource* current() const;

        bool hasNext() const;

        /**
         * \return the next source or 0 if at end.
         */
        AudioDataSource* next();

        /**
         * Reset the iterator
         */
        AudioDataSource* first();

    private:
        AudioDoc* m_doc;
        AudioTrack* m_currentTrack;
        AudioDataSource* m_currentSource;
    };
}

#endif
