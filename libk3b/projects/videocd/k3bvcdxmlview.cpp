/*
*
* Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
*             THX to Manfred Odenstein <odix@chello.at>
* Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
*
* This file is part of the K3b project.
* Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* See the file "COPYING" for the exact licensing terms.
*/

#include "k3bvcdxmlview.h"
#include "k3bvcdtrack.h"
#include "k3bcore.h"
#include "k3bversion.h"

#include <KStandardDirs>
#include <KDebug>

#include <QDomDocument>
#include <QDomElement>
#include <QTextStream>


class K3b::VcdXmlView::Private
{
public:
    void addComment( QDomDocument& doc, QDomElement& parent, const QString& text );
    QDomElement addSubElement( QDomDocument&, QDomElement&, const QString& name, const QString& value = QString() );
    QDomElement addSubElement( QDomDocument&, QDomElement&, const QString& name, const int& value );

    QDomElement addFolderElement( QDomDocument&, QDomElement&, const QString& name );
    void addFileElement( QDomDocument&, QDomElement&, const QString& src, const QString& name, bool mixed = false );
    void doPbc( QDomDocument&, QDomElement&, VcdTrack* );
    void setNumkeyBSN( QDomDocument& , QDomElement&, VcdTrack* );
    void setNumkeySEL( QDomDocument& , QDomElement&, VcdTrack* );
    
    VcdDoc* doc;
    QString xmlstring;
    int startkey;
};

void K3b::VcdXmlView::Private::addComment( QDomDocument& doc, QDomElement& parent, const QString& text )
{
    QDomComment comment = doc.createComment( text );
    parent.appendChild( comment );
}

QDomElement K3b::VcdXmlView::Private::addSubElement( QDomDocument& doc, QDomElement& parent, const QString& name, const QString& value )
{
    QDomElement element = doc.createElement( name );
    parent.appendChild( element );
    if ( !value.isNull() ) {
        QDomText t = doc.createTextNode( value );
        element.appendChild( t );
    }
    return element;
}

QDomElement K3b::VcdXmlView::Private::addSubElement( QDomDocument& doc, QDomElement& parent, const QString& name, const int& value )
{
    QDomElement element = doc.createElement( name );
    parent.appendChild( element );
    if ( value >= -1 ) {
        QDomText t = doc.createTextNode( QString( "%1" ).arg( value ) );
        element.appendChild( t );
    }
    return element;
}

QDomElement K3b::VcdXmlView::Private::addFolderElement( QDomDocument& doc, QDomElement& parent, const QString& name )
{
    QDomElement elemFolder = addSubElement( doc, parent, "folder" );
    addSubElement( doc, elemFolder, "name", name );

    return elemFolder;
}

void K3b::VcdXmlView::Private::addFileElement( QDomDocument& doc, QDomElement& parent, const QString& src, const QString& name, bool mixed )
{
    QDomElement elemFile = addSubElement( doc, parent, "file" );
    elemFile.setAttribute( "src", QString( "%1" ).arg( src ) );
    if ( mixed )
        elemFile.setAttribute( "format", "mixed" );

    addSubElement( doc, elemFile, "name", name );
}

