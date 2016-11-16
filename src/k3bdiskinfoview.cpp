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

#include <KIconThemes/KIconLoader>
#include <KI18n/KLocalizedString>
#include <KIOCore/KIO/Global>

#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtGui/QFont>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QPalette>
#include <QtGui/QPixmap>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQuickWidgets/QQuickWidget>

#include "stringtablemodel.h"
#include "stringgridmodel.h"

#define MAKE_STR(arg) MAKE_STR2(arg)
#define MAKE_STR2(arg) #arg
#define MAKE_QML_URL(file) QUrl(QString("file://" MAKE_STR(QML_INSTALL_DIR) "/k3b/" file))

StringTableModel *K3b::DiskInfoView::populateMediumInfoTable(const K3b::Medium& medium)
{
    StringTableModel *table = new StringTableModel();
    
    table->roles()
        << "key"
        << "value";  
    
    const K3b::Device::DiskInfo& info = medium.diskInfo();
    
    table->newRow()
        << i18n("Type:")
        << ( (info.mediaType() == K3b::Device::MEDIA_UNKNOWN) ? i18n("Unknown (probably CD-ROM)") : K3b::Device::mediaTypeString(info.mediaType()) );
    
    if( Device::isDvdMedia(info.mediaType()))
        table->newRow()
            << i18n("Media ID:")
            << ( !info.mediaId().isEmpty() ? QString::fromLatin1(info.mediaId()) : i18n("unknown") );
    
    table->newRow() 
        << i18n("Capacity:")
        << (i18n("%1 min",info.capacity().toString()) + " (" + KIO::convertSize(info.capacity().mode1Bytes()) + ')');
    
    if( !info.empty() )
    {
        table->newRow() << 
            i18n("Used Capacity:") << 
            (i18n("%1 min", medium.actuallyUsedCapacity().toString()) + " (" + KIO::convertSize(medium.actuallyUsedCapacity().mode1Bytes()) + ')');
    }
    
    if( !info.empty() || info.appendable() || ( info.mediaType() & ( Device::MEDIA_DVD_PLUS_RW|Device::MEDIA_DVD_RW_OVWR|Device::MEDIA_BD_RE ) ) )
    {
        table->newRow() 
            << i18n("Remaining:")
            << (i18n("%1 min", medium.actuallyRemainingSize().toString() ) + " (" + KIO::convertSize(medium.actuallyRemainingSize().mode1Bytes()) + ')');
    }
    
    table->newRow()
        << i18n("Rewritable:")
        << ( info.rewritable() ? i18nc("Availability", "yes") : i18nc("Availability", "no") );
    table->newRow()
        << i18n("Appendable:")
        << ( info.appendable() || ( info.mediaType() & ( Device::MEDIA_DVD_PLUS_RW|Device::MEDIA_DVD_RW_OVWR|Device::MEDIA_BD_RE ) ) ? i18nc("Availability", "yes") : i18nc("Availability", "no") );
    table->newRow()
        << i18n("Empty:")
        << ( info.empty() ? i18nc("Availability", "yes") : i18nc("Availability", "no") );

    if( !( info.mediaType() & K3b::Device::MEDIA_CD_ALL ) )
        table->newRow()
            << i18nc("Number of layers on an optical medium", "Layers:")
            << QString::number(info.numLayers());

    if( info.mediaType() == K3b::Device::MEDIA_DVD_PLUS_RW )
    {
        table->newRow() << i18n("Background Format:");
        
        switch( info.bgFormatState() ) 
        {
        case K3b::Device::BG_FORMAT_NONE:
            table->lastRow() << i18n("not formatted");
            break;
        case K3b::Device::BG_FORMAT_INCOMPLETE:
            table->lastRow() << i18n("incomplete");
            break;
        case K3b::Device::BG_FORMAT_IN_PROGRESS:
            table->lastRow() << i18n("in progress");
            break;
        case K3b::Device::BG_FORMAT_COMPLETE:
            table->lastRow() << i18n("complete");
            break;
        default:
            table->lastRow() << i18n("unknown state");
        }
    }

    table->newRow()
        << i18n("Sessions:")
        << QString::number(info.numSessions());

    if( info.mediaType() & K3b::Device::MEDIA_WRITABLE )
    {
        QString speedStr;
        if( medium.writingSpeeds().isEmpty() ) 
        {
            speedStr = '-';
        }
        else 
        {
            foreach( int speed, medium.writingSpeeds() ) 
            {
                if( !speedStr.isEmpty() ) 
                    speedStr.append( ", " );

                if( K3b::Device::isCdMedia( info.mediaType() ) )
                    speedStr.append( QString( "%1x (%2 KB/s)" ).arg( speed/K3b::Device::SPEED_FACTOR_CD ).arg( speed ) );
                else if( K3b::Device::isDvdMedia( info.mediaType() ) )
                    speedStr.append( QString().sprintf( "%.1fx (%d KB/s)", (double)speed / ( double )K3b::Device::SPEED_FACTOR_DVD, speed ) );
                else if ( K3b::Device::isBdMedia( info.mediaType() ) )
                    speedStr.append( QString().sprintf( "%.1fx (%d KB/s)", (double)speed / ( double )K3b::Device::SPEED_FACTOR_BD, speed ) );
            }
        }

        table->newRow()
            << i18n("Supported writing speeds:")
            << speedStr;
    }

    return table;
}

