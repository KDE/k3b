/*
*
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
* Copyright (C) 2009 Sebastian Trueg <trueg@k3b.org>
* Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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

#include "k3bvideocdinfo.h"
#include "k3bcore.h"
#include "k3bprocess.h"
#include "k3bexternalbinmanager.h"

#include <KConfig>
#include <KLocalizedString>

#include <QDebug>
#include <QDomDocument>
#include <QDomElement>


K3b::VideoCdInfo::VideoCdInfo( QObject* parent )
    : QObject( parent )
{
    m_process = 0L;
    m_isXml = false;
}


K3b::VideoCdInfo::~VideoCdInfo()
{
    delete m_process;
}

void K3b::VideoCdInfo::cancelAll()
{
    if ( m_process->isRunning() ) {
        m_process->disconnect( this );
        m_process->kill();
    }
}

void K3b::VideoCdInfo::info( const QString& device )
{
    if ( !k3bcore ->externalBinManager() ->foundBin( "vcdxrip" ) ) {
        qDebug() << "(K3b::VideoCdInfo::info) could not find vcdxrip executable";
        emit infoFinished( false );
        return ;
    }

    delete m_process;
    m_process = new K3b::Process();

    *m_process << k3bcore ->externalBinManager() ->binPath( "vcdxrip" );

    *m_process << "-q" << "--norip" << "-i" << device << "-o" << "-";

    m_process->setSplitStdout( true );
    m_process->setSuppressEmptyLines( false );
    connect( m_process, SIGNAL(stderrLine(QString)),
             this, SLOT(slotParseOutput(QString)) );
    connect( m_process, SIGNAL(stdoutLine(QString)),
             this, SLOT(slotParseOutput(QString)) );
    connect( m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
             this, SLOT(slotInfoFinished(int,QProcess::ExitStatus)) );

    if ( !m_process->start( KProcess::SeparateChannels ) ) {
        qDebug() << "(K3b::VideoCdInfo::info) could not start vcdxrip";
        cancelAll();
        emit infoFinished( false );
    }
}

void K3b::VideoCdInfo::slotParseOutput( const QString& inp )
{
    if ( inp.contains( "<?xml" ) )
        m_isXml = true;

    if ( m_isXml )
        m_xmlData += inp;
    else
        qDebug() << "(K3b::VideoCdInfo::slotParseOutput) " << inp;

    if ( inp.contains( "</videocd>" ) )
        m_isXml = false;
}

void K3b::VideoCdInfo::slotInfoFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
    if ( exitStatus == QProcess::NormalExit ) {
        // TODO: check the process' exitStatus()
        switch ( exitCode ) {
            case 0:
                break;
            default:
                cancelAll();
                qDebug() << "vcdxrip finished with exit code" << exitCode;
                emit infoFinished( false );
                return ;
        }
    } else {
        cancelAll();
        qDebug() << "vcdxrip crashed!";
        emit infoFinished( false );
        return ;
    }

    if ( m_xmlData.isEmpty() ) {
        qDebug() << "XML data empty!";
        emit infoFinished( false );
        return ;
    }

    parseXmlData();
    emit infoFinished( true );
}

void K3b::VideoCdInfo::parseXmlData()
{
    QDomDocument xml_doc;
    QDomElement xml_root;

    m_Result.xmlData = m_xmlData;

    xml_doc.setContent( m_xmlData );
    xml_root = xml_doc.documentElement();

    m_Result.type = xml_root.attribute( "class" );
    m_Result.version = xml_root.attribute( "version" );

    for ( QDomNode node = xml_root.firstChild(); !node.isNull(); node = node.nextSibling() ) {
        QDomElement el = node.toElement();
        QString tagName = el.tagName().toLower();

        if ( tagName == "pvd" ) {
            for ( QDomNode snode = node.firstChild(); !snode.isNull(); snode = snode.nextSibling() ) {
                QDomElement sel = snode.toElement();
                QString pvdElement = sel.tagName().toLower();
                QString pvdElementText = sel.text();
                if ( pvdElement == "volume-id" )
                    m_Result.volumeId = pvdElementText;
            }

        } else if ( tagName == "sequence-items" ) {
            for ( QDomNode snode = node.firstChild(); !snode.isNull(); snode = snode.nextSibling() ) {
                QDomElement sel = snode.toElement();
                m_Result.addEntry( K3b::VideoCdInfoResultEntry(
                                       sel.attribute( "src" ),
                                       sel.attribute( "id" ) ),
                                   K3b::VideoCdInfoResult::SEQUENCE
                                 );
            }
        } else if ( tagName == "segment-items" ) {
            for ( QDomNode snode = node.firstChild(); !snode.isNull(); snode = snode.nextSibling() ) {
                QDomElement sel = snode.toElement();
                m_Result.addEntry( K3b::VideoCdInfoResultEntry(
                                       sel.attribute( "src" ),
                                       sel.attribute( "id" ) ),
                                   K3b::VideoCdInfoResult::SEGMENT
                                 );
            }
        } else {
            qDebug() << QString( "(K3b::VideoCdInfo::parseXmlData) tagName '%1' not used" ).arg( tagName );
        }
    }
}

const K3b::VideoCdInfoResult& K3b::VideoCdInfo::result() const
{
    return m_Result;
}

const K3b::VideoCdInfoResultEntry& K3b::VideoCdInfoResult::entry( int number, int type ) const
{
    switch ( type ) {
        case K3b::VideoCdInfoResult::FILE:
            if ( number >= m_fileEntry.count() )
                return m_emptyEntry;
            return m_fileEntry[ number ];
        case K3b::VideoCdInfoResult::SEGMENT:
            if ( number >= m_segmentEntry.count() )
                return m_emptyEntry;
            return m_segmentEntry[ number ];
        case K3b::VideoCdInfoResult::SEQUENCE:
            if ( number >= m_sequenceEntry.count() )
                return m_emptyEntry;
            return m_sequenceEntry[ number ];
        default:
            qDebug() << "(K3b::VideoCdInfoResult::entry) not supported entrytype.";
    }

    return m_emptyEntry;

}


void K3b::VideoCdInfoResult::addEntry( const K3b::VideoCdInfoResultEntry& entry, int type )
{
    switch ( type ) {
        case K3b::VideoCdInfoResult::FILE:
            m_fileEntry.append( entry );
            break;
        case K3b::VideoCdInfoResult::SEGMENT:
            m_segmentEntry.append( entry );
            break;
        case K3b::VideoCdInfoResult::SEQUENCE:
            m_sequenceEntry.append( entry );
            break;
        default:
            qDebug() << "(K3b::VideoCdInfoResult::addEntry) not supported entrytype.";
    }
}

int K3b::VideoCdInfoResult::foundEntries( int type ) const
{
    switch ( type ) {
        case K3b::VideoCdInfoResult::FILE:
            return m_fileEntry.count();
        case K3b::VideoCdInfoResult::SEGMENT:
            return m_segmentEntry.count();
        case K3b::VideoCdInfoResult::SEQUENCE:
            return m_sequenceEntry.count();
        default:
            qDebug() << "(K3b::VideoCdInfoResult::addEntry) not supported entrytype.";
    }
    return 0;
}



