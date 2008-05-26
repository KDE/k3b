/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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

#include <config-k3b.h>


#include "k3bdiskinfoview.h"

#include <k3bdiskinfo.h>
#include <k3bcdtext.h>
#include <k3bdeviceglobals.h>
#include <k3bglobals.h>
#include <k3biso9660.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qfont.h>
#include <qcolor.h>
#include <qstring.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qpixmap.h>
#include <QtGui/QTextBrowser>

#include <klocale.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kio/global.h>


K3bDiskInfoView::K3bDiskInfoView( QWidget* parent )
    : K3bMediaContentsView( true,
                            K3bMedium::CONTENT_ALL,
                            K3bDevice::MEDIA_ALL|K3bDevice::MEDIA_UNKNOWN,
                            K3bDevice::STATE_EMPTY|K3bDevice::STATE_INCOMPLETE|K3bDevice::STATE_COMPLETE|K3bDevice::STATE_UNKNOWN,
                            parent )
{
    m_infoView = new QTextBrowser( this );
    setMainWidget( m_infoView );
}


K3bDiskInfoView::~K3bDiskInfoView()
{
}


void K3bDiskInfoView::reloadMedium()
{
    updateTitle();

    QString s;
    if( medium().diskInfo().diskState() == K3bDevice::STATE_NO_MEDIA ) {
        s = i18n("No medium present");
    }
    else {

    }
}


void K3bDiskInfoView::updateTitle()
{
    setTitle( medium().shortString( true ) );

    if( medium().diskInfo().diskState() == K3bDevice::STATE_NO_MEDIA ) {
        setRightPixmap( K3bTheme::MEDIA_NONE );
    }
    else {
        if( medium().diskInfo().empty() ) {
            setRightPixmap( K3bTheme::MEDIA_EMPTY );
        }
        else {
            switch( medium().toc().contentType() ) {
            case K3bDevice::AUDIO:
                setRightPixmap( K3bTheme::MEDIA_AUDIO );
                break;
            case K3bDevice::DATA: {
                if( medium().content() & K3bMedium::CONTENT_VIDEO_DVD ) {
                    setRightPixmap( K3bTheme::MEDIA_VIDEO );
                }
                else {
                    setRightPixmap( K3bTheme::MEDIA_DATA );
                }
                break;
            }
            case K3bDevice::MIXED:
                setRightPixmap( K3bTheme::MEDIA_MIXED );
                break;
            default:
                setRightPixmap( K3bTheme::MEDIA_NONE );
            }
        }
    }
}

// void K3bDiskInfoView::reloadMedium()
// {

//         createMediaInfoItems( medium() );


//         // iso9660 info
//         // /////////////////////////////////////////////////////////////////////////////////////
//         if( medium().content() & K3bMedium::CONTENT_DATA ) {
//             (void)new K3ListViewItem( m_infoView, m_infoView->lastChild() ); // empty spacer item
//             createIso9660InfoItems( medium().iso9660Descriptor() );
//         }

//         // tracks
//         // /////////////////////////////////////////////////////////////////////////////////////
//         if( !medium().toc().isEmpty() ) {

//             if( m_infoView->childCount() )
//                 (void)new K3ListViewItem( m_infoView, m_infoView->lastChild() ); // empty spacer item

//             K3ListViewItem* trackHeaderItem = new HeaderViewItem( m_infoView, m_infoView->lastChild(), i18n("Tracks") );

//             // create header item
//             K3ListViewItem* item = new K3ListViewItem( trackHeaderItem,
//                                                        i18n("Type"),
//                                                        i18n("Attributes"),
//                                                        i18n("First-Last Sector"),
//                                                        i18n("Length") );

// #ifdef K3B_DEBUG
//             item->setText( 4, "Index0" );
// #endif

//             int lastSession = 0;

//             // if we have multiple sessions we create a header item for every session
//             K3ListViewItem* trackItem = 0;
//             if( medium().diskInfo().numSessions() > 1 && medium().toc()[0].session() > 0 ) {
//                 trackItem = new HeaderViewItem( trackHeaderItem, item, i18n("Session %1",QString::number(1)) );
//                 lastSession = 1;
//             }
//             else
//                 trackItem = trackHeaderItem;

