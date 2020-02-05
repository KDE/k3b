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
#include "k3bthememanager.h"
#include "k3bapplication.h"

#include "k3bdiskinfo.h"
#include "k3bcdtext.h"
#include "k3bdeviceglobals.h"
#include "k3bglobals.h"
#include "k3biso9660.h"

#include <KCddb/Cdinfo>

#include <KIconLoader>
#include <KLocalizedString>
#include <KIO/Global>

#include <QDebug>
#include <QString>
#include <QFont>
#include <QColor>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#ifdef HAVE_QT5WEBKITWIDGETS
#include <QWebView>
#endif
#include <QLabel>
#include <QLayout>
#include <QTextBrowser>

namespace {
    QString sectionHeader( const QString& title ) {
        return "<h1 class=\"title\">" + title + "</h1>";
    }

    QString tableRow( const QString& s1, const QString& s2 ) {
        return "<tr><td class=\"infokey\">" + s1 + "</td><td class=\"infovalue\">" + s2 + "</td></tr>";
    }
}


K3b::DiskInfoView::DiskInfoView( QWidget* parent )
    : K3b::MediaContentsView( true,
                              Medium::ContentAll,
                              Device::MEDIA_ALL|Device::MEDIA_NONE|Device::MEDIA_UNKNOWN,
                              Device::STATE_ALL|Device::STATE_NO_MEDIA|Device::STATE_UNKNOWN,
                              parent )
{
#ifdef HAVE_QT5WEBKITWIDGETS
    m_infoView = new QWebView( this );
#else
    m_infoView = new QTextBrowser( this );
#endif
    setMainWidget( m_infoView );
}


K3b::DiskInfoView::~DiskInfoView()
{
}


void K3b::DiskInfoView::reloadMedium()
{
    updateTitle();

    QString s;
    K3b::Theme* theme = k3bappcore->themeManager()->currentTheme();
    if (medium().diskInfo().diskState() == K3b::Device::STATE_NO_MEDIA) {
        s = QString("<font color=\"%1\">%2</font>").arg(theme ?
                theme->palette().color(QPalette::Foreground).name() :
                "green").arg(i18n("No medium present"));
    } else {
        // FIXME: import a css stylesheet from the theme and, more importantly,
        // create a nicer css
        // We are not designers :)
        s = "<head>"
            "<title></title>"
            "<style type=\"text/css\">";
        if (theme) {
            s += QString(".title { padding:4px; font-size:medium; background-color:%1; color:%2 } ")
                 .arg(theme->palette().color(QPalette::Background).name())
                 .arg(theme->palette().color(QPalette::Foreground).name());
        }
        s +=  QString(".infovalue { font-weight:bold; padding-left:10px; color:%1; } "
              ".infokey { color:%1; } "
              ".trackheader { text-align:left; color:%1; } "
              ".session { font-style:italic; color:%1; } "
              ".cdtext { font-weight:bold; font-style:italic; color:%1; } "
              ".tracknumber { color:%1; } "
              ".tracktype { color:%1; } "
              ".trackattributes { color:%1; } "
              ".trackrange { color:%1; } "
              ".tracklength { color:%1; } "
              "td { vertical-align:top; } ")
            .arg(theme->palette().color(QPalette::Foreground).name());
        s += "</style>"
             "</head>"
             "<body>";

        s += sectionHeader(i18n("Medium"));
        s += createMediaInfoItems(medium());

        if (medium().content() & K3b::Medium::ContentData) {
            s += sectionHeader(i18n("ISO 9660 Filesystem Info"));
            s += createIso9660InfoItems(medium().iso9660Descriptor());
        }

        if (!medium().toc().isEmpty()) {
            s += sectionHeader(i18n("Tracks"));
            s += createTrackItems(medium());
        }
    }

    s += "</body>";

    m_infoView->setHtml(s);
#ifdef K3B_DEBUG
    qDebug() << s;
#endif
}