StringTableModel *K3b::DiskInfoView::populateFilesystemInfoTable(const K3b::Iso9660SimplePrimaryDescriptor& iso)
{
    StringTableModel *table = new StringTableModel();

    table->roles() << "key" << "value";    
    
    table->newRow() << i18n("System Id:")       << ( iso.systemId.isEmpty() ? QString("-") : iso.systemId );
    table->newRow() << i18n("Volume Id:")       << ( iso.volumeId.isEmpty() ? QString("-") : iso.volumeId );
    table->newRow() << i18n("Volume Set Id:")   << ( iso.volumeSetId.isEmpty() ? QString("-") : iso.volumeSetId );
    table->newRow() << i18n("Publisher Id:")    << ( iso.publisherId.isEmpty() ? QString("-") : iso.publisherId );
    table->newRow() << i18n("Preparer Id:")     << ( iso.preparerId.isEmpty() ? QString("-") : iso.preparerId );
    table->newRow() << i18n("Application Id:")  << ( iso.applicationId.isEmpty() ? QString("-") : iso.applicationId );
    table->newRow() << i18n("Volume Size:")     << QString("%1 (%2 * %3 = %4)")
                   .arg(KIO::convertSize(iso.logicalBlockSize*iso.volumeSpaceSize))
                   .arg(i18nc( "Size of one block, always 2048", "%1 B", iso.logicalBlockSize) )
                   .arg(i18ncp( "Number of blocks (one block has 2048 bytes)", "1 block", "%1 blocks", iso.volumeSpaceSize) )
                   .arg(i18np( "1 B", "%1 B", iso.logicalBlockSize*iso.volumeSpaceSize ) );
                   
    return table;
}

StringGridModel *K3b::DiskInfoView::populateTracksTable(const K3b::Medium& medium)
{
    StringGridModel *grid = new StringGridModel();
    
    *grid
        << GridItem("<b>"+i18n("Type")+"</b>", 2)
        << GridItem("<b>"+i18n("Attributes")+"</b>", 1)
        << GridItem("<b>"+i18n("First-Last Sector")+"</b>", 1)
        << GridItem("<b>"+i18n("Length")+"</b>", 3);

    int lastSession = 0;
    
    // if we have multiple sessions we create a header item for every session
    if( medium.diskInfo().numSessions() > 1 && medium.toc()[0].session() > 0 ) 
    {
        *grid << GridItem(i18n( "Session %1", 1 ), 7);
        lastSession = 1;
        return grid;
    }
    
    // create items for the tracks
    for( int trackIndex = 0; trackIndex < medium.toc().count(); ++trackIndex ) 
    {
        K3b::Device::Track track = medium.toc()[trackIndex];
        if( medium.diskInfo().numSessions() > 1 && track.session() != lastSession ) 
        {
            lastSession = track.session();
            *grid << GridItem(i18n( "Session %1", lastSession ), 7);
        }
        
        *grid << QString::number(trackIndex+1).rightJustified( 2, ' ' );
        
        if( track.type() == K3b::Device::Track::TYPE_AUDIO )
        {
            *grid << i18n("Audio");
        } 
        else 
        {
            if( track.mode() == K3b::Device::Track::MODE1 )
                *grid << i18n("Data/Mode1");
            else if( track.mode() == K3b::Device::Track::MODE2 )
                *grid << i18n("Data/Mode2");
            else if( track.mode() == K3b::Device::Track::XA_FORM1 )
                *grid << i18n("Data/Mode2 XA Form1");
            else if( track.mode() == K3b::Device::Track::XA_FORM2 )
                *grid << i18n("Data/Mode2 XA Form2");
            else
                *grid << i18n("Data");
        }
        
        *grid << QString( "%1/%2" )
             .arg( track.copyPermitted() ? i18n("copy") : i18n("no copy") )
             .arg( track.type() == K3b::Device::Track::TYPE_AUDIO
                   ? ( track.preEmphasis() ?  i18n("preemp") : i18n("no preemp") )
                   : ( track.recordedIncremental() ?  i18n("incremental") : i18n("uninterrupted") ) );

        *grid << QString("%1 - %2")
             .arg(track.firstSector().lba())
             .arg(track.lastSector().lba());
             
        *grid << QString::number( track.length().lba() ) + " (" + track.length().toString() + ")";
        
        *grid << QString( "%1 (%2)" ).arg(track.index0().toString()).arg(track.index0().lba());
        
        if( !medium.cdText().isEmpty() )
        {
            K3b::Device::TrackCdText text = medium.cdText().track( trackIndex );
            *grid << text.performer() + " - " + text.title() + " (" + i18n("CD-Text") + ')';
        }
        else
        {
            KCDDB::TrackInfo info = medium.cddbInfo().track( trackIndex );
            *grid << info.get( KCDDB::Artist ).toString() + " - " + info.get( KCDDB::Title ).toString();
        }
    }
    
    return grid;
}

