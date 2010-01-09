/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C)      2010 Michal Malek <michalm@jabster.pl>
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

#include "k3bthreadjob.h"
#include <QMultiMap>


namespace KCDDB {
    class CDInfo;
}

namespace K3b {
    
class AudioEncoder;
class AudioDoc;
class AudioTrack;
    
class AudioProjectConvertingJob : public ThreadJob
{
    Q_OBJECT

public:
    /**
        * Maps filenames to track 1-based track indexes
        * When multiple tracks are associated with one filename
        * they are merged into one file.
        */
    typedef QMultiMap<QString,int> Tracks;

public:
    AudioProjectConvertingJob( AudioDoc* doc, JobHandler* hdl, QObject* parent );
    ~AudioProjectConvertingJob();

    QString jobDescription() const;
    QString jobDetails() const;

    void setCddbEntry( const KCDDB::CDInfo& cddbEntry );

    // if 0 (default) wave files are created
    void setEncoder( AudioEncoder* encoder );

    /**
     * Used for encoders that support multiple formats
     */
    void setFileType( const QString& fileType );

    void setTracksToRip( const Tracks& tracks );

    void setWritePlaylist( const QString& filename, bool relativePaths );
    void setWriteCueFile( bool b );

private:
    bool run();

    bool convertTrack( AudioTrack& track, const QString& filename, const QString& prevFilename );
    bool writePlaylist();
    bool writeCueFile();

    /**
     * Finds a relative path from baseDir to absPath
     */
    QString findRelativePath( const QString& absPath, const QString& baseDir );

    class Private;
    Private* d;
};
}

#endif