void K3b::DiskInfoView::updateTitle()
{
    QString title = medium().shortString();
    QString subTitle = medium().shortString( Medium::NoStringFlags );
    if( title == subTitle )
        subTitle.truncate(0);
    setTitle( title, subTitle );

    if( medium().diskInfo().diskState() == K3b::Device::STATE_NO_MEDIA ) {
        setRightPixmap( K3b::Theme::MEDIA_NONE );
    }
    else {
        if( medium().diskInfo().empty() ) {
            setRightPixmap( K3b::Theme::MEDIA_EMPTY );
        }
        else {
            switch( medium().toc().contentType() ) {
            case K3b::Device::AUDIO:
                setRightPixmap( K3b::Theme::MEDIA_AUDIO );
                break;
            case K3b::Device::DATA: {
                if( medium().content() & K3b::Medium::ContentVideoDVD ) {
                    setRightPixmap( K3b::Theme::MEDIA_VIDEO );
                }
                else {
                    setRightPixmap( K3b::Theme::MEDIA_DATA );
                }
                break;
            }
            case K3b::Device::MIXED:
                setRightPixmap( K3b::Theme::MEDIA_MIXED );
                break;
            default:
                setRightPixmap( K3b::Theme::MEDIA_NONE );
            }
        }
    }
}


namespace {
    QString trackItem( const K3b::Medium& medium, int trackIndex ) {
        K3b::Device::Track track = medium.toc()[trackIndex];

        QString s = "<tr>";

        // FIXME: Icons

        // track number
        s += "<td class=\"tracknumber\">" + QString::number(trackIndex+1).rightJustified( 2, ' ' );
        s += "</td>";

        // track type
        s += "<td class=\"tracktype\">(";
        if( track.type() == K3b::Device::Track::TYPE_AUDIO ) {
            //    item->setPixmap( 0, SmallIcon( "audio-x-generic" ) );
            s += i18n("Audio");
        } else {
//            item->setPixmap( 0, SmallIcon( "application-x-tar" ) );
            if( track.mode() == K3b::Device::Track::MODE1 )
                s += i18n("Data/Mode1");
            else if( track.mode() == K3b::Device::Track::MODE2 )
                s += i18n("Data/Mode2");
            else if( track.mode() == K3b::Device::Track::XA_FORM1 )
                s += i18n("Data/Mode2 XA Form1");
            else if( track.mode() == K3b::Device::Track::XA_FORM2 )
                s += i18n("Data/Mode2 XA Form2");
            else
                s += i18n("Data");
        }
        s += ")</td>";

        // track attributes
        s += "<td class=\"trackattributes\">";
        s += QString( "%1/%2" )
             .arg( track.copyPermitted() ? i18n("copy") : i18n("no copy") )
             .arg( track.type() == K3b::Device::Track::TYPE_AUDIO
                   ? ( track.preEmphasis() ?  i18n("preemp") : i18n("no preemp") )
                   : ( track.recordedIncremental() ?  i18n("incremental") : i18n("uninterrupted") ) );
        s += "</td>";

        // track range
        s += "<td class=\"trackrange\">";
        s += QString("%1 - %2")
             .arg(track.firstSector().lba())
             .arg(track.lastSector().lba());
        s += "</td>";

        // track length
        s += "<td class=\"tracklength\">" + QString::number( track.length().lba() ) + " (" + track.length().toString() + ")</td>";

#ifdef K3B_DEBUG
        s += "<td>";
        if( track.type() == K3b::Device::Track::TYPE_AUDIO )
            s += QString( "%1 (%2)" ).arg(track.index0().toString()).arg(track.index0().lba());
        s += "</td>";
#endif

        s += "</tr>";

        if( !medium.cdText().isEmpty() ||
            medium.cddbInfo().isValid() ) {
            s += "<tr><td></td><td class=\"cdtext\" colspan=\"4\">";
            if( !medium.cdText().isEmpty() ) {
                K3b::Device::TrackCdText text = medium.cdText().track( trackIndex );
                s += text.performer() + " - " + text.title() + " (" + i18n("CD-Text") + ')';
            }
            else {
                KCDDB::TrackInfo info = medium.cddbInfo().track( trackIndex );
                s += info.get( KCDDB::Artist ).toString() + " - " + info.get( KCDDB::Title ).toString();
            }
            s += "</td></tr>";
        }

        return s;
    }

