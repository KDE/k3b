/*
*
* $Id$
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
*             THX to Manfred Odenstein <odix@chello.at>
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

#include <qfile.h>

#include <kstandarddirs.h>
#include <kdebug.h>


#include "k3bvcdxmlview.h"
#include "k3bvcdtrack.h"
#include <k3bcore.h>
#include <k3bversion.h>

K3bVcdXmlView::K3bVcdXmlView( K3bVcdDoc* pDoc )
{

    m_doc = pDoc;

}

K3bVcdXmlView::~K3bVcdXmlView()
{}


bool K3bVcdXmlView::write( const QString& fname )
{

    QDomDocument xmlDoc( "videocd PUBLIC \"-//GNU//DTD VideoCD//EN\" \"http://www.gnu.org/software/vcdimager/videocd.dtd\"" );
    // xmlDoc.appendChild( xmlDoc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"iso-8859-1\"" ) );
    xmlDoc.appendChild( xmlDoc.createProcessingInstruction( "xml", "version=\"1.0\"" ) );

    // create root element
    QDomElement root = xmlDoc.createElement( "videocd" );
    root.setAttribute( "xmlns", "http://www.gnu.org/software/vcdimager/1.0/" );
    root.setAttribute( "class", m_doc->vcdOptions() ->vcdClass() );
    root.setAttribute( "version", m_doc->vcdOptions() ->vcdVersion() );
    xmlDoc.appendChild( root );

    // create option elements

    // Broken SVCD mode - NonCompliantMode
    if ( m_doc->vcdOptions() ->NonCompliantMode() ) {
        QDomElement elemOption;
        elemOption = addSubElement( xmlDoc, root, "option" );
        elemOption.setAttribute( "name", "svcd vcd30 mpegav" );
        elemOption.setAttribute( "value", "true" );

        elemOption = addSubElement( xmlDoc, root, "option" );
        elemOption.setAttribute( "name", "svcd vcd30 entrysvd" );
        elemOption.setAttribute( "value", "true" );
    }

    // Relaxed aps
    if ( m_doc->vcdOptions() ->RelaxedAps() ) {
        QDomElement elemOption;
        elemOption = addSubElement( xmlDoc, root, "option" );
        elemOption.setAttribute( "name", "relaxed aps" );
        elemOption.setAttribute( "value", "true" );
    }

    // Update scan offsets
    if ( m_doc->vcdOptions() ->UpdateScanOffsets() ) {
        QDomElement elemOption;
        elemOption = addSubElement( xmlDoc, root, "option" );
        elemOption.setAttribute( "name", "update scan offsets" );
        elemOption.setAttribute( "value", "true" );

    }

    // Gaps & Margins
    if ( m_doc->vcdOptions() ->UseGaps() ) {
        QDomElement elemOption;
        elemOption = addSubElement( xmlDoc, root, "option" );
        elemOption.setAttribute( "name", "leadout pregap" );
        elemOption.setAttribute( "value", m_doc->vcdOptions() ->PreGapLeadout() );

        elemOption = addSubElement( xmlDoc, root, "option" );
        elemOption.setAttribute( "name", "track pregap" );
        elemOption.setAttribute( "value", m_doc->vcdOptions() ->PreGapTrack() );

        if ( m_doc->vcdOptions() ->vcdClass() == "vcd" ) {
            elemOption = addSubElement( xmlDoc, root, "option" );
            elemOption.setAttribute( "name", "track front margin" );
            elemOption.setAttribute( "value", m_doc->vcdOptions() ->FrontMarginTrack() );

            elemOption = addSubElement( xmlDoc, root, "option" );
            elemOption.setAttribute( "name", "track rear margin" );
            elemOption.setAttribute( "value", m_doc->vcdOptions() ->RearMarginTrack() );
        } else {
            elemOption = addSubElement( xmlDoc, root, "option" );
            elemOption.setAttribute( "name", "track front margin" );
            elemOption.setAttribute( "value", m_doc->vcdOptions() ->FrontMarginTrackSVCD() );

            elemOption = addSubElement( xmlDoc, root, "option" );
            elemOption.setAttribute( "name", "track rear margin" );
            elemOption.setAttribute( "value", m_doc->vcdOptions() ->RearMarginTrackSVCD() );
        }

    }

    // create info element
    QDomElement elemInfo = addSubElement( xmlDoc, root, "info" );
    addSubElement( xmlDoc, elemInfo, "album-id", m_doc->vcdOptions() ->albumId().upper() );
    addSubElement( xmlDoc, elemInfo, "volume-count", m_doc->vcdOptions() ->volumeCount() );
    addSubElement( xmlDoc, elemInfo, "volume-number", m_doc->vcdOptions() ->volumeNumber() );
    addSubElement( xmlDoc, elemInfo, "restriction", m_doc->vcdOptions() ->Restriction() );

    // create pvd element
    QDomElement elemPvd = addSubElement( xmlDoc, root, "pvd" );
    addSubElement( xmlDoc, elemPvd, "volume-id", m_doc->vcdOptions() ->volumeId().upper() );
    addSubElement( xmlDoc, elemPvd, "system-id", m_doc->vcdOptions() ->systemId() );
    addSubElement( xmlDoc, elemPvd, "application-id", m_doc->vcdOptions() ->applicationId() );
    addSubElement( xmlDoc, elemPvd, "preparer-id", QString( "K3b - Version %1" ).arg( k3bcore->version() ).upper() );
    addSubElement( xmlDoc, elemPvd, "publisher-id", m_doc->vcdOptions() ->publisher().upper() );


    // create filesystem element
    QDomElement elemFileSystem = addSubElement( xmlDoc, root, "filesystem" );

    // SEGMENT folder, some standalone DVD-Player need this
    if ( m_doc->vcdOptions() ->SegmentFolder() )
        addFolderElement( xmlDoc, elemFileSystem, "SEGMENT" );

    // create cdi element
    if ( m_doc->vcdOptions() ->CdiSupport() ) {
        QDomElement elemFolder = addFolderElement( xmlDoc, elemFileSystem, "CDI" );

        addFileElement( xmlDoc, elemFolder, locate( "data", "k3b/cdi/cdi_imag.rtf" ), "CDI_IMAG.RTF", true );
        addFileElement( xmlDoc, elemFolder, locate( "data", "k3b/cdi/cdi_text.fnt" ), "CDI_TEXT.FNT" );
        addFileElement( xmlDoc, elemFolder, locate( "data", "k3b/cdi/cdi_vcd.app" ), "CDI_VCD.APP" );

        QString usercdicfg = locateLocal( "appdata", "cdi/cdi_vcd.cfg" );
        if ( QFile::exists( usercdicfg ) )
            addFileElement( xmlDoc, elemFolder, usercdicfg, "CDI_VCD.CFG" );
        else
            addFileElement( xmlDoc, elemFolder, locate( "data", "k3b/cdi/cdi_vcd.cfg" ), "CDI_VCD.CFG" );
    }

    // sequence-items element & segment-items element
    QDomElement elemsequenceItems;
    QDomElement elemsegmentItems;

    // sequence-item element & segment-item element
    QDomElement elemsequenceItem;
    QDomElement elemsegmentItem;

    // pbc
    QDomElement elemPbc;
                
    // Add Tracks to XML
    QPtrListIterator<K3bVcdTrack> it( *m_doc->tracks() );
    for ( ; it.current(); ++it ) {
        if ( !it.current() ->isSegment() ) {
            // sequence-items element needed at least a sequence to fit the XML
            if ( elemsequenceItems.isNull() )
                elemsequenceItems = addSubElement( xmlDoc, root, "sequence-items" );

            elemsequenceItem = addSubElement( xmlDoc, elemsequenceItems, "sequence-item" );
            elemsequenceItem.setAttribute( "src", QString( "%1" ).arg( QFile::encodeName( it.current() ->absPath() ) ) );
            elemsequenceItem.setAttribute( "id", QString( "sequence-%1" ).arg( QString::number( it.current() ->index() ).rightJustify( 3, '0' ) ) );

            if ( m_doc->vcdOptions() ->PbcEnabled() ) {
                 if ( elemPbc.isNull() ) {
                    elemPbc = addSubElement( xmlDoc, root, "pbc" );
                    QDomElement elemEndlist = addSubElement( xmlDoc, elemPbc, "endlist" );
                    elemEndlist.setAttribute( "id", "end");
                    elemEndlist.setAttribute( "rejected", "true");
                 }
                doPbc(xmlDoc, elemPbc, it.current() );
            }
        } else {
            // sequence-items element needs at least one segment to fit the XML
            if ( elemsegmentItems.isNull() )
                elemsegmentItems = addSubElement( xmlDoc, root, "segment-items" );

            elemsegmentItem = addSubElement( xmlDoc, elemsegmentItems, "segment-item" );
            elemsegmentItem.setAttribute( "src", QString( "%1" ).arg( QFile::encodeName( it.current() ->absPath() ) ) );
            elemsegmentItem.setAttribute( "id", QString( "segment-%1" ).arg( QString::number( it.current() ->index() ).rightJustify( 3, '0' ) ) );

            if ( m_doc->vcdOptions() ->PbcEnabled() ) {
                 if ( elemPbc.isNull() ) {
                    elemPbc = addSubElement( xmlDoc, root, "pbc" );
                    QDomElement elemEndlist = addSubElement( xmlDoc, elemPbc, "endlist" );
                    elemEndlist.setAttribute( "id", "end");
                    elemEndlist.setAttribute( "rejected", "true");
                 }
                doPbc(xmlDoc, elemPbc, it.current() );
            }
        }
    }

    m_xmlstring = xmlDoc.toString();
    kdDebug() << QString( "(K3bVcdXmlView) Write Data to %1:" ).arg( fname ) << endl;
    kdDebug() << endl << m_xmlstring << endl;

    QFile xmlFile( fname );
    if ( xmlFile.open( IO_WriteOnly ) ) {
        QTextStream ts( & xmlFile );
        ts << m_xmlstring;
        xmlFile.close();
        return true;
    }

    return false;
}

QDomElement K3bVcdXmlView::addSubElement( QDomDocument& doc, QDomElement& parent, const QString& name, const QString& value )
{
    QDomElement element = doc.createElement( name );
    parent.appendChild( element );
    if ( !value.isNull() ) {
        QDomText t = doc.createTextNode( value );
        element.appendChild( t );
    }
    return element;
}

QDomElement K3bVcdXmlView::addSubElement( QDomDocument& doc, QDomElement& parent, const QString& name, const int& value )
{
    QDomElement element = doc.createElement( name );
    parent.appendChild( element );
    if ( value >= 0 ) {
        QDomText t = doc.createTextNode( QString( "%1" ).arg( value ) );
        element.appendChild( t );
    }
    return element;
}

QDomElement K3bVcdXmlView::addFolderElement( QDomDocument& doc, QDomElement& parent, const QString& name )
{
    QDomElement elemFolder = addSubElement( doc, parent, "folder" );
    addSubElement( doc, elemFolder, "name", name );

    return elemFolder;
}

void K3bVcdXmlView::addFileElement( QDomDocument& doc, QDomElement& parent, const QString& src, const QString& name, bool mixed )
{
    QDomElement elemFile = addSubElement( doc, parent, "file" );
    elemFile.setAttribute( "src", QString( "%1" ).arg( src ) );
    if ( mixed )
        elemFile.setAttribute( "format", "mixed" );

    addSubElement( doc, elemFile, "name", name );
}

void K3bVcdXmlView::doPbc(QDomDocument& doc, QDomElement& parent, K3bVcdTrack* track)
{
    QString ref = (track->isSegment())? "segment" : "sequence";
    QDomElement elemSelection = addSubElement( doc, parent, "selection" );
    elemSelection.setAttribute( "id", QString( "select-%1-%2" ).arg( ref ).arg( QString::number( track->index() ).rightJustify( 3, '0' ) ) );

    for (int i = 0; i < K3bVcdTrack::_maxPbcTracks; i++) {
        QDomElement elemPbcSelectionPNRDT;

        if ( track->getPbcTrack( i ) ) {
            int index = track->getPbcTrack( i )->index();
            ref = (track->getPbcTrack( i )->isSegment())? "segment" : "sequence";

            switch ( i ) {
                case K3bVcdTrack::PREVIOUS:
                    elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "prev" );
                    elemPbcSelectionPNRDT.setAttribute( "ref", QString( "select-%1-%2" ).arg( ref ).arg( QString::number( index ).rightJustify( 3, '0' ) ) );
                    break;
                case K3bVcdTrack::NEXT:
                    elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "next" );
                    elemPbcSelectionPNRDT.setAttribute( "ref", QString( "select-%1-%2" ).arg( ref ).arg( QString::number( index ).rightJustify( 3, '0' ) ) );
                    break;
                case K3bVcdTrack::RETURN:
                    elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "return" );
                    elemPbcSelectionPNRDT.setAttribute( "ref", QString( "select-%1-%2" ).arg( ref ).arg( QString::number( index ).rightJustify( 3, '0' ) ) );
                    break;
                case K3bVcdTrack::DEFAULT:
                    elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "default" );
                    elemPbcSelectionPNRDT.setAttribute( "ref", QString( "select-%1-%2" ).arg( ref ).arg( QString::number( index ).rightJustify( 3, '0' ) ) );
                    break;
                case K3bVcdTrack::AFTERTIMEOUT:
                    elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "timeout" );
                    elemPbcSelectionPNRDT.setAttribute( "ref", QString( "select-%1-%2" ).arg( ref ).arg( QString::number( index ).rightJustify( 3, '0' ) ) );
                    break;
            }
        } else {
            // jump to <endlist> otherwise do noop while disabled
            if ( track->getNonPbcTrack( i ) == K3bVcdTrack::VIDEOEND ) {
                switch ( i ) {
                    case K3bVcdTrack::PREVIOUS:
                        elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "prev" );
                        elemPbcSelectionPNRDT.setAttribute( "ref", "end");
                        break;
                    case K3bVcdTrack::NEXT:
                        elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "next" );
                        elemPbcSelectionPNRDT.setAttribute( "ref", "end");
                        break;
                    case K3bVcdTrack::RETURN:
                        elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "return" );
                        elemPbcSelectionPNRDT.setAttribute( "ref", "end");
                        break;
                    case K3bVcdTrack::DEFAULT:
                        elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "default" );
                        elemPbcSelectionPNRDT.setAttribute( "ref", "end");
                        break;
                    case K3bVcdTrack::AFTERTIMEOUT:
                        elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "timeout" );
                        elemPbcSelectionPNRDT.setAttribute( "ref", "end");
                        break;
                }
            }
        }
    }

    addSubElement( doc, elemSelection, "wait", track->getWaitTime() );
    QDomElement loop = addSubElement( doc, elemSelection, "loop", track->getPlayTime() );
    if ( track->Reactivity() )
        loop.setAttribute( "jump-timing", "delayed");

    addSubElement( doc, elemSelection, "play-item" ).setAttribute( "ref", QString( "%1-%2" ).arg( ref ).arg( QString::number( track->index() ).rightJustify( 3, '0' ) ) );

}
