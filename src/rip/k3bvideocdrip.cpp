/*
*
* $Id$
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
*
* This file is part of the K3b project.
* Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* See the file "COPYING" for the exact licensing terms.
*/

#include <kconfig.h>
#include <kdebug.h>
#include <kio/global.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <ktempfile.h>
#include <kurl.h>

#include <qdatetime.h>
#include <qdom.h>
#include <qfile.h>
#include <qstring.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qurl.h>

// K3b Includes
#include "k3bvideocdrip.h"
#include <k3bcore.h>
#include <k3bexternalbinmanager.h>
#include <k3bglobals.h>
#include <k3bprocess.h>

K3bVideoCdRip::K3bVideoCdRip( QObject* parent, const char* name )
        : K3bJob( parent, name ),
        m_ripsourceType( 0 ),
        m_canceled( false ),
        m_process( 0 )
{}


K3bVideoCdRip::~K3bVideoCdRip()
{
    if ( m_process )
        delete m_process;
}


void K3bVideoCdRip::cancel()
{
    cancelAll();

    emit infoMessage( i18n( "Job canceled by user." ), K3bJob::ERROR );
    emit canceled();
    emit finished( false );
}


void K3bVideoCdRip::cancelAll()
{
    m_canceled = true;

    if ( m_process->isRunning() ) {
        m_process->disconnect( this );
        m_process->kill();
    }
}


void K3bVideoCdRip::start()
{
    kdDebug() << "(K3bVideoCdRip) starting job" << endl;

    emit started();
    m_canceled = false;

    vcdxRip();
}