    QString sessionItem( int session ) {
        return "<tr><td class=\"session\" colspan=\"5\">" + i18n( "Session %1", session ) + "</td></tr>";
    }
}

QString K3b::DiskInfoView::createTrackItems( const K3b::Medium& medium )
{
    QString s = "<table>";

    // table header
    s += "<tr><th class=\"trackheader\" colspan=\"2\">"
         + i18n("Type")
         + "</th><th class=\"trackheader\">"
         + i18n("Attributes")
         + "</th><th class=\"trackheader\">"
         + i18n("First-Last Sector")
         + "</th><th class=\"trackheader\">"
         + i18n("Length");
#ifdef K3B_DEBUG
    s += "</td></td>Index0";
#endif
    s += "</th></tr>";

    int lastSession = 0;

    // if we have multiple sessions we create a header item for every session
    if( medium.diskInfo().numSessions() > 1 && medium.toc()[0].session() > 0 ) {
        s += sessionItem( 1 );
        lastSession = 1;
    }

    // create items for the tracks
    for( int index = 0; index < medium.toc().count(); ++index ) {
        K3b::Device::Track track = medium.toc()[index];
        if( medium.diskInfo().numSessions() > 1 && track.session() != lastSession ) {
            lastSession = track.session();
            s += sessionItem( lastSession );
        }
        s += trackItem( medium, index );
    }

    s += "</table>";

    return s;
}


