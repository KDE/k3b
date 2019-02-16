/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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


#ifndef K3BAUDIOTRACK_H
#define K3BAUDIOTRACK_H

#include "k3bmsf.h"
#include "k3bcdtext.h"
#include "k3btrack.h"
#include "k3b_export.h"

#include <KIO/Global>

#include <QObject>
#include <QString>

namespace K3b {
    class AudioDataSource;
    class AudioDoc;


    /**
     * @author Sebastian Trueg
     */
    class LIBK3B_EXPORT AudioTrack : public QObject
    {
        Q_OBJECT

        friend class AudioDataSource;
        friend class AudioDoc;

    public:
        AudioTrack();
        explicit AudioTrack( AudioDoc* parent );
        ~AudioTrack() override;

        AudioDoc* doc() const;

        Device::Track toCdTrack() const;

        /**
         * @return length of track in frames
         */
        Msf length() const;
        KIO::filesize_t size() const;

        QString artist() const;
        QString performer() const;
        QString title() const;
        QString arranger() const;
        QString songwriter() const;
        QString composer() const;
        QString isrc() const;
        QString cdTextMessage() const;
        Device::TrackCdText cdText() const;

        bool copyProtection() const;
        bool preEmp() const;

        /**
         * @obsolete use setPerformer
         **/
        void setArtist( const QString& a );
        void setPerformer( const QString& a );
        void setTitle( const QString& t );
        void setArranger( const QString& t );
        void setSongwriter( const QString& t );
        void setComposer( const QString& t );
        void setIsrc( const QString& t );
        void setCdTextMessage( const QString& t );

        void setCdText( const Device::TrackCdText& cdtext );

        void setPreEmp( bool b );
        void setCopyProtection( bool b );

        Msf index0() const;
        /**
         * The length of the postgap, ie. the number of blocks with index0.
         * This is always 0 for the last track.
         */
        Msf postGap() const;
        void setIndex0( const Msf& );

        /**
         * \return The track number starting at 1.
         */
        unsigned int trackNumber() const;

        /**
         * Remove this track from the list and return it.
         */
        AudioTrack* take();

        /**
         * Move this track after @p track.
         * If @p track is null this track will be merged into the beginning
         * of the docs list.
         */
        void moveAfter( AudioTrack* track );

        /**
         * Move this track ahead of @p track.
         * If @p track is null this track will be appended to the end
         * of the docs list.
         */
        void moveAhead( AudioTrack* track );

        /**
         * Merge @p trackToMerge into this one.
         */
        void merge( AudioTrack* trackToMerge, AudioDataSource* sourceAfter = 0 );

        AudioTrack* prev() const;
        AudioTrack* next() const;

        /**
         * Use with care.
         */
        void setFirstSource( AudioDataSource* source );
        AudioDataSource* firstSource() const;
        AudioDataSource* lastSource() const;
        int numberSources() const;

        /**
         * Append source to the end of the sources list.
         */
        void addSource( AudioDataSource* source );

        /**
         * called by AudioDataSource because of the lack of signals
         */
        void sourceChanged( AudioDataSource* );

        /**
         * Create a copy of this track containing copies of all the sources
         * but not being part of some list.
         */
        AudioTrack* copy() const;

        /**
         * Split the track at position pos and return the split track
         * on success.
         * The new track will be moved after this track.
         *
         * \param pos The position at which to split. \a pos will be the
         * first frame in the new track.
         */
        AudioTrack* split( const Msf& pos );

        /**
         * Create reader associated with the track
         */
        virtual QIODevice* createReader( QObject* parent = 0 );

        /**
         * Is this track in a list
         */
        bool inList() const;

        /**
         * Get the source at index.
         * \return the requested source or 0 if index is out
         * of bounds.
         */
        AudioDataSource* getSource( int index ) const;

    Q_SIGNALS:
        void sourceAboutToBeRemoved( int position );
        void sourceRemoved( int position );
        void sourceAboutToBeAdded( int position );
        void sourceAdded( int position );
        void changed();

    protected:
        /**
         * Tells the audio doc one source is about to be removed
         */
        void emitSourceAboutToBeRemoved( AudioDataSource* source );

        /**
         * Tells the audio doc one source was removed from the list.
         */
        void emitSourceRemoved( AudioDataSource* source );

        /**
         * Tells the audio doc one source is about to be added
         */
        void emitSourceAboutToBeAdded( int position );

        /**
         * Tells the audio doc one source was added to the list.
         */
        void emitSourceAdded( AudioDataSource* source );

        void setIndex0Offset( const Msf& msf );
        void setParent( AudioDoc* parent );

    private:
        /**
         * Tells the doc that the track has changed
         */
        void emitChanged();

        void debug();

        class Private;
        Private* d;
    };
}

#endif
