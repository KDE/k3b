/*

    SPDX-FileCopyrightText: 2004-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#include "k3btocfilewriter.h"

#include "k3btrack.h"
#include "k3bmsf.h"
#include "k3bcore.h"
#include "k3bversion.h"

#include <QDateTime>
#include <QFile>


K3b::TocFileWriter::TocFileWriter()
    : m_hideFirstTrack(false),
      m_sessionToWrite(1)
{
}


bool K3b::TocFileWriter::save( const QString& filename )
{
    QFile f( filename );

    if( !f.open( QIODevice::WriteOnly ) ) {
        qDebug() << "(K3b::CueFileWriter) could not open file " << f.fileName();
        return false;
    }

    QTextStream s( &f );

    return save( s );
}


bool K3b::TocFileWriter::save( QTextStream& t )
{
    writeHeader(t);

    if( !m_cdText.isEmpty() )
        writeGlobalCdText(t);

    //
    // see if we have multiple sessions
    //
    int sessions = 1;
    for( K3b::Device::Toc::iterator it = m_toc.begin(); it != m_toc.end(); ++it ) {
        if( (*it).session() > 1 )
            sessions = (*it).session();
    }

    if( m_sessionToWrite > sessions )
        m_sessionToWrite = 1;

    //
    // We can only hide the first track if both the first and the second track are
    // audio tracks.
    // We also can only hide the first track in the first session.
    //
    bool hideFirstTrack = m_hideFirstTrack;
    if( m_toc.count() < 2 ||
        m_toc[0].type() != K3b::Device::Track::TYPE_AUDIO ||
        m_toc[1].type() != K3b::Device::Track::TYPE_AUDIO ||
        (sessions > 1 && m_sessionToWrite != 1 ) )
        hideFirstTrack = false;


    // the dataStart will be the offset in case we do not write the first session
    K3b::Msf dataStart;

    int trackIndex = 0;
    if( hideFirstTrack ) {
        const K3b::Device::Track& hiddenTrack = m_toc[0];
        const K3b::Device::Track& track = m_toc[1];

        t << "// Track number 1 (hidden) and track number 2 (as track 1)" << endl;
        t << "TRACK AUDIO" << endl;

        if( track.copyPermitted() )
            t << "COPY" << endl;
        else
            t << "NO COPY" << endl;

        if( track.preEmphasis() )
            t << "PRE_EMPHASIS" << endl;
        else
            t << "NO PRE_EMPHASIS" << endl;

        if( !m_cdText.isEmpty() )
            writeTrackCdText( m_cdText[0], t );

        // the "hidden" file will be used as pregap for the "first" track
        t << "AUDIOFILE ";
        writeDataSource( 0, t );
        if( readFromStdin() )
            t << hiddenTrack.firstSector().toString();
        else
            t << " 0";
        t << " " << hiddenTrack.length().toString() << endl;
        t << "START" << endl; // use the whole hidden file as pregap

        // now comes the "real" first track
        t << "AUDIOFILE ";
        writeDataSource( 1, t );
        if( readFromStdin() )
            t << track.firstSector().toString() << " ";
        else
            t << "0 ";
        // no index 0 for the last track. Or should we allow this???
        if( m_toc.count() == 2 )
            t << track.length().toString();
        else
            t << track.realAudioLength().toString();
        t << endl << endl;

        trackIndex+=2;
    }
    else {
        //
        // Seek to the first track to write.
        // In case we hid the first track above it was the first track anyway.
        //
        while( m_toc[trackIndex].session() < m_sessionToWrite &&
               m_toc[trackIndex].session() > 0 )
            ++trackIndex;

        dataStart = m_toc[trackIndex].firstSector();
    }

    qDebug() << "(K3b::TocFileWriter) using offset of: " << dataStart.toString();

    while( trackIndex < m_toc.count() ) {
        if( m_toc[trackIndex].session() == 0 || m_toc[trackIndex].session() == m_sessionToWrite )
            writeTrack( trackIndex, dataStart, t );
        trackIndex++;
    }

    return ( t.status() == QTextStream::Ok );
}


void K3b::TocFileWriter::writeHeader( QTextStream& t )
{
    // little comment
    t << "// TOC-file to use with cdrdao created by K3b " << k3bcore->version()
      << ", " << QDateTime::currentDateTime().toString() << endl << endl;

    t << "// " << m_toc.count() << " tracks" << endl;
    if( m_toc.back().session() > 0 ) {
        t << "// " << m_toc.back().session() << " sessions" << endl
          << "// this is session number " << m_sessionToWrite << endl;
    }
    t << endl;

    // check the cd type
    if( m_toc.contentType() == K3b::Device::AUDIO ) {
        t << "CD_DA";
    }
    else {
        bool hasMode2Tracks = false;
        for( K3b::Device::Toc::iterator it = m_toc.begin(); it != m_toc.end(); ++it ) {
            const K3b::Device::Track& track = *it;
            if( track.type() == K3b::Device::Track::TYPE_DATA &&
                (track.mode() == K3b::Device::Track::MODE2 ||
                 track.mode() == K3b::Device::Track::XA_FORM1 ||
                 track.mode() == K3b::Device::Track::XA_FORM2 ) ) {
                hasMode2Tracks = true;
                break;
            }
        }

        if( hasMode2Tracks )
            t << "CD_ROM_XA";
        else
            t << "CD_ROM";
    }

    t << endl << endl;
}


void K3b::TocFileWriter::writeTrack( int index, const K3b::Msf& offset, QTextStream& t )
{
    const K3b::Device::Track& track = m_toc[index];

    t << "// Track number " << (index+1) << endl;

    if( track.type() == K3b::Device::Track::TYPE_AUDIO ) {
        t << "TRACK AUDIO" << endl;

        if( track.copyPermitted() )
            t << "COPY" << endl;
        else
            t << "NO COPY" << endl;

        if( track.preEmphasis() )
            t << "PRE_EMPHASIS" << endl;
        else
            t << "NO PRE_EMPHASIS" << endl;

        if( !m_cdText.isEmpty() )
            writeTrackCdText( m_cdText[index], t );

        //
        // cdrdao sees the pregap as part of the current track and not as part of
        // the previous like it really is.
        //

        if( index == 0 ) {
            if( (track.firstSector()-offset) > 0 ) {
                //
                // the first track is the only track K3b does not generate null-pregap data for
                // since cdrecord does not allow this. So We just do it here the same way and tell
                // cdrdao to create the first pregap for us
                //

                t << "PREGAP "
                  << (track.firstSector()-offset).toString() << endl;
            }
        }
        else {
            const K3b::Device::Track& lastTrack = m_toc[index-1];

            //
            // the pregap data
            //
            if( lastTrack.index0() > 0 ) {
                t << "AUDIOFILE ";
                writeDataSource( index-1, t );
                if( readFromStdin() )
                    t << (lastTrack.firstSector() + lastTrack.index0() - offset).toString();
                else
                    t << (lastTrack.index0() - offset).toString();
                t << " "
                  << (lastTrack.length() - lastTrack.index0()).toString()
                  << endl
                  << "START" << endl;
            }
        }

        //
        // The track data
        //
        t << "AUDIOFILE ";
        writeDataSource( index, t );
        if( readFromStdin() )
            t << (track.firstSector() - offset).toString() << " ";
        else
            t << "0 ";
        // no index 0 for the last track. Or should we allow this???
        if( index == m_toc.count()-1 )
            t << track.length().toString();
        else
            t << track.realAudioLength().toString();
        t << endl;
    }
    else {
        if( track.mode() == K3b::Device::Track::XA_FORM1 )
            t << "TRACK MODE2_FORM1" << endl;
        else if( track.mode() == K3b::Device::Track::XA_FORM2 )
            t << "TRACK MODE2_FORM2" << endl;
        else
            t << "TRACK MODE1" << endl;

        if( !m_cdText.isEmpty() && !m_toc.contentType() != K3b::Device::DATA ) {
            //
            // insert fake cdtext
            // cdrdao does not work without it and it seems not to do any harm.
            //
            t << "CD_TEXT {" << endl
              << "  LANGUAGE 0 {" << endl
              << "    TITLE " << "\"\"" << endl
              << "    PERFORMER " << "\"\"" << endl
              << "    ISRC " << "\"\"" << endl
              << "    ARRANGER " << "\"\"" << endl
              << "    SONGWRITER " << "\"\"" << endl
              << "    COMPOSER " << "\"\"" << endl
              << "    MESSAGE " << "\"\"" << endl
              << "  }" << endl
              << "}" << endl;
        }

        if( readFromStdin() )
            t << "DATAFILE \"-\" " << track.length().toString() << endl;
        else
            t << "DATAFILE \"" << m_filenames[index] << "\"" << endl;
        t << endl;
    }

    t << endl;
}


void K3b::TocFileWriter::writeGlobalCdText( QTextStream& t )
{
    t << "CD_TEXT {" << endl;
    t << "  LANGUAGE_MAP { 0: EN }" << endl;
    t << "  LANGUAGE 0 {" << endl;
    t << "    TITLE " << "\"" << m_cdText.title() << "\"" << endl;
    t << "    PERFORMER " << "\"" << m_cdText.performer() << "\"" << endl;
    t << "    DISC_ID " << "\"" << m_cdText.discId() << "\"" << endl;
    t << "    UPC_EAN " << "\"" << m_cdText.upcEan() << "\"" << endl;
    t << endl;
    t << "    ARRANGER " << "\"" << m_cdText.arranger() << "\"" << endl;
    t << "    SONGWRITER " << "\"" << m_cdText.songwriter() << "\"" << endl;
    t << "    COMPOSER " << "\"" << m_cdText.composer() << "\"" << endl;
    t << "    MESSAGE " << "\"" << m_cdText.message() << "\"" << endl;
    t << "  }" << endl;
    t << "}" << endl;
    t << endl;
}


void K3b::TocFileWriter::writeTrackCdText( const K3b::Device::TrackCdText& track, QTextStream& t )
{
    t << "CD_TEXT {" << endl;
    t << "  LANGUAGE 0 {" << endl;
    t << "    TITLE " << "\"" << track.title() << "\"" << endl;
    t << "    PERFORMER " << "\"" << track.performer() << "\"" << endl;
    t << "    ISRC " << "\"" << track.isrc() << "\"" << endl;
    t << "    ARRANGER " << "\"" << track.arranger() << "\"" << endl;
    t << "    SONGWRITER " << "\"" << track.songwriter() << "\"" << endl;
    t << "    COMPOSER " << "\"" << track.composer() << "\"" << endl;
    t << "    MESSAGE " << "\"" << track.message() << "\"" << endl;
    t << "  }" << endl;
    t << "}" << endl;
}


void K3b::TocFileWriter::writeDataSource( int trackIndex, QTextStream& t )
{
    if( readFromStdin() )
        t << "\"-\" ";
    else
        t << "\"" << m_filenames[trackIndex] << "\" ";
}


bool K3b::TocFileWriter::readFromStdin() const
{
    return ( m_toc.count() > m_filenames.count() );
}