void K3b::VcdXmlView::Private::doPbc( QDomDocument& doc, QDomElement& parent, K3b::VcdTrack* track )
{
    QString ref = ( track->isSegment() ) ? "segment" : "sequence";

    QDomElement elemSelection = addSubElement( doc, parent, "selection" );
    elemSelection.setAttribute( "id", QString( "select-%1-%2" ).arg( ref ).arg( QString::number( track->index() ).rightJustified( 3, '0' ) ) );

    setNumkeyBSN( doc, elemSelection, track );

    Q_FOREACH( VcdTrack::PbcTracks pbc, VcdTrack::trackPlaybackValues() ) {
        QDomElement elemPbcSelectionPNRDT;

        if ( track->getPbcTrack( pbc ) ) {
            int index = track->getPbcTrack( pbc ) ->index();
            QString ref = ( track->getPbcTrack( pbc ) ->isSegment() ) ? "segment" : "sequence";

            switch ( pbc ) {
            case K3b::VcdTrack::PREVIOUS:
                elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "prev" );
                elemPbcSelectionPNRDT.setAttribute( "ref", QString( "select-%1-%2" ).arg( ref ).arg( QString::number( index ).rightJustified( 3, '0' ) ) );
                break;
            case K3b::VcdTrack::NEXT:
                elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "next" );
                elemPbcSelectionPNRDT.setAttribute( "ref", QString( "select-%1-%2" ).arg( ref ).arg( QString::number( index ).rightJustified( 3, '0' ) ) );
                break;
            case K3b::VcdTrack::RETURN:
                elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "return" );
                elemPbcSelectionPNRDT.setAttribute( "ref", QString( "select-%1-%2" ).arg( ref ).arg( QString::number( index ).rightJustified( 3, '0' ) ) );
                break;
            case K3b::VcdTrack::DEFAULT:
                elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "default" );
                elemPbcSelectionPNRDT.setAttribute( "ref", QString( "select-%1-%2" ).arg( ref ).arg( QString::number( index ).rightJustified( 3, '0' ) ) );
                break;
            case K3b::VcdTrack::AFTERTIMEOUT:
                if ( track->getWaitTime() >= 0 ) {
                    elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "timeout" );
                    elemPbcSelectionPNRDT.setAttribute( "ref", QString( "select-%1-%2" ).arg( ref ).arg( QString::number( index ).rightJustified( 3, '0' ) ) );
                }
                break;
            }
        } else {
            // jump to <endlist> otherwise do noop while disabled
            if ( track->getNonPbcTrack( pbc ) == K3b::VcdTrack::VIDEOEND ) {
                switch ( pbc ) {
                case K3b::VcdTrack::PREVIOUS:
                    elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "prev" );
                    elemPbcSelectionPNRDT.setAttribute( "ref", "end" );
                    break;
                case K3b::VcdTrack::NEXT:
                    elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "next" );
                    elemPbcSelectionPNRDT.setAttribute( "ref", "end" );
                    break;
                case K3b::VcdTrack::RETURN:
                    elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "return" );
                    elemPbcSelectionPNRDT.setAttribute( "ref", "end" );
                    break;
                case K3b::VcdTrack::DEFAULT:
                    elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "default" );
                    elemPbcSelectionPNRDT.setAttribute( "ref", "end" );
                    break;
                case K3b::VcdTrack::AFTERTIMEOUT:
                    if ( track->getWaitTime() >= 0 ) {
                        elemPbcSelectionPNRDT = addSubElement( doc, elemSelection, "timeout" );
                        elemPbcSelectionPNRDT.setAttribute( "ref", "end" );
                    }
                    break;
                }
            }
        }
    }

    addSubElement( doc, elemSelection, "wait", track->getWaitTime() );
    QDomElement loop = addSubElement( doc, elemSelection, "loop", track->getPlayTime() );
    if ( track->Reactivity() )
        loop.setAttribute( "jump-timing", "delayed" );
    else
        loop.setAttribute( "jump-timing", "immediate" );

    addSubElement( doc, elemSelection, "play-item" ).setAttribute( "ref", QString( "%1-%2" ).arg( ref ).arg( QString::number( track->index() ).rightJustified( 3, '0' ) ) );

    setNumkeySEL( doc, elemSelection, track );
}

void K3b::VcdXmlView::Private::setNumkeyBSN( QDomDocument& doc, QDomElement& parent, K3b::VcdTrack* track )
{
    if ( track->PbcNumKeys() ) {
        if ( track->PbcNumKeysUserdefined() ) {
            QMap<int, K3b::VcdTrack*> numKeyMap = track->DefinedNumKey();
            QMap<int, K3b::VcdTrack*>::const_iterator trackIt;

            startkey = 0;
            trackIt = numKeyMap.constBegin();
            if ( trackIt != numKeyMap.constEnd() )
                startkey = trackIt.key();

            if ( startkey > 0 )
                addSubElement( doc, parent, "bsn", startkey );
            else // user has no numKeys defined for this track
                track->setPbcNumKeys( false );

        } else {
            // default start with key #1
            addSubElement( doc, parent, "bsn", 1 );
        }
    }
}

