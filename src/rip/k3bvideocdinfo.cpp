/*
*
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
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



#include <qstring.h>
#include <q3valuelist.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <qdom.h>

#include <klocale.h>
#include <kconfig.h>
#include <kdebug.h>

#include "k3bvideocdinfo.h"

#include <k3bprocess.h>
#include <k3bexternalbinmanager.h>


K3bVideoCdInfo::K3bVideoCdInfo( QObject* parent )
        : QObject( parent )
{
    m_process = 0L;
    m_isXml = false;
}


K3bVideoCdInfo::~K3bVideoCdInfo()
{
    delete m_process;
}

void K3bVideoCdInfo::cancelAll()
{
    if ( m_process->isRunning() ) {
        m_process->disconnect( this );
        m_process->kill();
    }
}

void K3bVideoCdInfo::info( const QString& device )
{
    if ( !k3bcore ->externalBinManager() ->foundBin( "vcdxrip" ) ) {
        kDebug() << "(K3bVideoCdInfo::info) could not find vcdxrip executable";
        emit infoFinished( false );
        return ;
    }

    delete m_process;
    m_process = new K3bProcess();

    *m_process << k3bcore ->externalBinManager() ->binPath( "vcdxrip" );

    *m_process << "-q" << "--norip" << "-i" << device << "-o" << "-";

    connect( m_process, SIGNAL( receivedStderr( K3Process*, char*, int ) ),
             this, SLOT( slotParseOutput( K3Process*, char*, int ) ) );
    connect( m_process, SIGNAL( receivedStdout( K3Process*, char*, int ) ),
             this, SLOT( slotParseOutput( K3Process*, char*, int ) ) );
    connect( m_process, SIGNAL( processExited( K3Process* ) ),
             this, SLOT( slotInfoFinished() ) );

    if ( !m_process->start( K3Process::NotifyOnExit, K3Process::AllOutput ) ) {
        kDebug() << "(K3bVideoCdInfo::info) could not start vcdxrip";
        cancelAll();
        emit infoFinished( false );
    }
}

void K3bVideoCdInfo::slotParseOutput( K3Process*, char* output, int len )
{
    QString buffer = QString::fromLocal8Bit( output, len );

    // split to lines
    QStringList lines = buffer.split( '\n' );
    QStringList::Iterator end( lines.end());
    for ( QStringList::Iterator str = lines.begin(); str != end; ++str ) {

        if ( ( *str ).contains( "<?xml" ) )
            m_isXml = true;

        if ( m_isXml )
            m_xmlData += *str;
        else
            kDebug() << "(K3bVideoCdInfo::slotParseOutput) " << *str;

        if ( ( *str ).contains( "</videocd>" ) )
            m_isXml = false;
    }
}

void K3bVideoCdInfo::slotInfoFinished()
{
    if ( m_process->normalExit() ) {
        // TODO: check the process' exitStatus()
        switch ( m_process->exitStatus() ) {
            case 0:
                break;
            default:
                cancelAll();
                emit infoFinished( false );
                return ;
        }
    } else {
        cancelAll();
        emit infoFinished( false );
        return ;
    }

    if ( m_xmlData.isEmpty() ) {
        emit infoFinished( false );
        return ;
    }

    parseXmlData();
    emit infoFinished( true );
}

void K3bVideoCdInfo::parseXmlData()
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
                QString seqElement = sel.tagName().toLower();
                m_Result.addEntry( K3bVideoCdInfoResultEntry(
                                       sel.attribute( "src" ),
                                       sel.attribute( "id" ) ),
                                   K3bVideoCdInfoResult::SEQUENCE
                                 );
            }
        } else if ( tagName == "segment-items" ) {
            for ( QDomNode snode = node.firstChild(); !snode.isNull(); snode = snode.nextSibling() ) {
                QDomElement sel = snode.toElement();
                QString seqElement = sel.tagName().toLower();
                m_Result.addEntry( K3bVideoCdInfoResultEntry(
                                       sel.attribute( "src" ),
                                       sel.attribute( "id" ) ),
                                   K3bVideoCdInfoResult::SEGMENT
                                 );
            }
        } else {
            kDebug() << QString( "(K3bVideoCdInfo::parseXmlData) tagName '%1' not used" ).arg( tagName );
        }
    }
}

const K3bVideoCdInfoResult& K3bVideoCdInfo::result() const
{
    return m_Result;
}

const K3bVideoCdInfoResultEntry& K3bVideoCdInfoResult::entry( int number, int type ) const
{
    switch ( type ) {
        case K3bVideoCdInfoResult::FILE:
            if ( number >= m_fileEntry.count() )
                return m_emptyEntry;
            return m_fileEntry[ number ];
        case K3bVideoCdInfoResult::SEGMENT:
            if ( number >= m_segmentEntry.count() )
                return m_emptyEntry;
            return m_segmentEntry[ number ];
        case K3bVideoCdInfoResult::SEQUENCE:
            if ( number >= m_sequenceEntry.count() )
                return m_emptyEntry;
            return m_sequenceEntry[ number ];
        default:
            kDebug() << "(K3bVideoCdInfoResult::entry) not supported entrytype.";
    }

    return m_emptyEntry;

}


void K3bVideoCdInfoResult::addEntry( const K3bVideoCdInfoResultEntry& entry, int type )
{
    switch ( type ) {
        case K3bVideoCdInfoResult::FILE:
            m_fileEntry.append( entry );
            break;
        case K3bVideoCdInfoResult::SEGMENT:
            m_segmentEntry.append( entry );
            break;
        case K3bVideoCdInfoResult::SEQUENCE:
            m_sequenceEntry.append( entry );
            break;
        default:
            kDebug() << "(K3bVideoCdInfoResult::addEntry) not supported entrytype.";
    }
}

int K3bVideoCdInfoResult::foundEntries( int type ) const
{
    switch ( type ) {
        case K3bVideoCdInfoResult::FILE:
            return m_fileEntry.count();
        case K3bVideoCdInfoResult::SEGMENT:
            return m_segmentEntry.count();
        case K3bVideoCdInfoResult::SEQUENCE:
            return m_sequenceEntry.count();
        default:
            kDebug() << "(K3bVideoCdInfoResult::addEntry) not supported entrytype.";
    }
    return 0;
}

#include "k3bvideocdinfo.moc"

