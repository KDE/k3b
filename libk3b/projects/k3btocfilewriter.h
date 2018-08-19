/* 
 *
 * Copyright (C) 2004-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_TOC_FILE_WRITER_H_
#define _K3B_TOC_FILE_WRITER_H_

#include "k3btoc.h"
#include "k3bcdtext.h"

#include <QStringList>
#include <QTextStream>

namespace K3b {
    namespace Device {
        class TrackCdText;
    }

    class TocFileWriter
    {
    public:
        TocFileWriter();

        bool save( QTextStream& );
        bool save( const QString& filename );

        void setData( const Device::Toc& toc ) { m_toc = toc; }
        void setCdText( const Device::CdText& text ) { m_cdText = text; }
        void setFilenames( const QStringList& names ) { m_filenames = names; }
        void setHideFirstTrack( bool b ) { m_hideFirstTrack = b; }

        /**
         * The default is 1.
         */
        void setSession( int s ) { m_sessionToWrite = s; }

    private:
        void writeHeader( QTextStream& t );
        void writeGlobalCdText( QTextStream& t );
        void writeTrackCdText( const Device::TrackCdText& track, QTextStream& t );
        void writeTrack( int index, const Msf& offset, QTextStream& t );
        void writeDataSource( int trackNumber, QTextStream& t );
        bool readFromStdin() const;

        Device::Toc m_toc;
        Device::CdText m_cdText;
        QStringList m_filenames;
        bool m_hideFirstTrack;
        int m_sessionToWrite;
    };
}

#endif