void K3b::DiskInfoView::populateQmlContext(QQmlContext* context)
{
    bool isMediumPresent = medium().diskInfo().diskState() != K3b::Device::STATE_NO_MEDIA;
    bool isIsoFilesystem = false;
    bool isTocNotEmpty = false;

    if(isMediumPresent)
    {    
        const K3b::Medium& med = medium();          
        swapProperty("mediumTable", &mediumTable, populateMediumInfoTable(med));
        
        isIsoFilesystem = med.content() & K3b::Medium::ContentData;        
        if(isIsoFilesystem)        
            swapProperty("filesystemInfoTable", &filesystemInfoTable, populateFilesystemInfoTable(med.iso9660Descriptor()));
        
        isTocNotEmpty = !med.toc().isEmpty();
        if(isTocNotEmpty)
            swapProperty("tracksGrid", &tracksGrid, populateTracksTable(med));
    }
    
    context->setContextProperty(QString("isMediumPresent"), QVariant::fromValue(isMediumPresent));
    context->setContextProperty(QString("isIsoFilesystem"), QVariant::fromValue(isIsoFilesystem));
    context->setContextProperty(QString("isTocNotEmpty"), QVariant::fromValue(isTocNotEmpty)); 
    
    context->setContextProperty(QString("noMediumLabel"), QVariant(i18n("No medium present")));
    context->setContextProperty(QString("mediumLabel"), QVariant(i18n("Medium")));
    context->setContextProperty(QString("isoLabel"), QVariant(i18n("ISO9660 Filesystem Info")));
    context->setContextProperty(QString("tracksLabel"), QVariant(i18n("Tracks")));
}

template<class T> void K3b::DiskInfoView::swapProperty(const char* key, T **itemPtr, T *newItem)
{
    m_qmlEngine->rootContext()->setContextProperty(key, newItem);
    delete *itemPtr;
    *itemPtr=newItem;
}

K3b::DiskInfoView::DiskInfoView( QWidget* parent )
    : K3b::MediaContentsView( true,
                              Medium::ContentAll,
                              Device::MEDIA_ALL|Device::MEDIA_NONE|Device::MEDIA_UNKNOWN,
                              Device::STATE_ALL|Device::STATE_NO_MEDIA|Device::STATE_UNKNOWN,
                              parent )
{
    m_qmlEngine = new QQmlApplicationEngine();
    mediumTable = NULL;
    filesystemInfoTable = NULL;
    tracksGrid = NULL;

    populateQmlContext(m_qmlEngine->rootContext());
    
    m_infoView = new QQuickWidget(m_qmlEngine, this );
    
    m_infoView->setSource(MAKE_QML_URL("diskinfo.qml"));

    m_infoView->setMinimumHeight(this->height());

    m_infoView->setResizeMode(QQuickWidget::SizeRootObjectToView);

    setMainWidget( m_infoView );
}

K3b::DiskInfoView::~DiskInfoView()
{
    delete m_qmlEngine;
    delete mediumTable;
    delete filesystemInfoTable;
    delete tracksGrid;
}

void K3b::DiskInfoView::reloadMedium()
{
    updateTitle();
    
    populateQmlContext(m_infoView->rootContext());

    m_infoView->setMinimumHeight(this->height());
    
    m_infoView->update();
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