//             // create items for the tracks
//             int index = 1;
//             foreach( const K3bTrack& track, medium().toc() ) {

//                 if( medium().diskInfo().numSessions() > 1 && track.session() != lastSession ) {
//                     lastSession = track.session();
//                     trackItem->setOpen(true);
//                     trackItem = new HeaderViewItem( trackHeaderItem,
//                                                     m_infoView->lastItem()->parent(),
//                                                     i18n("Session %1",lastSession) );
//                 }

//                 item = new K3ListViewItem( trackItem, item );
//                 QString text;
//                 if( track.type() == K3bTrack::AUDIO ) {
//                     item->setPixmap( 0, SmallIcon( "audio-x-generic" ) );
//                     text = i18n("Audio");
//                 } else {
//                     item->setPixmap( 0, SmallIcon( "application-x-tar" ) );
//                     if( track.mode() == K3bTrack::MODE1 )
//                         text = i18n("Data/Mode1");
//                     else if( track.mode() == K3bTrack::MODE2 )
//                         text = i18n("Data/Mode2");
//                     else if( track.mode() == K3bTrack::XA_FORM1 )
//                         text = i18n("Data/Mode2 XA Form1");
//                     else if( track.mode() == K3bTrack::XA_FORM2 )
//                         text = i18n("Data/Mode2 XA Form2");
//                     else
//                         text = i18n("Data");
//                 }
//                 item->setText( 0, i18n("%1 (%2)", QString::number(index).rightJustified( 2, ' ' ),text) );
//                 item->setText( 1, QString( "%1/%2" )
//                                .arg( track.copyPermitted() ? i18n("copy") : i18n("no copy") )
//                                .arg( track.type() == K3bTrack::AUDIO
//                                      ? ( track.preEmphasis() ?  i18n("preemp") : i18n("no preemp") )
//                                      : ( track.recordedIncremental() ?  i18n("incremental") : i18n("uninterrupted") ) ) );
//                 item->setText( 2,
//                                QString("%1 - %2")
//                                .arg(track.firstSector().lba())
//                                .arg(track.lastSector().lba()) );
//                 item->setText( 3, QString::number( track.length().lba() ) + " (" + track.length().toString() + ")" );

// #ifdef K3B_DEBUG
//                 if( track.type() == K3bTrack::AUDIO )
//                     item->setText( 4, QString( "%1 (%2)" ).arg(track.index0().toString()).arg(track.index0().lba()) );
// #endif
//                 ++index;
//             }

//             trackItem->setOpen(true);
//             trackHeaderItem->setOpen( true );
//         }


//         // CD-TEXT
//         // /////////////////////////////////////////////////////////////////////////////////////
//         if( !medium().cdText().isEmpty() ) {
//             medium().cdText().debug();
//             if( m_infoView->childCount() )
//                 (void)new K3ListViewItem( m_infoView, m_infoView->lastChild() ); // empty spacer item

//             K3ListViewItem* cdTextHeaderItem = new HeaderViewItem( m_infoView,
//                                                                    m_infoView->lastChild(),
//                                                                    i18n("CD-TEXT (excerpt)") );

//             // create header item
//             K3ListViewItem* item = new K3ListViewItem( cdTextHeaderItem,
//                                                        i18n("Performer"),
//                                                        i18n("Title"),
//                                                        i18n("Songwriter"),
//                                                        i18n("Composer") );
//             item = new K3ListViewItem( cdTextHeaderItem, item );
//             item->setText( 0, i18n("CD:") + " " +
//                            medium().cdText().performer() );
//             item->setText( 1, medium().cdText().title() );
//             item->setText( 2, medium().cdText().songwriter() );
//             item->setText( 3, medium().cdText().composer() );

