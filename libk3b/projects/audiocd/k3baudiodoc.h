/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C)      2009 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 * Copyright (C)      2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BAUDIODOC_H
#define K3BAUDIODOC_H

#include "k3bdoc.h"
#include "k3bcdtext.h"
#include "k3btoc.h"

#include "k3b_export.h"

class QDomElement;

namespace K3b {
    class AudioTrack;
    class AudioDataSource;
    class AudioDecoder;
    class AudioFile;

    /**
     * Document class for an audio project.
     * @author Sebastian Trueg
     */
    class LIBK3B_EXPORT AudioDoc : public Doc
    {
        Q_OBJECT

        friend class MixedDoc;
        friend class AudioTrack;
        friend class AudioFile;

    public:
        explicit AudioDoc( QObject* );
        ~AudioDoc() override;

        Type type() const override { return AudioProject; }
        QString typeString() const override { return QString::fromLatin1("audio"); }

        QString name() const override;

        bool newDocument() override;

        void clear() override;

        Device::MediaTypes supportedMediaTypes() const override;

        bool hideFirstTrack() const;
        int numOfTracks() const override;

        bool normalize() const;

        AudioTrack* firstTrack() const;
        AudioTrack* lastTrack() const;

        /**
         * Slow.
         * \return the AudioTrack with track number trackNum (starting at 1) or 0 if trackNum > numOfTracks()
         */
        AudioTrack* getTrack( int trackNum );

        /**
         * Creates a new audiofile inside this doc which has no track yet.
         */
        AudioFile* createAudioFile( const QUrl& url );

        /** get the current size of the project */
        KIO::filesize_t size() const override;
        Msf length() const override;

        // CD-Text
        bool cdText() const;
        QString title() const;
        QString artist() const;
        QString disc_id() const;
        QString arranger() const;
        QString songwriter() const;
        QString composer() const;
        QString upc_ean() const;
        QString cdTextMessage() const;

        /**
         * Create complete CD-Text including the tracks' data.
         */
        Device::CdText cdTextData() const;

        int audioRippingParanoiaMode() const;
        int audioRippingRetries() const;
        bool audioRippingIgnoreReadErrors() const;

        /**
         * Represent the structure of the doc as CD Table of Contents.
         */
        Device::Toc toToc() const;

        BurnJob* newBurnJob( JobHandler*, QObject* parent = 0 ) override;

        /**
         * returns the new after track, ie. the last added track or null if
         * the import failed.
         *
         * This is a blocking method.
         *
         * \param cuefile The Cuefile to be imported
         * \param after   The track after which the new tracks should be inserted
         * \param decoder The decoder to be used for the new tracks. If 0 a new one will be created.
         *
         * BE AWARE THAT THE DECODER HAS TO FIT THE AUDIO FILE IN THE CUE.
         */
        AudioTrack* importCueFile( const QString& cuefile, AudioTrack* after, AudioDecoder* decoder = 0 );

        /**
         * Create a decoder for a specific url. If another AudioFileSource with this
         * url is already part of this project the associated decoder is returned.
         *
         * In the first case the decoder will not be initialized yet (AudioDecoder::analyseFile
         * is not called yet).
         *
         * \param url The url for which a decoder is requested.
         * \param reused If not null this variable is set to true if the decoder is already in
         *               use and AudioDecoder::analyseFile() does not have to be called anymore.
         */
        AudioDecoder* getDecoderForUrl( const QUrl& url, bool* reused = 0 );

        /**
         * Transforms given url list into flat file list.
         * Each directory and M3U playlist is expanded into the files.
         * Note: directories are not expanded recursively.
         */
        static QList<QUrl> extractUrlList( const QList<QUrl>& urls );

        static bool readPlaylistFile( const QUrl& url, QList<QUrl>& playlist );

    public Q_SLOTS:
        void addUrls( const QList<QUrl>& ) override;
        void addTrack( const QUrl&, int );
        void addTracks( const QList<QUrl>&, int );
        /**
         * Adds a track without any testing
         *
         * Slow because it uses getTrack.
         */
        void addTrack( AudioTrack* track, int position = 0 );

        void addSources( AudioTrack* parent, const QList<QUrl>& urls, AudioDataSource* sourceAfter = 0 );

        void removeTrack( AudioTrack* );
        void moveTrack( AudioTrack* track, AudioTrack* after );

        void setHideFirstTrack( bool b );
        void setNormalize( bool b );

        // CD-Text
        void writeCdText( bool b );
        void setTitle( const QString& v );
        void setArtist( const QString& v );
        void setPerformer( const QString& v );
        void setDisc_id( const QString& v );
        void setArranger( const QString& v );
        void setSongwriter( const QString& v );
        void setComposer( const QString& v );
        void setUpc_ean( const QString& v );
        void setCdTextMessage( const QString& v );

        // Audio-CD Ripping
        void setAudioRippingParanoiaMode( int i );
        void setAudioRippingRetries( int r );
        void setAudioRippingIgnoreReadErrors( bool b );

    private Q_SLOTS:
        void slotTrackChanged( K3b::AudioTrack* track );
        void slotTrackRemoved( int position );

    Q_SIGNALS:
        void trackChanged( K3b::AudioTrack* track );

        // signals for the model
        void trackAboutToBeAdded( int position );
        void trackAdded( int position );
        void trackAboutToBeRemoved( int position );
        void trackRemoved( int position );
        void sourceAboutToBeAdded( K3b::AudioTrack* parent, int position );
        void sourceAdded( K3b::AudioTrack* parent, int position );
        void sourceAboutToBeRemoved( K3b::AudioTrack* parent, int position );
        void sourceRemoved( K3b::AudioTrack* parent, int position );

    protected:
        /** reimplemented from Doc */
        bool loadDocumentData( QDomElement* ) override;
        /** reimplemented from Doc */
        bool saveDocumentData( QDomElement* ) override;

    private:
        // the stuff for adding files
        // ---------------------------------------------------------
        AudioTrack* createTrack( const QUrl& url );

        /**
         * Used by AudioTrack to update the track list
         */
        void setFirstTrack( AudioTrack* track );
        /**
         * Used by AudioTrack to update the track list
         */
        void setLastTrack( AudioTrack* track );

        /**
         * Used by AudioFile to tell the doc that it does not need the decoder anymore.
         */
        void decreaseDecoderUsage( AudioDecoder* );
        void increaseDecoderUsage( AudioDecoder* );

        class Private;
        Private* d;
    };
}

#endif
