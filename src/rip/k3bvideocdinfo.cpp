/*
 *
 * $Id: $
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



#include <qstring.h>
#include <qvaluelist.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <qdom.h>

#include <klocale.h>
#include <kconfig.h>
#include <kdebug.h>

#include "k3bvideocdinfo.h"

#include <k3bprocess.h>
#include <k3bexternalbinmanager.h>


K3bVideoCdInfo::K3bVideoCdInfo( QObject* parent, const char* name )
  : QObject( parent, name )
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
        kdDebug() << "(K3bVideoCd::info) could not find vcdxrip executable" << endl;
        emit infoFinished( false );
        return ;
    }

    delete m_process;
    m_process = new K3bProcess();
    
    *m_process << k3bcore ->externalBinManager() ->binPath( "vcdxrip" );

    *m_process << "-q" << "--norip" << "-C" << device << "-o" << "-";

    connect( m_process, SIGNAL( receivedStderr( KProcess*, char*, int ) ),
             this, SLOT( slotParseOutput( KProcess*, char*, int ) ) );
    connect( m_process, SIGNAL( receivedStdout( KProcess*, char*, int ) ),
             this, SLOT( slotParseOutput( KProcess*, char*, int ) ) );
    connect( m_process, SIGNAL( processExited( KProcess* ) ),
             this, SLOT( slotInfoFinished() ) );

    if ( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
        kdDebug() << "(K3bVideoCd::info) could not start vcdxrip" << endl;
        cancelAll();
        emit infoFinished( false );
    }
}

void K3bVideoCdInfo::slotParseOutput( KProcess*, char* output, int len )
{
    QString buffer = QString::fromLocal8Bit( output, len );
            
    // split to lines
    QStringList lines = QStringList::split( "\n", buffer );

    for ( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ ) {
        // kdDebug() << "slotParseOutput: Lines: " << *str << endl;

        if ( ( *str ).contains( "<?xml" ) )
            m_isXml = true;

        if ( m_isXml )
            m_xmlData += *str;
        else
            kdDebug() << "(K3bVideoCdInfo::slotParseOutput) " << *str << endl;

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
       return;
   }
   
   parseXmlData();
   emit infoFinished( true );
}

void K3bVideoCdInfo::parseXmlData()
{
    QDomDocument xml_doc;
    QDomElement xml_root;

    xml_doc.setContent( m_xmlData );
    xml_root = xml_doc.documentElement();

    m_Result.type = xml_root.attribute( "class" );
    m_Result.version = xml_root.attribute( "version" );

    for ( QDomNode node = xml_root.firstChild(); !node.isNull(); node = node.nextSibling() ) {
        QDomElement el = node.toElement();
        QString tagName = el.tagName().lower();

        if ( tagName == "pvd" ) {
                    for ( QDomNode snode = node.firstChild(); !snode.isNull(); snode = snode.nextSibling() ) {
                        QDomElement sel = snode.toElement();
                        QString pvdElement = sel.tagName().lower();
                        QString pvdElementText = sel.text();                        
                        if ( pvdElement == "volume-id" )
                            m_Result.volumeId = pvdElementText;
                    }

        }
        else if ( tagName == "sequence-items" ) {
                    for ( QDomNode snode = node.firstChild(); !snode.isNull(); snode = snode.nextSibling() ) {
                        QDomElement sel = snode.toElement();
                        QString seqElement = sel.tagName().lower();
                        m_Result.sequence << sel.attribute( "src" );
                        m_Result.sequenceId << sel.attribute( "id" );
                    }
        }
        else if ( tagName == "segment-items" ) {
                    for ( QDomNode snode = node.firstChild(); !snode.isNull(); snode = snode.nextSibling() ) {
                        QDomElement sel = snode.toElement();
                        QString seqElement = sel.tagName().lower();
                        m_Result.sequence << sel.attribute( "src" );
                        m_Result.sequenceId << sel.attribute( "id" );
                    }
        }
        else if ( tagName == "sequence-items" ) {
                    for ( QDomNode snode = node.firstChild(); !snode.isNull(); snode = snode.nextSibling() ) {
                        QDomElement sel = snode.toElement();
                        QString seqElement = sel.tagName().lower();
                        m_Result.sequence << sel.attribute( "src" );
                        m_Result.sequenceId << sel.attribute( "id" );
                    }
        }
        else {
                kdDebug() << QString("(K3bVideoCd::parseXmlData) tagName '%1' not used").arg( tagName ) << endl;
        }
    }
}

const K3bVideoCdInfoResult& K3bVideoCdInfo::result() const
{
  return m_Result;
}

#include "k3bvideocdinfo.moc"

