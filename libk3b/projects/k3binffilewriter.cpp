/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3binffilewriter.h"

#include "k3bcdtext.h"
#include "k3btrack.h"
#include "k3bmsf.h"
#include "k3bcore.h"
#include "k3bversion.h"

#include <QDateTime>
#include <QFile>



K3b::InfFileWriter::InfFileWriter()
    : m_index0(-1),
      m_trackNumber(1),
      m_trackStart(0),
      m_trackLength(0),
      m_preEmphasis(false),
      m_copyPermitted(true),
      m_bigEndian(false)
{
}


bool K3b::InfFileWriter::save( const QString& filename )
{
    QFile f( filename );

    if( !f.open( QIODevice::WriteOnly ) ) {
        qDebug() << "(K3b::InfFileWriter) could not open file " << f.fileName();
        return false;
    }

    QTextStream s( &f );

    return save( s );
}


bool K3b::InfFileWriter::save( QTextStream& s )
{
    // now write the inf data
    // ----------------------
    // header
    s << "# Cdrecord-Inf-File written by K3b " << k3bcore->version()
      << ", " << QDateTime::currentDateTime().toString() << Qt::endl
      << "#" << Qt::endl;

    s << "ISRC=\t\t" << m_isrc << Qt::endl;
    s << "MCN=\t\t" << m_mcn << Qt::endl;

    // CD-Text
    s << "Albumperformer=\t" << "'" << m_albumPerformer << "'" << Qt::endl;
    s << "Albumtitle=\t" << "'" << m_albumTitle << "'" << Qt::endl;

    s << "Performer=\t" << "'" << m_trackPerformer << "'" << Qt::endl;
    s << "Songwriter=\t" << "'" << m_trackSongwriter << "'" << Qt::endl;
    s << "Composer=\t" << "'" << m_trackComposer << "'" << Qt::endl;
    s << "Arranger=\t" << "'" << m_trackArranger << "'" << Qt::endl;
    s << "Message=\t" << "'" << m_trackMessage << "'" << Qt::endl;

    s << "Tracktitle=\t" << "'" << m_trackTitle << "'" << Qt::endl;

    s << "Tracknumber=\t" << m_trackNumber << Qt::endl;

    // track start
    s << "Trackstart=\t" << m_trackStart.lba() << Qt::endl;

    // track length
    s << "# Tracklength: " << m_trackLength.toString() << Qt::endl;
    s << "Tracklength=\t" << m_trackLength.totalFrames() << ", 0" << Qt::endl;

    // pre-emphasis
    s << "Pre-emphasis=\t";
    if( m_preEmphasis )
        s << "yes";
    else
        s << "no";
    s << Qt::endl;

    // channels (always 2)
    s << "Channels=\t2" << Qt::endl;

    // copy-permitted
    // TODO: not sure about this!
    //       there are three options: yes, no, once
    //       but using "once" gives the same result as with cdrdao
    //       and that's important.
    s << "Copy_permitted=\t";
    if( m_copyPermitted )
        s << "yes";
    else
        s << "once";
    s << Qt::endl;

    // endianess - wav is little -> onthefly: big, with images: little
    s << "Endianess=\t";
    if( m_bigEndian )
        s << "big";
    else
        s << "little";
    s << Qt::endl;

    // write indices
    // the current tracks' data contains the pregap of the next track
    // if the pregap has length 0 we need no index 0
    if( m_indices.isEmpty() )
        s << "Index=\t\t0" << Qt::endl;
    else {
        for( int i = 0; i < m_indices.count(); ++i )
            s << "Index=\t\t" << m_indices[i] << Qt::endl;
    }

    s << "Index0=\t\t" << m_index0 << Qt::endl;

    return ( s.status() == QTextStream::Ok );
}


void K3b::InfFileWriter::setTrack( const K3b::Device::Track& track )
{
    m_indices.clear();

    // the first index always has to be a zero (cdrecord manpage)
    m_indices.append( 0 );

    QList<K3b::Msf> indexList = track.indices();
    for( int i = 0; i < indexList.count(); ++i )
        m_indices.append( indexList[i].lba() );

    if( track.index0() > 0 )
        m_index0 = track.index0().lba();
    else
        m_index0 = -1;

    setPreEmphasis( track.preEmphasis() );
    setCopyPermitted( track.copyPermitted() );

    setTrackStart( track.firstSector() );
    setTrackLength( track.length() );

    setIsrc( track.isrc() );

    setBigEndian( true );
}


void K3b::InfFileWriter::addIndex( long i )
{
    m_indices.append( i );
}


void K3b::InfFileWriter::setTrackCdText( const K3b::Device::TrackCdText& cdtext )
{
    setTrackTitle( cdtext.title() );
    setTrackPerformer( cdtext.performer() );
    setTrackSongwriter( cdtext.songwriter() );
    setTrackComposer( cdtext.composer() );
    setTrackArranger( cdtext.arranger() );
    setTrackMessage( cdtext.message() );
}


void K3b::InfFileWriter::setCdText( const K3b::Device::CdText& cdtext )
{
    setAlbumTitle( cdtext.title() );
    setAlbumPerformer( cdtext.performer() );
}