//             int index = 1;
//             for( int i = 0; i < medium().cdText().count(); ++i ) {
//                 item = new K3ListViewItem( cdTextHeaderItem, item );
//                 item->setText( 0, QString::number(index).rightJustified( 2, ' ' ) + " " +
//                                medium().cdText().track( i ).performer() );
//                 item->setText( 1, medium().cdText().track(i).title() );
//                 item->setText( 2, medium().cdText().track(i).songwriter() );
//                 item->setText( 3, medium().cdText().track(i).composer() );
//                 ++index;
//             }

//             cdTextHeaderItem->setOpen( true );
//         }
//     }
// }


// void K3bDiskInfoView::createMediaInfoItems( const K3bMedium& medium )
// {
//     const K3bDevice::DiskInfo& info = medium.diskInfo();

//     K3ListViewItem* atipItem = new HeaderViewItem( m_infoView, m_infoView->lastItem(), i18n("Medium") );
//     QString typeStr;
//     if( info.mediaType() != K3bDevice::MEDIA_UNKNOWN )
//         typeStr = K3bDevice::mediaTypeString( info.mediaType() );
//     else
//         typeStr = i18n("Unknown (probably CD-ROM)");

//     K3ListViewItem* atipChild = new K3ListViewItem( atipItem, i18n("Type:"), typeStr );

//     if( info.isDvdMedia() )
//         atipChild = new K3ListViewItem( atipItem, atipChild,
//                                         i18n("Media ID:"),
//                                         !info.mediaId().isEmpty() ? QString::fromLatin1( info.mediaId() ) : i18n("unknown") );


//     atipChild = new K3ListViewItem( atipItem, atipChild,
//                                     i18n("Capacity:"),
//                                     i18n("%1 min",info.capacity().toString()),
//                                     KIO::convertSize(info.capacity().mode1Bytes()) );

//     if( !info.empty() )
//         atipChild = new K3ListViewItem( atipItem, atipChild,
//                                         i18n("Used Capacity:"),
//                                         i18n("%1 min",info.size().toString()),
//                                         KIO::convertSize(info.size().mode1Bytes()) );

//     if( info.appendable() )
//         atipChild = new K3ListViewItem( atipItem, atipChild,
//                                         i18n("Remaining:"),
//                                         i18n("%1 min",info.remainingSize().toString() ),
//                                         KIO::convertSize(info.remainingSize().mode1Bytes()) );

//     atipChild = new K3ListViewItem( atipItem, atipChild,
//                                     i18n("Rewritable:"),
//                                     info.rewritable() ? i18n("yes") : i18n("no") );

//     atipChild = new K3ListViewItem( atipItem, atipChild,
//                                     i18n("Appendable:"),
//                                     info.appendable() ? i18n("yes") : i18n("no") );

//     atipChild = new K3ListViewItem( atipItem, atipChild,
//                                     i18n("Empty:"),
//                                     info.empty() ? i18n("yes") : i18n("no") );

//     if( K3bDevice::isDvdMedia( info.mediaType() ) )
//         atipChild = new K3ListViewItem( atipItem, atipChild,
//                                         i18n("Layers:"),
//                                         QString::number( info.numLayers() ) );

//     if( info.mediaType() == K3bDevice::MEDIA_DVD_PLUS_RW ) {
//         atipChild = new K3ListViewItem( atipItem, atipChild,
//                                         i18n("Background Format:") );
//         switch( info.bgFormatState() ) {
//         case K3bDevice::BG_FORMAT_NONE:
//             atipChild->setText( 1, i18n("not formatted") );
//             break;
//         case K3bDevice::BG_FORMAT_INCOMPLETE:
//             atipChild->setText( 1, i18n("incomplete") );
//             break;
//         case K3bDevice::BG_FORMAT_IN_PROGRESS:
//             atipChild->setText( 1, i18n("in progress") );
//             break;
//         case K3bDevice::BG_FORMAT_COMPLETE:
//             atipChild->setText( 1, i18n("complete") );
//             break;
//         }
//     }

//     atipChild = new K3ListViewItem( atipItem, atipChild,
//                                     i18n("Sessions:"),
//                                     QString::number( info.numSessions() ) );