void K3b::VcdXmlView::Private::setNumkeySEL( QDomDocument& doc, QDomElement& parent, K3b::VcdTrack* track )
{
    if ( track->PbcNumKeys() ) {
        QDomElement elemPbcSelectionNumKeySEL;
        QString ref = ( track->isSegment() ) ? "segment" : "sequence";
        int none = startkey;
        if ( track->PbcNumKeysUserdefined() ) {
            QMap<int, K3b::VcdTrack*> numKeyMap = track->DefinedNumKey();
            QMap<int, K3b::VcdTrack*>::const_iterator trackIt;

            for ( trackIt = numKeyMap.constBegin(); trackIt != numKeyMap.constEnd(); ++trackIt ) {

                kDebug() << QString( "trackIt key: %1 none: %2" ).arg( trackIt.key() ).arg( none );
                while ( none < trackIt.key() ) {
                    elemPbcSelectionNumKeySEL = addSubElement( doc, parent, "select" );
                    elemPbcSelectionNumKeySEL.setAttribute( "ref", QString( "select-%1-%2" ).arg( ref ).arg( QString::number( track->index() ).rightJustified( 3, '0' ) ) );
                    addComment( doc, parent, QString( "key %1 -> %2 (normal none)" ).arg( none ).arg( track->absolutePath() ) );
                    none++;
                }

                if ( trackIt.value() ) {
                    QString ref = ( trackIt.value() ->isSegment() ) ? "segment" : "sequence";
                    elemPbcSelectionNumKeySEL = addSubElement( doc, parent, "select" );
                    elemPbcSelectionNumKeySEL.setAttribute( "ref", QString( "select-%1-%2" ).arg( ref ).arg( QString::number( trackIt.value() ->index() ).rightJustified( 3, '0' ) ) );
                    addComment( doc, parent, QString( "key %1 -> %2" ).arg( trackIt.key() ).arg( trackIt.value() ->absolutePath() ) );
                } else {
                    elemPbcSelectionNumKeySEL = addSubElement( doc, parent, "select" );
                    elemPbcSelectionNumKeySEL.setAttribute( "ref", "end" );
                    addComment( doc, parent, QString( "key %1 -> end" ).arg( trackIt.key() ) );
                }
                none++;
            }
        } else {
            // default reference to itSelf
            elemPbcSelectionNumKeySEL = addSubElement( doc, parent, "select" );
            elemPbcSelectionNumKeySEL.setAttribute( "ref", QString( "select-%1-%2" ).arg( ref ).arg( QString::number( track->index() ).rightJustified( 3, '0' ) ) );
        }
    }
}

K3b::VcdXmlView::VcdXmlView( K3b::VcdDoc* pDoc )
    : d( new Private )
{
    d->doc = pDoc;
}

K3b::VcdXmlView::~VcdXmlView()
{
    delete d;
}

