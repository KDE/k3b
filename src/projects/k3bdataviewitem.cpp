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

#include "k3bdataviewitem.h"

#include "k3bfileitem.h"
#include "k3bdiritem.h"
#include "k3bspecialdataitem.h"
#include "k3bsessionimportitem.h"
#include "k3bdatadoc.h"
#include <k3bvalidators.h>

#include <kio/global.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kurl.h>
#include <KIcon>

#include <qpainter.h>
#include <qpalette.h>
#include <qfileinfo.h>


K3b::DataViewItem::DataViewItem( K3b::DataItem* item, Q3ListView* parent )
    : K3b::ListViewItem( parent ),
      m_dataItem(item)
{
    init();
}

K3b::DataViewItem::DataViewItem( K3b::DataItem* item, Q3ListViewItem* parent )
    : K3b::ListViewItem( parent ),
      m_dataItem(item)
{
    init();
}

K3b::DataViewItem::~DataViewItem()
{}


void K3b::DataViewItem::init()
{
    setEditor( 0, LINE );
    static QValidator* s_validator = K3b::Validators::iso9660Validator();
    setValidator( 0, s_validator );
}

void K3b::DataViewItem::paintCell( QPainter* p, const QColorGroup& cg, int column, int width, int align )
{
    QColorGroup _cg = cg;

    if( !dataItem()->isRemoveable() && dataItem()->doc()->root() != dataItem() ) {
        _cg.setColor( QColorGroup::Text, listView()->palette().color( QPalette::Disabled, QPalette::Text ) );
    }

    if( column == 0 ) {
        QFontMetrics fm = p->fontMetrics();
#ifdef __GNUC__
#warning Paint nicer thingies for rockridge and joliet hiding
#endif
//         if( dataItem()->hideOnRockRidge() ) {
//             int tw = qMax( fm.width( "rr" ) + 2*listView()->itemMargin(), height() );
//             p->fillRect( width-tw, 0, tw, height(), isSelected() ? _cg.highlight() : _cg.brush(QColorGroup::Base) );
//             p->setPen( isSelected() ? _cg.highlightedText() : Qt::red );
//             p->drawEllipse( width-tw, 0, tw, height() );
//             p->drawText( width-tw, 0, tw, height(), Qt::AlignCenter, "rr" );
//             width -= tw;
//         }

//         if( dataItem()->hideOnJoliet() ) {
//             int tw = qMax( fm.width( "j" ) + 2*listView()->itemMargin(), height() );
//             p->fillRect( width-tw, 0, tw, height(), isSelected() ? _cg.highlight() : _cg.brush(QColorGroup::Base) );
//             p->setPen( isSelected() ? _cg.highlightedText() : Qt::blue );
//             p->drawEllipse( width-tw, 0, tw, height() );
//             p->drawText( width-tw, 0, tw, height(), Qt::AlignCenter, "j" );
//             width -= tw;
//         }
    }
    else if( column == 4 ) {
        if( dataItem()->isSymLink() ) {
            if( !dataItem()->doc()->isoOptions().followSymbolicLinks() &&
                dataItem()->doc()->isoOptions().createRockRidge() &&
                !dataItem()->isValid() ) {
                // paint the link in red
                _cg.setColor( QColorGroup::Text, Qt::red );
            }
        }
    }

    K3b::ListViewItem::paintCell( p, _cg, column, width, align );
}


void K3b::DataViewItem::setText( int col, const QString& text )
{
    if( col == 0 && dataItem()->isRenameable() ) {
        dataItem()->setK3bName( text );
    }
    K3b::ListViewItem::setText( col, text );
}


QString K3b::DataViewItem::key( int col, bool a ) const
{
    if( col == 2 ) {
        // to have correct sorting we need to justify the size in bytes
        // 100 TB should be enough for the next year... ;-)

        if( a )
            return ( dataItem()->isDir() ? QString("0") : QString("1") )
                + QString::number( dataItem()->size() ).rightJustified( 16, '0' );
        else
            return ( dataItem()->isDir() ? QString("1") : QString("0") )
                + QString::number( dataItem()->size() ).rightJustified( 16, '0' );
    }

    if( a )
        return ( dataItem()->isDir() ? QString("0") : QString("1") ) + text(col);
    else
        return ( dataItem()->isDir() ? QString("1") : QString("0") ) + text(col);
}


K3b::DataDirViewItem::DataDirViewItem( K3b::DirItem* dir, Q3ListView* parent )
    : K3b::DataViewItem( dir, parent )
{
    m_dirItem = dir;
    setPixmap( 0, dir->depth() > 7 ? SmallIcon( "folder-root" ) : SmallIcon( "folder" ) );
}


K3b::DataDirViewItem::DataDirViewItem( K3b::DirItem* dir, Q3ListViewItem* parent )
    : K3b::DataViewItem( dir, parent )
{
    m_dirItem = dir;
    setPixmap( 0, dir->depth() > 7 ? SmallIcon( "folder-root" ) : SmallIcon( "folder" ) );
}


