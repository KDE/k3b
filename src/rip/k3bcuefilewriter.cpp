/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bcuefilewriter.h"

#include "k3btrack.h"
#include "k3bmsf.h"
#include "k3bcore.h"
#include "k3bversion.h"

#include <QDateTime>
#include <QFile>


K3b::CueFileWriter::CueFileWriter()
{
}


bool K3b::CueFileWriter::save( const QString& filename )
{
    QFile f( filename );

    if( !f.open( QIODevice::WriteOnly ) ) {
        qDebug() << "(K3b::CueFileWriter) could not open file " << f.fileName();
        return false;
    }

    QTextStream s( &f );

    return save( s );
}


bool K3b::CueFileWriter::save( QTextStream& t )
{
    t << "REM Cue file written by K3b " << k3bcore->version() << Qt::endl
      << Qt::endl;

    if( !m_cdText.isEmpty() ) {
        t << "PERFORMER \"" << m_cdText.performer() << "\"" << Qt::endl;
        t << "TITLE \"" << m_cdText.title() << "\"" << Qt::endl;
    }

    t << "FILE \"" << m_image << "\" " << m_dataType.toUpper() << Qt::endl;

    // the tracks
    int i = 0;
    for( K3b::Device::Toc::const_iterator it = m_toc.constBegin();
         it != m_toc.constEnd(); ++it ) {

        const K3b::Device::Track& track = *it;

        t << "  TRACK " << QString::number(i+1).rightJustified( 2, '0' ) << " AUDIO" << Qt::endl;

        if( m_cdText.count() > i && !m_cdText[i].isEmpty() ) {
            t << "    PERFORMER \"" << m_cdText[i].performer() << "\"" << Qt::endl;
            t << "    TITLE \"" << m_cdText[i].title() << "\"" << Qt::endl;
        }

        //
        // the pregap is part of the current track like in toc files
        // and not part of the last track as on the CD
        //
        if( i > 0 ) {
            --it;
            if( (*it).index0() > 0 )
                t << "    INDEX 00 " << ((*it).firstSector() + (*it).index0()).toString() << Qt::endl;
            ++it;
        }
        t << "    INDEX 01 " << track.firstSector().toString() << Qt::endl;
        // TODO: add additional indices

        i++;
    }

    return ( t.status() == QTextStream::Ok );
}