void K3b::VcdXmlView::write( QFile& file )
{

    QDomDocument xmlDoc( "videocd PUBLIC \"-//GNU//DTD VideoCD//EN\" \"http://www.gnu.org/software/vcdimager/videocd.dtd\"" );
    // xmlDoc.appendChild( xmlDoc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"iso-8859-1\"" ) );
    xmlDoc.appendChild( xmlDoc.createProcessingInstruction( "xml", "version=\"1.0\"" ) );

    // create root element
    QDomElement root = xmlDoc.createElement( "videocd" );
    root.setAttribute( "xmlns", "http://www.gnu.org/software/vcdimager/1.0/" );
    root.setAttribute( "class", d->doc->vcdOptions() ->vcdClass() );
    root.setAttribute( "version", d->doc->vcdOptions() ->vcdVersion() );
    xmlDoc.appendChild( root );

    // create option elements

    // Broken SVCD mode - NonCompliantMode
    if ( d->doc->vcdOptions() ->NonCompliantMode() ) {
        QDomElement elemOption;
        elemOption = d->addSubElement( xmlDoc, root, "option" );
        elemOption.setAttribute( "name", "svcd vcd30 mpegav" );
        elemOption.setAttribute( "value", "true" );

        elemOption = d->addSubElement( xmlDoc, root, "option" );
        elemOption.setAttribute( "name", "svcd vcd30 entrysvd" );
        elemOption.setAttribute( "value", "true" );
    }

    // VCD3.0 track interpretation
    if ( d->doc->vcdOptions() ->VCD30interpretation() ) {
        QDomElement elemOption;
        elemOption = d->addSubElement( xmlDoc, root, "option" );
        elemOption.setAttribute( "name", "svcd vcd30 tracksvd" );
        elemOption.setAttribute( "value", "true" );
    }

    // Relaxed aps
    if ( d->doc->vcdOptions() ->RelaxedAps() ) {
        QDomElement elemOption;
        elemOption = d->addSubElement( xmlDoc, root, "option" );
        elemOption.setAttribute( "name", "relaxed aps" );
        elemOption.setAttribute( "value", "true" );
    }

    // Update scan offsets
    if ( d->doc->vcdOptions() ->UpdateScanOffsets() ) {
        QDomElement elemOption;
        elemOption = d->addSubElement( xmlDoc, root, "option" );
        elemOption.setAttribute( "name", "update scan offsets" );
        elemOption.setAttribute( "value", "true" );

    }

    // Gaps & Margins
    if ( d->doc->vcdOptions() ->UseGaps() ) {
        QDomElement elemOption;
        elemOption = d->addSubElement( xmlDoc, root, "option" );
        elemOption.setAttribute( "name", "leadout pregap" );
        elemOption.setAttribute( "value", d->doc->vcdOptions() ->PreGapLeadout() );

        elemOption = d->addSubElement( xmlDoc, root, "option" );
        elemOption.setAttribute( "name", "track pregap" );
        elemOption.setAttribute( "value", d->doc->vcdOptions() ->PreGapTrack() );

        if ( d->doc->vcdOptions() ->vcdClass() == "vcd" ) {
            elemOption = d->addSubElement( xmlDoc, root, "option" );
            elemOption.setAttribute( "name", "track front margin" );
            elemOption.setAttribute( "value", d->doc->vcdOptions() ->FrontMarginTrack() );

            elemOption = d->addSubElement( xmlDoc, root, "option" );
            elemOption.setAttribute( "name", "track rear margin" );
            elemOption.setAttribute( "value", d->doc->vcdOptions() ->RearMarginTrack() );
        } else {
            elemOption = d->addSubElement( xmlDoc, root, "option" );
            elemOption.setAttribute( "name", "track front margin" );
            elemOption.setAttribute( "value", d->doc->vcdOptions() ->FrontMarginTrackSVCD() );

            elemOption = d->addSubElement( xmlDoc, root, "option" );
            elemOption.setAttribute( "name", "track rear margin" );
            elemOption.setAttribute( "value", d->doc->vcdOptions() ->RearMarginTrackSVCD() );
        }

    }

    // create info element
    QDomElement elemInfo = d->addSubElement( xmlDoc, root, "info" );
    d->addSubElement( xmlDoc, elemInfo, "album-id", d->doc->vcdOptions() ->albumId().toUpper() );
    d->addSubElement( xmlDoc, elemInfo, "volume-count", d->doc->vcdOptions() ->volumeCount() );
    d->addSubElement( xmlDoc, elemInfo, "volume-number", d->doc->vcdOptions() ->volumeNumber() );
    d->addSubElement( xmlDoc, elemInfo, "restriction", d->doc->vcdOptions() ->Restriction() );

    // create pvd element
    QDomElement elemPvd = d->addSubElement( xmlDoc, root, "pvd" );
    d->addSubElement( xmlDoc, elemPvd, "volume-id", d->doc->vcdOptions() ->volumeId().toUpper() );
    d->addSubElement( xmlDoc, elemPvd, "system-id", d->doc->vcdOptions() ->systemId() );
    d->addSubElement( xmlDoc, elemPvd, "application-id", d->doc->vcdOptions() ->applicationId() );
    d->addSubElement( xmlDoc, elemPvd, "preparer-id", QString( "K3b - Version %1" ).arg( k3bcore->version() ).toUpper() );
    d->addSubElement( xmlDoc, elemPvd, "publisher-id", d->doc->vcdOptions() ->publisher().toUpper() );


    // create filesystem element
    QDomElement elemFileSystem = d->addSubElement( xmlDoc, root, "filesystem" );

    // SEGMENT folder, some standalone DVD-Player need this
    if ( !d->doc->vcdOptions() ->haveSegments() && d->doc->vcdOptions() ->SegmentFolder() )
        d->addFolderElement( xmlDoc, elemFileSystem, "SEGMENT" );

    // create cdi element
    if ( d->doc->vcdOptions() ->CdiSupport() ) {
        QDomElement elemFolder = d->addFolderElement( xmlDoc, elemFileSystem, "CDI" );

        d->addFileElement( xmlDoc, elemFolder, KStandardDirs::locate( "data", "k3b/cdi/cdi_imag.rtf" ), "CDI_IMAG.RTF", true );
        d->addFileElement( xmlDoc, elemFolder, KStandardDirs::locate( "data", "k3b/cdi/cdi_text.fnt" ), "CDI_TEXT.FNT" );
        d->addFileElement( xmlDoc, elemFolder, KStandardDirs::locate( "data", "k3b/cdi/cdi_vcd.app" ), "CDI_VCD.APP" );

        QString usercdicfg = KStandardDirs::locateLocal( "appdata", "cdi/cdi_vcd.cfg" );
        if ( QFile::exists( usercdicfg ) )
            d->addFileElement( xmlDoc, elemFolder, usercdicfg, "CDI_VCD.CFG" );
        else
            d->addFileElement( xmlDoc, elemFolder, KStandardDirs::locate( "data", "k3b/cdi/cdi_vcd.cfg" ), "CDI_VCD.CFG" );
    }

    // sequence-items element & segment-items element
    QDomElement elemsequenceItems;
    QDomElement elemsegmentItems;

    // sequence-item element & segment-item element
    QDomElement elemsequenceItem;
    QDomElement elemsegmentItem;

    // if we have segments, elemsegmentItems must be before any sequence in xml file order
    if ( d->doc->vcdOptions()->haveSegments()  )
        elemsegmentItems = d->addSubElement( xmlDoc, root, "segment-items" );

    // sequence must always available ...
    elemsequenceItems = d->addSubElement( xmlDoc, root, "sequence-items" );
    // if we have no sequence (photo (s)vcd) we must add a dummy sequence they inform the user to turn on pbc on there videoplayer
    if ( !d->doc->vcdOptions()->haveSequence() )  {
        QString filename;
        if  ( d->doc->vcdOptions()->mpegVersion() == 1 )
            filename = KStandardDirs::locate( "data", "k3b/extra/k3bphotovcd.mpg" );
        else
            filename = KStandardDirs::locate( "data", "k3b/extra/k3bphotosvcd.mpg" );

        elemsequenceItem = d->addSubElement( xmlDoc, elemsequenceItems, "sequence-item" );
        elemsequenceItem.setAttribute( "src", filename );
        elemsequenceItem.setAttribute( "id", "sequence-000"  );
        // default entry
        QDomElement elemdefaultEntry;
        elemdefaultEntry = d->addSubElement( xmlDoc, elemsequenceItem, "default-entry" );
        elemdefaultEntry.setAttribute( "id", "entry-000"  );
    }


    // pbc
    QDomElement elemPbc;

    // Add Tracks to XML
    Q_FOREACH( K3b::VcdTrack* track,  *d->doc->tracks() ) {
        if ( !track ->isSegment() ) {
            QString seqId = QString::number( track ->index() ).rightJustified( 3, '0' );

            elemsequenceItem = d->addSubElement( xmlDoc, elemsequenceItems, "sequence-item" );
            elemsequenceItem.setAttribute( "src", track ->absolutePath() );
            elemsequenceItem.setAttribute( "id", QString( "sequence-%1" ).arg( seqId ) );

            // default entry
            QDomElement elemdefaultEntry;
            elemdefaultEntry = d->addSubElement( xmlDoc, elemsequenceItem, "default-entry" );
            elemdefaultEntry.setAttribute( "id", QString( "entry-%1" ).arg( seqId ) );

        } else {
            // sequence-items element needs at least one segment to fit the XML
            elemsegmentItem = d->addSubElement( xmlDoc, elemsegmentItems, "segment-item" );
            elemsegmentItem.setAttribute( "src", track ->absolutePath() );
            elemsegmentItem.setAttribute( "id", QString( "segment-%1" ).arg( QString::number( track ->index() ).rightJustified( 3, '0' ) ) );

        }
    }
    Q_FOREACH( K3b::VcdTrack* track,  *d->doc->tracks() ) {
        if ( d->doc->vcdOptions() ->PbcEnabled() ) {
            if ( elemPbc.isNull() )
                elemPbc = d->addSubElement( xmlDoc, root, "pbc" );

            d->doPbc( xmlDoc, elemPbc, track );
        }
    }

    if ( ! elemPbc.isNull() ) {
        QDomElement elemEndlist = d->addSubElement( xmlDoc, elemPbc, "endlist" );
        elemEndlist.setAttribute( "id", "end" );
        elemEndlist.setAttribute( "rejected", "true" );
    }

    d->xmlstring = xmlDoc.toString();
    
    QTextStream ts( &file );
    ts << d->xmlstring;
}

const QString& K3b::VcdXmlView::xmlString() const
{
    return d->xmlstring;
}
