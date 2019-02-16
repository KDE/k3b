/*
 *
 *  Copyright (C) 2011 Michal Malek <michalm@jabster.pl>
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

#ifndef K3BMASSAUDIOENCODINGJOB_H
#define K3BMASSAUDIOENCODINGJOB_H

#include "k3bmsf.h"
#include "k3bthreadjob.h"

#include <QHash>
#include <QMultiMap>
#include <QScopedPointer>
#include <QString>

class QIODevice;

namespace KCDDB {
    class CDInfo;
}

namespace K3b {
    class AudioEncoder;

    class MassAudioEncodingJob : public ThreadJob
    {
        Q_OBJECT
    
    public:
        /**
        * Maps filenames to track 1-based track indexes
        * When multiple tracks are associated with one filename
        * they are merged and encoded into one file.
        */
        typedef QMultiMap<QString,int> Tracks;

    public:
        MassAudioEncodingJob( bool bigEndian, JobHandler* jobHandler, QObject* parent );
        ~MassAudioEncodingJob() override;

        /**
         * Sets CDDB information for the list of track
         */
        void setCddbEntry( const KCDDB::CDInfo& cddbEntry );
        const KCDDB::CDInfo& cddbEntry() const;

        /**
         * Sets audio encoder the tracks will be encoded with.
         * If encoder=0 (default) wave files are created
         */
        void setEncoder( AudioEncoder* encoder );
        AudioEncoder* encoder() const;

        /**
        * Sets additional file format information
        * used for encoders that support multiple formats
        */
        void setFileType( const QString& fileType );
        const QString& fileType() const;

        /**
         * Sets list of files to encode
         */
        void setTrackList( const Tracks& tracks );
        const Tracks& trackList() const;
        
        /**
         * Enables creating plyalist for encoded tracks
         */
        void setWritePlaylist( const QString& filename, bool relativePaths );
        
        /**
         * Enables writing CUE file for encoded tracks
         */
        void setWriteCueFile( bool writeCueFile );
        
        QString jobDetails() const override;
        QString jobTarget() const override;
        
    protected:
        /**
         * Initializes the job, shows information etc. Runs just before
         * actual encoding loop. By default does nothing and returns true.
         * @return true on success, false on failure
         */
        virtual bool init();

        /**
         * Performs cleanup just before leaving run() function.
         */
        virtual void cleanup();
        
        /**
         * @param trackIndex 1-based track index
         * @returns track length
         */
        virtual Msf trackLength( int trackIndex ) const = 0;
        
        /**
         * Creates reader for a given track
         */
        virtual QIODevice* createReader( int trackIndex ) const = 0;
        
        /**
         * Prints information about currently processed track
         */
        virtual void trackStarted( int trackIndex ) = 0;
        
        /**
         * Prints information about previously processed track
         */
        virtual void trackFinished( int trackIndex, const QString& filename ) = 0;
        
    private:
        bool run() override;
        
        /**
         * Reads data from source and encode it to provided filename
         * \param trackIndex 1-based track index
         * \param filename path to the target file
         * \param filename path to previously encoded file
         */
        bool encodeTrack( int trackIndex, const QString& filename, const QString& prevFilename );

        /**
         * Writes a playlist file for previously specified tracks
         */
        bool writePlaylist();
        
        /**
         * Writes a CUE file for previously specified track(s)
         */
        bool writeCueFile();
        
    private:
        class Private;
        QScopedPointer<Private> d;
    };

} // namespace K3b

#endif // K3BMASSAUDIOENCODERJOB_H
