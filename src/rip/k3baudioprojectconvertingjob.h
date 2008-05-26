/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3B_AUDIO_PROJECT_CONVERTING_JOB_H
#define K3B_AUDIO_PROJECT_CONVERTING_JOB_H

#include <k3bthreadjob.h>
#include <qobject.h>
#include <q3valuevector.h>
#include <qpair.h>

#include <libkcddb/cdinfo.h>


class K3bAudioEncoder;
class K3bAudioDoc;
class K3bAudioTrack;


class K3bAudioProjectConvertingJob : public K3bThreadJob
{
    Q_OBJECT

public:
    K3bAudioProjectConvertingJob( K3bAudioDoc*, K3bJobHandler* hdl, QObject* parent );
    ~K3bAudioProjectConvertingJob();

    QString jobDescription() const;
    QString jobDetails() const;

    void setSingleFile( bool b ) { m_singleFile = b; }

    void setCddbEntry( const KCDDB::CDInfo& e ) { m_cddbEntry = e; }

    // if 0 (default) wave files are created
    void setEncoder( K3bAudioEncoder* f );

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

private:
    bool run();

    bool convertTrack( K3bAudioTrack*, const QString& filename );
    bool writePlaylist();
    bool writeCueFile();

    /**
     * Finds a relative path from baseDir to absPath
     */
    QString findRelativePath( const QString& absPath, const QString& baseDir );

    KCDDB::CDInfo m_cddbEntry;

    bool m_singleFile;
    bool m_writePlaylist;
    bool m_relativePathInPlaylist;
    QString m_playlistFilename;

    bool m_writeCueFile;

    QVector<QPair<int, QString> > m_tracks;

    K3bAudioDoc* m_doc;

    class Private;
    Private* d;
};

#endif