QString K3b::DiskInfoView::createMediaInfoItems( const K3b::Medium& medium )
{
    const K3b::Device::DiskInfo& info = medium.diskInfo();

    QString s = "<table>";

    QString typeStr;
    if( info.mediaType() != K3b::Device::MEDIA_UNKNOWN )
        typeStr = K3b::Device::mediaTypeString( info.mediaType() );
    else
        typeStr = i18n("Unknown (probably CD-ROM)");
    s += tableRow( i18n( "Type:" ), typeStr );
    if( Device::isDvdMedia( info.mediaType() ) )
        s += tableRow( i18n("Media ID:"), !info.mediaId().isEmpty() ? QString::fromLatin1( info.mediaId() ) : i18n("unknown") );
    s += tableRow( i18n("Capacity:"), i18n("%1 min",info.capacity().toString()) + " (" + KIO::convertSize(info.capacity().mode1Bytes()) + ')' );
    if( !info.empty() )
        s += tableRow( i18n("Used Capacity:"), i18n("%1 min", medium.actuallyUsedCapacity().toString()) + " (" + KIO::convertSize(medium.actuallyUsedCapacity().mode1Bytes()) + ')' );
    if( !info.empty() ||
        info.appendable() ||
        ( info.mediaType() & ( Device::MEDIA_DVD_PLUS_RW|Device::MEDIA_DVD_RW_OVWR|Device::MEDIA_BD_RE ) ) )
        s += tableRow( i18n("Remaining:"), i18n("%1 min", medium.actuallyRemainingSize().toString() ) + " (" + KIO::convertSize(medium.actuallyRemainingSize().mode1Bytes()) + ')' );
    s += tableRow( i18n("Rewritable:"), info.rewritable() ? i18nc("Availability", "yes") : i18nc("Availability", "no") );
    s += tableRow( i18n("Appendable:"), info.appendable() || ( info.mediaType() & ( Device::MEDIA_DVD_PLUS_RW|Device::MEDIA_DVD_RW_OVWR|Device::MEDIA_BD_RE ) ) ? i18nc("Availability", "yes") : i18nc("Availability", "no") );
    s += tableRow( i18n("Empty:"), info.empty() ? i18nc("Availability", "yes") : i18nc("Availability", "no") );
    if( !( info.mediaType() & K3b::Device::MEDIA_CD_ALL ) )
        s += tableRow( i18nc("Number of layers on an optical medium", "Layers:"), QString::number( info.numLayers() ) );

    if( info.mediaType() == K3b::Device::MEDIA_DVD_PLUS_RW ) {
        QString bgf;
        switch( info.bgFormatState() ) {
        case K3b::Device::BG_FORMAT_NONE:
            bgf = i18n("not formatted");
            break;
        case K3b::Device::BG_FORMAT_INCOMPLETE:
            bgf = i18n("incomplete");
            break;
        case K3b::Device::BG_FORMAT_IN_PROGRESS:
            bgf = i18n("in progress");
            break;
        case K3b::Device::BG_FORMAT_COMPLETE:
            bgf = i18n("complete");
            break;
        default:
            bgf = i18n("unknown state");
        }

        s += tableRow( i18n("Background Format:"), bgf );
    }

    s += tableRow( i18n("Sessions:"), QString::number( info.numSessions() ) );

    if( info.mediaType() & K3b::Device::MEDIA_WRITABLE ) {
        QString speedStr;
        if( medium.writingSpeeds().isEmpty() ) {
            speedStr = '-';
        }
        else {
            foreach( int speed, medium.writingSpeeds() ) {
                if( !speedStr.isEmpty() ) {
                    speedStr.append( "<br>" );
                }

                if( K3b::Device::isCdMedia( info.mediaType() ) )
                    speedStr.append( QString( "%1x (%2 KB/s)" ).arg( speed/K3b::Device::SPEED_FACTOR_CD ).arg( speed ) );
                else if( K3b::Device::isDvdMedia( info.mediaType() ) )
                    speedStr.append( QString().sprintf( "%.1fx (%d KB/s)", (double)speed / ( double )K3b::Device::SPEED_FACTOR_DVD, speed ) );
                else if ( K3b::Device::isBdMedia( info.mediaType() ) )
                    speedStr.append( QString().sprintf( "%.1fx (%d KB/s)", (double)speed / ( double )K3b::Device::SPEED_FACTOR_BD, speed ) );
            }
        }

        s += tableRow( i18n("Supported writing speeds:"), speedStr );
    }

    s += "</table>";

    return s;
}


QString K3b::DiskInfoView::createIso9660InfoItems( const K3b::Iso9660SimplePrimaryDescriptor& iso )
{
    QString s = "<table>";

    s += tableRow( i18n("System Id:"), iso.systemId.isEmpty() ? QString("-") : iso.systemId );
    s += tableRow( i18n("Volume Id:"), iso.volumeId.isEmpty() ? QString("-") : iso.volumeId );
    s += tableRow( i18n("Volume Set Id:"), iso.volumeSetId.isEmpty() ? QString("-") : iso.volumeSetId );
    s += tableRow( i18n("Publisher Id:"), iso.publisherId.isEmpty() ? QString("-") : iso.publisherId );
    s += tableRow( i18n("Preparer Id:"), iso.preparerId.isEmpty() ? QString("-") : iso.preparerId );
    s += tableRow( i18n("Application Id:"), iso.applicationId.isEmpty() ? QString("-") : iso.applicationId );
    s += tableRow( i18n("Volume Size:"),
                   QString( "%1 (%2 * %3 = %4)" )
                   .arg(KIO::convertSize(iso.logicalBlockSize*iso.volumeSpaceSize))
                   .arg(i18nc( "Size of one block, always 2048", "%1 B", iso.logicalBlockSize) )
                   .arg(i18ncp( "Number of blocks (one block has 2048 bytes)", "1 block", "%1 blocks", iso.volumeSpaceSize) )
                   .arg(i18np( "1 B", "%1 B", iso.logicalBlockSize*iso.volumeSpaceSize ) ) );

    s += "</table>";

    return s;
}



