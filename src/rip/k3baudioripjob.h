/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3B_AUDIO_RIP_THREAD_H
#define K3B_AUDIO_RIP_THREAD_H

#include <k3bthreadjob.h>
#include <q3valuevector.h>
#include <qpair.h>

#include <libkcddb/cdinfo.h>


namespace K3b {
    class AudioEncoder;

    namespace Device {
        class Device;
    }

    class AudioRipJob : public ThreadJob
    {
        Q_OBJECT

    public:
        AudioRipJob( JobHandler* hdl, QObject* parent );
        ~AudioRipJob();

        QString jobDescription() const;
        QString jobDetails() const;

        // paranoia settings
        void setParanoiaMode( int mode );
        void setMaxRetries( int r );
        void setNeverSkip( bool b );

        void setSingleFile( bool b ) { m_singleFile = b; }

        void setUseIndex0( bool b ) { m_useIndex0 = b; }

        void setDevice( Device::Device* dev ) { m_device = dev; }

        void setCddbEntry( const KCDDB::CDInfo& e ) { m_cddbEntry = e; }

        // if 0 (default) wave files are created
        void setEncoder( AudioEncoder* f );

        /**
         * Used for encoders that support multiple formats
         */
        void setFileType( const QString& );

        /**
         * 1 is the first track
         */
        void setTracksToRip( const QVector<QPair<int, QString> >& t ) { m_tracks = t; }

        void setWritePlaylist( bool b ) { m_writePlaylist = b; }
        void setPlaylistFilename( const QString& s ) { m_playlistFilename = s; }
        void setUseRelativePathInPlaylist( bool b ) { m_relativePathInPlaylist = b; }
        void setWriteCueFile( bool b ) { m_writeCueFile = b; }

    public Q_SLOTS:
        void start();

    private:
        bool run();
        void jobFinished( bool );

        bool ripTrack( int track, const QString& filename );
        void cleanupAfterCancellation();
        bool writePlaylist();
        bool writeCueFile();

        /**
         * Finds a relative path from baseDir to absPath
         */
        QString findRelativePath( const QString& absPath, const QString& baseDir );

        KCDDB::CDInfo m_cddbEntry;
        Device::Device* m_device;

        bool m_bUsePattern;
        bool m_singleFile;
        bool m_useIndex0;

        bool m_writePlaylist;
        bool m_relativePathInPlaylist;
        QString m_playlistFilename;

        bool m_writeCueFile;

        QVector<QPair<int, QString> > m_tracks;

        class Private;
        Private* d;
    };
}

#endif