void K3bVideoCdRip::vcdxRip()
{
    emit newTask( i18n( "Check files" ) );

    m_stage = stageUnknown;
    delete m_process;
    m_process = new K3bProcess();

    const K3bExternalBin* bin = k3bcore ->externalBinManager() ->binObject( "vcdxrip" );

    if ( !bin ) {
        kdDebug() << "(K3bVideoCdRip) could not find vcdxrip executable" << endl;
        emit infoMessage( i18n( "Could not find %1 executable." ).arg( "vcdxrip" ), K3bJob::ERROR );
        emit infoMessage( i18n( "To rip VideoCD's you must install VcdImager Version %1." ).arg( ">= 0.7.12" ), K3bJob::INFO );
        emit infoMessage( i18n( "You can find this on your distribution disks or download it from http://www.vcdimager.org" ), K3bJob::INFO );
        cancelAll();
        emit finished( false );
        return ;
    }

    if ( !bin->copyright.isEmpty() )
        emit infoMessage( i18n( "Using %1 %2 - Copyright (C) %3" ).arg( bin->name() ).arg( bin->version ).arg( bin->copyright ), INFO );


    *m_process << k3bcore ->externalBinManager() ->binPath( "vcdxrip" );

    // additional user parameters from config
    const QStringList& params = k3bcore->externalBinManager() ->program( "vcdxrip" ) ->userParameters();
    for ( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
        *m_process << *it;

    *m_process << "--gui" << "--progress" << "-i" << m_ripsource << "-o" << "/dev/null";

    connect( m_process, SIGNAL( receivedStderr( KProcess*, char*, int ) ),
             this, SLOT( slotParseVcdXRipOutput( KProcess*, char*, int ) ) );
    connect( m_process, SIGNAL( receivedStdout( KProcess*, char*, int ) ),
             this, SLOT( slotParseVcdXRipOutput( KProcess*, char*, int ) ) );
    connect( m_process, SIGNAL( processExited( KProcess* ) ),
             this, SLOT( slotVcdXRipFinished() ) );

    m_process->setWorkingDirectory( QUrl( m_destPath ).dirPath() );

    // vcdxrip comandline parameters
    kdDebug() << "***** vcdxrip parameters:" << endl;
    ;
    const QValueList<QCString>& args = m_process->args();
    QString s;
    for ( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
        s += *it + " ";
    }
    kdDebug() << s << flush << endl;
    emit debuggingOutput( "vcdxrip command:", s );

    emit newTask( i18n( "Extracting" ) );
    emit infoMessage( i18n( "Start extracting." ), K3bJob::INFO );
    emit infoMessage( i18n( "Extract files from %1 to %2." ).arg( m_ripsource ).arg( m_destPath ), K3bJob::INFO );
            
    if ( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
        kdDebug() << "(K3bVideoCdRip) could not start vcdxrip" << endl;
        emit infoMessage( i18n( "Could not start %1." ).arg( "vcdxrip" ), K3bJob::ERROR );
        cancelAll();
        emit finished( false );
    }
}

void K3bVideoCdRip::slotParseVcdXRipOutput( KProcess*, char* output, int len )
{
    QString buffer = QString::fromLocal8Bit( output, len );

    // split to lines
    QStringList lines = QStringList::split( "\n", buffer );

    QDomDocument xml_doc;
    QDomElement xml_root;

    // do every line
    for ( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ ) {
        *str = ( *str ).stripWhiteSpace();

        emit debuggingOutput( "vcdxrip", *str );

        xml_doc.setContent( QString( "<?xml version='1.0'?><vcdxrip>" ) + *str + "</vcdxrip>" );

        xml_root = xml_doc.documentElement();

        // There should be only one... but ...
        for ( QDomNode node = xml_root.firstChild(); !node.isNull(); node = node.nextSibling() ) {
            QDomElement el = node.toElement();
            if ( el.isNull() )
                continue;

            const QString tagName = el.tagName().lower();

            if ( tagName == "progress" ) {
                const QString oper = el.attribute( "operation" ).lower();
                const long long overallPos = el.attribute( "position" ).toLong();
                const long long pos = overallPos - m_subPosition;
                const long long size = el.attribute( "size" ).toLong() - m_subPosition;

                if ( oper == "extract" ) {
                    emit subPercent( ( int ) ( 100.0 * ( double ) pos / ( double ) size ) );
                    emit processedSubSize( ( pos * 2352 ) / 1024 / 1024 , ( size * 2352 ) / 1024 / 1024 );

                    m_bytesFinished = pos;

                    double relOverallWritten = ( ( double ) overallPos ) / ( double ) m_videocdsize ;
                    emit percent( ( int ) ( 100 * relOverallWritten ) );

                    kdDebug() << QString( "(K3bVideoCdRip::slotParseVcdXRipOutput) overallPos = %1, relOverallWritten = %2, videolen = " ).arg( overallPos ).arg( relOverallWritten ) << m_videocdsize << endl;

                } else {
                    return ;
                }

            } else if ( tagName == "log" ) {
                QDomText tel = el.firstChild().toText();
                const QString level = el.attribute( "level" ).lower();
                if ( tel.isText() ) {
                    const QString text = tel.data();
                    if ( level == "information" ) {
                        kdDebug() << QString( "(K3bVideoCdRip) vcdxrip information, %1" ).arg( text ) << endl;
                        parseInformation( text );
                    } else {
                        if ( level != "error" ) {
                            kdDebug() << QString( "(K3bVideoCdRip) vcdxrip warning, %1" ).arg( text ) << endl;
                            emit debuggingOutput( "vcdxrip", text );
                        } else {
                            kdDebug() << QString( "(K3bVideoCdRip) vcdxrip error, %1" ).arg( text ) << endl;
                            emit infoMessage( text, K3bJob::ERROR );
                        }
                    }
                }
            }
        }
    }
}


void K3bVideoCdRip::slotVcdXRipFinished()
{
    if ( m_process->normalExit() ) {
        // TODO: check the process' exitStatus()
        switch ( m_process->exitStatus() ) {
            case 0:
                emit infoMessage( i18n( "Files successfully extracted." ), K3bJob::STATUS );
                break;
            default:
                emit infoMessage( i18n( "%1 returned an unknown error (code %2)." ).arg( "vcdxrip" ).arg( m_process->exitStatus() ), K3bJob::ERROR );
                emit infoMessage( strerror( m_process->exitStatus() ), K3bJob::ERROR );
                emit infoMessage( i18n( "Please send me an email with the last output..." ), K3bJob::ERROR );
                cancelAll();
                emit finished( false );
                return ;
        }
    } else {
        emit infoMessage( i18n( "%1 did not exit cleanly." ).arg( "Vcdxrip" ), K3bJob::ERROR );
        cancelAll();
        emit finished( false );
        return ;
    }

    emit finished( true );
}

void K3bVideoCdRip::parseInformation( QString text )
{
    // parse extra info
    if ( text.contains( "detected extended VCD2.0 PBC files" ) )
        emit infoMessage( i18n( "detected extended VCD2.0 PBC files" ), K3bJob::INFO );

    // parse startposition and extracting sequence info
    // extracting avseq05.mpg... (start lsn 32603 (+28514))
    else if ( text.startsWith( "extracting" ) ) {
        if ( text.contains( "(start lsn" ) ) {
            int index = text.find( "(start lsn" );
            int end = text.find( " (+" );
            if ( end > 0) {
                m_subPosition = text.mid( index + 11, end - index - 11 ).stripWhiteSpace().toLong();
            }
            else {
                // found segment here we can get only the start lsn :)
                // extracting item0001.mpg... (start lsn 225, 1 segments)
                int end = text.find(  ",", index );
                int overallPos = text.mid( index + 11, end - index - 11 ).stripWhiteSpace().toLong();
                double relOverallWritten = ( ( double ) overallPos ) / ( double ) m_videocdsize ;
                emit percent( ( int ) ( 100 * relOverallWritten ) );
            }


            index = 11;
            end = text.find( "(start lsn" );
            emit newSubTask( i18n( "Extracting %1" ).arg( text.mid( index, end - index ).stripWhiteSpace() ) );
        }
        // parse extracting files info
        // extracting CDI/CDI_IMAG.RTF to _cdi_cdi_imag.rtf (lsn 258, size 1315168, raw 1)
        else if ( text.contains( "(lsn" ) && text.contains( "size" ) ) {
            int index = 11;
            int end = text.find( "to" );
            QString extractFileName = text.mid( index, end - index ).stripWhiteSpace();
            index = text.find( " to " );
            end = text.find( " (lsn" );
            QString toFileName = text.mid( index + 4, end - index - 4 ).stripWhiteSpace();
            emit newSubTask( i18n( "Extracting %1 to %2" ).arg( extractFileName ).arg( toFileName ) );
        }
    }

}

#include "k3bvideocdrip.moc"