//     if( info.mediaType() & K3bDevice::MEDIA_WRITABLE ) {
//         atipChild = new K3ListViewItem( atipItem, atipChild,
//                                         i18n("Supported writing speeds:") );
//         QString s;
//         if( medium.writingSpeeds().isEmpty() )
//             s = "-";
//         else
//             foreach( int speed, medium.writingSpeeds() ) {
//                 if( !s.isEmpty() ) {
//                     s.append( "\n" );
//                     atipChild->setMultiLinesEnabled( true );
//                 }

//                 if( K3bDevice::isCdMedia( info.mediaType() ) )
//                     s.append( QString( "%1x (%2 KB/s)" ).arg( speed/K3bDevice::SPEED_FACTOR_CD ).arg( speed ) );
//                 else if( K3bDevice::isDvdMedia( info.mediaType() ) )
//                     s.append( QString().sprintf( "%.1fx (%d KB/s)", (double)speed / ( double )K3bDevice::SPEED_FACTOR_DVD, speed ) );
//                 else if ( K3bDevice::isBdMedia( info.mediaType() ) )
//                     s.append( QString().sprintf( "%.1fx (%d KB/s)", (double)speed / ( double )K3bDevice::SPEED_FACTOR_BD, speed ) );
//             }

//         atipChild->setText( 1, s );
//     }

//     atipItem->setOpen( true );
// }


// void K3bDiskInfoView::createIso9660InfoItems( const K3bIso9660SimplePrimaryDescriptor& iso )
// {
//     K3ListViewItem* iso9660Item = new HeaderViewItem( m_infoView, m_infoView->lastChild(),
//                                                       i18n("ISO9660 Filesystem Info") );
//     K3ListViewItem* iso9660Child = 0;

//     iso9660Child = new K3ListViewItem( iso9660Item, iso9660Child,
//                                        i18n("System Id:"),
//                                        iso.systemId.isEmpty()
//                                        ? QString("-")
//                                        : iso.systemId );
//     iso9660Child = new K3ListViewItem( iso9660Item, iso9660Child,
//                                        i18n("Volume Id:"),
//                                        iso.volumeId.isEmpty()
//                                        ? QString("-")
//                                        : iso.volumeId );
//     iso9660Child = new K3ListViewItem( iso9660Item, iso9660Child,
//                                        i18n("Volume Set Id:"),
//                                        iso.volumeSetId.isEmpty()
//                                        ? QString("-")
//                                        : iso.volumeSetId );
//     iso9660Child = new K3ListViewItem( iso9660Item, iso9660Child,
//                                        i18n("Publisher Id:"),
//                                        iso.publisherId.isEmpty()
//                                        ? QString("-")
//                                        : iso.publisherId );
//     iso9660Child = new K3ListViewItem( iso9660Item, iso9660Child,
//                                        i18n("Preparer Id:"),
//                                        iso.preparerId.isEmpty()
//                                        ? QString("-")
//                                        : iso.preparerId );
//     iso9660Child = new K3ListViewItem( iso9660Item, iso9660Child,
//                                        i18n("Application Id:"),
//                                        iso.applicationId.isEmpty()
//                                        ? QString("-")
//                                        : iso.applicationId );
// //   iso9660Child = new K3ListViewItem( iso9660Item, iso9660Child,
// // 				    i18n("Volume Size:"),
// // 				    QString( "%1 (%2*%3)" )
// // 				    .arg(iso.logicalBlockSize
// // 					 *iso.volumeSpaceSize)
// // 				    .arg(iso.logicalBlockSize)
// // 				    .arg(iso.volumeSpaceSize),
// // 				    KIO::convertSize(iso.logicalBlockSize
// // 						     *iso.volumeSpaceSize)  );

//     iso9660Item->setOpen( true );
// }


// void K3bDiskInfoView::enableInteraction( bool enable )
// {
//     Q3ListViewItemIterator it( m_infoView );
//     while( it.current() ) {
//         it.current()->setEnabled( enable );
//         ++it;
//     }
// }

#include "k3bdiskinfoview.moc"