K3b::DataDirViewItem::~DataDirViewItem()
{
}


void K3b::DataDirViewItem::dragEntered()
{
    setOpen( true );
}


QString K3b::DataDirViewItem::text( int index ) const
{
    switch( index ) {
    case 0:
        return m_dirItem->k3bName();
    case 1:
        return i18n("Directory");
    case 2:
        return KIO::convertSize( m_dirItem->size() );
    default:
        return "";
    }
}


void K3b::DataDirViewItem::highlightIcon( bool b )
{
    if( m_pixmap.isNull() )
        m_pixmap = *pixmap(0);
#ifdef __GNUC__
#warning TODO port me to kde4
#endif
#if 0
    if( b )
        setPixmap( 0, KPixmapEffect::selectedPixmap( m_pixmap, listView()->colorGroup().highlight() ) );
    else
#endif
        setPixmap( 0, m_pixmap );
}



K3b::DataFileViewItem::DataFileViewItem( K3b::FileItem* file, Q3ListView* parent )
    : K3b::DataViewItem( file, parent )
{
    init( file );
}


K3b::DataFileViewItem::DataFileViewItem( K3b::FileItem* file, Q3ListViewItem* parent )
    : K3b::DataViewItem( file, parent )
{
    init( file );
}


void K3b::DataFileViewItem::init( K3b::FileItem* file )
{
    m_fileItem = file;

    // determine the mimetype
    m_pMimeType = KMimeType::findByUrl( KUrl(file->localPath()) );
    if( !m_pMimeType )
        setPixmap( 0, KIcon( "unknown" ).pixmap( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
    else
        setPixmap( 0, KIcon( m_pMimeType->iconName() ).pixmap( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
}


QString K3b::DataFileViewItem::text( int index ) const
{
    switch( index ) {
    case 0:
        return m_fileItem->k3bName();
    case 1:
    {
        QString comment = m_pMimeType->comment();
        if( comment.isEmpty() )
            comment = m_pMimeType->name();

        if( m_fileItem->isSymLink() )
            return i18n("Link to %1",comment);
        else
            return comment;
    }
    case 2:
        return KIO::convertSize( m_fileItem->size() );
    case 3:
        return m_fileItem->localPath();
    case 4: {
        if( !m_fileItem->isSymLink() ) {
            return QString();
        }

        QString s;
        if ( m_fileItem->doc()->isoOptions().followSymbolicLinks() ) {
            s = K3b::resolveLink( m_fileItem->localPath() );
        }
        else {
            s = QFileInfo( m_fileItem->localPath() ).readLink();
        }

        if( !m_fileItem->isValid() ) {
            s += " (" + i18n("outside of project") + ")";
        }

        return s;
    }
    default:
        return "";
    }
}



K3b::DataRootViewItem::DataRootViewItem( K3b::DataDoc* doc, Q3ListView* parent )
    : K3b::DataDirViewItem( doc->root(), parent )
{
    m_doc = doc;
    setPixmap( 0, SmallIcon( "media-optical" ) );
    setValidator( 0, new K3b::Latin1Validator() );
}


K3b::DataRootViewItem::~DataRootViewItem()
{
    delete validator(0);
}


QString K3b::DataRootViewItem::text( int index ) const
{
    switch( index ) {
    case 0:
        return ( m_doc->isoOptions().volumeID().isEmpty() ? i18n("root") : m_doc->isoOptions().volumeID() );
    default:
        return "";
    }
}


void K3b::DataRootViewItem::setText( int col, const QString& text )
{
    if( col == 0 )
        m_doc->setVolumeID( text );

    K3b::DataViewItem::setText( col, text );
}


K3b::SpecialDataViewItem::SpecialDataViewItem( K3b::SpecialDataItem* item, Q3ListView* parent )
    : K3b::DataViewItem( item, parent )
{
    setPixmap( 0, SmallIcon("unknown") );
}

QString K3b::SpecialDataViewItem::text( int col ) const
{
    switch( col ) {
    case 0:
        return dataItem()->k3bName();
    case 1:
        return ((K3b::SpecialDataItem*)dataItem())->mimeType();
    case 2:
        return KIO::convertSize( dataItem()->size() );
    default:
        return "";
    }
}



K3b::SessionImportViewItem::SessionImportViewItem( K3b::SessionImportItem* item, Q3ListView* parent )
    : K3b::DataViewItem( item, parent )
{
    setPixmap( 0, SmallIcon("unknown") );
}

QString K3b::SessionImportViewItem::text( int col ) const
{
    switch( col ) {
    case 0:
        return dataItem()->k3bName();
    case 1:
        return i18n("From previous session");
    case 2:
        return KIO::convertSize( dataItem()->size() );
    default:
        return "";
    }
}
