/*
*
* $Id$
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
*
* This file is part of the K3b project.
* Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* See the file "COPYING" for the exact licensing terms.
*/

// kde includes
#include <kaction.h>
#include <kcutlabel.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kstdaction.h>

// qt includes
#include <qfont.h>
#include <qframe.h>
#include <qheader.h>
#include <qlabel.h>
#include <qlayout.h>

// k3b includes
#include "k3bvideocdview.h"
#include "k3bvideocdrippingdialog.h"

#include <k3bdevice.h>
#include <k3bmsf.h>
#include <k3btoc.h>
#include <k3bcore.h>
#include <k3blistview.h>
#include <k3bstdguiitems.h>
#include <k3btoolbox.h>


class K3bVideoCdView::VideoTrackViewItem : public QListViewItem
{
    public:
        VideoTrackViewItem( QListViewItem* parent, QListViewItem* after )
                : QListViewItem( parent, after )
        {
            setSelectable( false );
        }

        VideoTrackViewItem( QListView* parent, QListViewItem* after )
                : QListViewItem( parent, after )
        {
            setSelectable( false );
        }
        
        VideoTrackViewItem( QListViewItem* parent,
                            QString name,
                            QString id,
                            int _trackNumber,
                            const K3b::Msf& length )
                : QListViewItem( parent )
        {
            setText( 0, QString( "%1. %2" ).arg( _trackNumber ).arg( id ) );
            setText( 1, name );
            if ( length > 0 ) {
                setText( 2, length.toString() );
                setText( 3, KIO::convertSize( length.mode2Form2Bytes() ) );
            }

            trackNumber = _trackNumber;
            setSelectable( false );
        }

        int trackNumber;

        void updateData( const K3bVideoCdInfoResultEntry& resultEntry )
        {
            setText( 0, QString( "%1. %2" ).arg( trackNumber ).arg( resultEntry.id ) );
            setText( 1, resultEntry.name );
        }

};

class K3bVideoCdView::VideoTrackViewCheckItem : public QCheckListItem
{
    public:
        VideoTrackViewCheckItem( QListViewItem* parent,
                                 QString desc )
                : QCheckListItem( parent,
                                  QString::null,
                                  QCheckListItem::CheckBox )
        {
            setText( 0, desc );

            setOn( true );
        }

        VideoTrackViewCheckItem( QListView* parent,
                                 QString desc )
                : QCheckListItem( parent,
                                  QString::null,
                                  QCheckListItem::CheckBox )
        {
            setText( 0, desc );

            setOn( true );
        }

        VideoTrackViewCheckItem( VideoTrackViewCheckItem* parent,
                                 QString desc )
                : QCheckListItem( parent,
                                  QString::null,
                                  QCheckListItem::CheckBox )
        {
            setText( 0, desc );

            setOn( true );
        }

        void updateData( const K3b::Msf& length, bool form2 = false )
        {
            setText( 2, length.toString() );
            if ( form2 )
                setText( 3, KIO::convertSize( length.mode2Form2Bytes() ) );
            else
                setText( 3, KIO::convertSize( length.mode2Form1Bytes() ) );
        }

};

K3bVideoCdView::K3bVideoCdView( QWidget* parent, const char *name )
        : K3bCdContentsView( true, parent, name )
{
    QGridLayout * mainGrid = new QGridLayout( mainWidget() );

    // toolbox
    // ----------------------------------------------------------------------------------
    QHBoxLayout* toolBoxLayout = new QHBoxLayout( 0, 0, 0, "toolBoxLayout" );
    m_toolBox = new K3bToolBox( mainWidget() );
    toolBoxLayout->addWidget( m_toolBox );
    QSpacerItem* spacer = new QSpacerItem( 10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum );
    toolBoxLayout->addItem( spacer );
    m_labelLength = new QLabel( mainWidget() );
    m_labelLength->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );
    toolBoxLayout->addWidget( m_labelLength );

    // the track view
    // ----------------------------------------------------------------------------------
    m_trackView = new K3bListView( mainWidget() );
    m_trackView->setFullWidth( true );
    m_trackView->setAllColumnsShowFocus( true );
    m_trackView->setSelectionMode( QListView::Single );
    m_trackView->setDragEnabled( true );
    m_trackView->addColumn( i18n( "Item Name" ) );
    m_trackView->addColumn( i18n( "Extracted Name" ) );
    m_trackView->addColumn( i18n( "Length" ) );
    m_trackView->addColumn( i18n( "Size" ) );

    m_trackView->header() ->setClickEnabled( false );

    m_trackView->setItemsRenameable( false );
    m_trackView->setRootIsDecorated( true );

    connect( m_trackView, SIGNAL( contextMenu( KListView*, QListViewItem*, const QPoint& ) ),
             this, SLOT( slotContextMenu( KListView*, QListViewItem*, const QPoint& ) ) );
    connect( m_trackView, SIGNAL( selectionChanged( QListViewItem* ) ),
             this, SLOT( slotTrackSelectionChanged( QListViewItem* ) ) );
    connect( m_trackView, SIGNAL( clicked( QListViewItem* ) ),
             this, SLOT( slotStateChanged( QListViewItem* ) ) );
    connect( m_trackView, SIGNAL( spacePressed( QListViewItem* ) ),
             this, SLOT( slotStateChanged( QListViewItem* ) ) );


    mainGrid->addLayout( toolBoxLayout, 0, 0 );
    mainGrid->addWidget( m_trackView, 1, 0 );

    initActions();
    slotTrackSelectionChanged( 0 );

    m_videocdinfo = 0L;
    m_videooptions = new K3bVideoCdRippingOptions();
    
    m_contentList.clear();
}


K3bVideoCdView::~K3bVideoCdView()
{
    delete m_videocdinfo;
    delete m_videooptions;
}


void K3bVideoCdView::setDisk( const K3bMedium& medium )
{
  m_toc = medium.toc();
    m_device = medium.device();

    m_trackView->clear();
    enableInteraction( false );

    m_contentList.append( new VideoTrackViewCheckItem( m_trackView, i18n("Video CD MPEG tracks") ) );
    m_contentList.append( new VideoTrackViewCheckItem( m_trackView, i18n("Video CD DATA track" ) ) );

    ( ( VideoTrackViewCheckItem* ) m_contentList[ 0 ] ) ->setOpen( true );

    // create a listviewItem for every video track
    int index = 0;
    m_videocddatasize = 0;
    m_videocdmpegsize = 0;
            
    K3b::Msf sequenceSize;

    for ( K3bDevice::Toc::const_iterator it = m_toc.begin();
            it != m_toc.end(); ++it ) {

        if ( index > 0 ) {
            K3b::Msf length( ( *it ).length() );
            sequenceSize += length;
            m_videocdmpegsize += length.mode2Form2Bytes();
            ( void ) new VideoTrackViewItem( ( VideoTrackViewCheckItem* ) m_contentList[ 0 ], i18n( "Sequence-%1" ).arg( index ), "", index, length );
        } else {
            K3b::Msf length( ( *it ).length() );
            m_videocddatasize += length.mode2Form1Bytes();
            ( ( VideoTrackViewCheckItem* ) m_contentList[ 1 ] ) ->updateData( length );
            ( void ) new VideoTrackViewCheckItem( ( VideoTrackViewCheckItem* ) m_contentList[ 1 ], i18n( "Files" ) );
            ( void ) new VideoTrackViewCheckItem( ( VideoTrackViewCheckItem* ) m_contentList[ 1 ], i18n( "Segments" ) );
        }

        index++;
    }

    ( ( VideoTrackViewCheckItem* ) m_contentList[ 0 ] ) ->updateData( sequenceSize, true );

    m_videooptions ->setVideoCdSource( m_device->devicename() );
    
    m_videocdinfo = new K3bVideoCdInfo( this );
    m_videocdinfo->info( m_device->devicename() );

    connect( m_videocdinfo, SIGNAL( infoFinished( bool ) ),
             this, SLOT( slotVideoCdInfoFinished( bool ) ) );

}

void K3bVideoCdView::slotVideoCdInfoFinished( bool success )
{

    if ( success ) {
        m_videocdinfoResult = m_videocdinfo->result();
        updateDisplay();
    }

    enableInteraction( true );
}

void K3bVideoCdView::updateDisplay()
{
    // update the listview

    VideoTrackViewItem * item = ( VideoTrackViewItem* ) m_contentList[ 0 ] ->firstChild();
    int index = 0;
    while ( item ) {
        item->updateData( m_videocdinfoResult.entry( index, K3bVideoCdInfoResult::SEQUENCE ) );
        item = ( VideoTrackViewItem* ) item->nextSibling();
        index++;
    }

    VideoTrackViewCheckItem* check_item = ( VideoTrackViewCheckItem* ) m_contentList[ 1 ] ->firstChild();
    while ( check_item ) {
        if ( check_item->key( 0, false ).compare( i18n( "Files" ) ) == 0 ) {
            if ( domTree.setContent( m_videocdinfoResult.xmlData ) ) {

                QDomElement root = domTree.documentElement();
                QDomNode node;
                node = root.firstChild();
                while ( !node.isNull() ) {
                    if ( node.isElement() && node.nodeName() == "filesystem" ) {
                        QDomElement body = node.toElement();
                        buildTree( check_item, body );
                        break;
                    }
                    node = node.nextSibling();
                }
            }
        } else {
            for ( index = 0; index < m_videocdinfoResult.foundEntries( K3bVideoCdInfoResult::SEGMENT ); index++ ) {
                ( void ) new VideoTrackViewItem( check_item, m_videocdinfoResult.entry( index, K3bVideoCdInfoResult::SEGMENT ).name, m_videocdinfoResult.entry( index, K3bVideoCdInfoResult::SEGMENT ).id , index + 1, 0 );
            }
        }
        check_item = ( VideoTrackViewCheckItem* ) check_item->nextSibling();
    }

    if ( !m_videocdinfoResult.volumeId.isEmpty() ) {
        QString description = m_videocdinfoResult.volumeId + " (" + m_videocdinfoResult.type + " " + m_videocdinfoResult.version + ")" ;
        setTitle( description );
        m_videooptions ->setVideoCdDescription( description );
    }
    else
        setTitle( i18n( "Video CD" ) );

    m_labelLength->setText( i18n( "1 track (%1)", "%n tracks (%1)", m_toc.count() ).arg( K3b::Msf( m_toc.length() ).toString() ) );
}


void K3bVideoCdView::initActions()
{
    m_actionCollection = new KActionCollection( this );

    KAction* actionSelectAll = KStdAction::selectAll( this, SLOT( slotSelectAll() ),
                               m_actionCollection, "select_all" );
    KAction* actionDeselectAll = KStdAction::deselect( this, SLOT( slotDeselectAll() ),
                                 m_actionCollection, "deselect_all" );
    actionDeselectAll->setText( i18n( "Dese&lect All" ) );
    KAction* actionSelect = new KAction( i18n( "Select Track" ), 0, 0, this,
                                         SLOT( slotSelect() ), actionCollection(),
                                         "select_track" );
    KAction* actionDeselect = new KAction( i18n( "Deselect Track" ), 0, 0, this,
                                           SLOT( slotDeselect() ), actionCollection(),
                                           "deselect_track" );

    KAction* actionStartRip = new KAction( i18n( "Start Ripping" ), "run", 0, this,
                                           SLOT( startRip() ), actionCollection(), "start_rip" );

    // TODO: set the actions tooltips and whatsthis infos

    // setup the popup menu
    m_popupMenu = new KActionMenu( actionCollection(), "popup_menu" );
    KAction* separator = new KActionSeparator( actionCollection(), "separator" );
    m_popupMenu->insert( actionSelect );
    m_popupMenu->insert( actionDeselect );
    m_popupMenu->insert( actionSelectAll );
    m_popupMenu->insert( actionDeselectAll );
    m_popupMenu->insert( separator );
    m_popupMenu->insert( actionStartRip );

    // setup the toolbox
    m_toolBox->addButton( actionStartRip );
}


void K3bVideoCdView::slotContextMenu( KListView*, QListViewItem*, const QPoint& p )
{
    m_popupMenu->popup( p );
}


void K3bVideoCdView::slotTrackSelectionChanged( QListViewItem* item )
{
    actionCollection() ->action( "select_track" ) ->setEnabled( item != 0 );
    actionCollection() ->action( "deselect_track" ) ->setEnabled( item != 0 );
}

void K3bVideoCdView::slotStateChanged( QListViewItem* item )
{
    /* > QT 3.1
    if ( !item == 0 && item ->isSelectable() ) {
        if ( ( ( VideoTrackViewCheckItem* ) item) ->state() == QCheckListItem::On)
            slotSelect();
        else if ( ( ( VideoTrackViewCheckItem* ) item) ->state() == QCheckListItem::Off)
            slotDeselect();
    }
    */
    if ( !item == 0 && item ->isSelectable() ) {
        if ( ( ( VideoTrackViewCheckItem* ) item) ->isOn() )
            slotSelect();
        else
            slotDeselect();
    }
}

void K3bVideoCdView::startRip()
{

    int selectedItems  = 0;
    for ( QListViewItemIterator it( m_trackView ); it.current(); ++it ) {
        if ( it.current() ->isSelectable() ) {
            if ( ( ( ( VideoTrackViewCheckItem* ) it.current()) ->key( 0, false ).compare( i18n("Video CD MPEG tracks" ) ) == 0 ) && ( ( VideoTrackViewCheckItem* ) it.current() ) ->isOn() ) {
                m_videooptions ->setVideoCdRipSequences( true );
                selectedItems++;
            }
            else if ( ( ( ( VideoTrackViewCheckItem* ) it.current()) ->key( 0, false ).compare( i18n("Files" ) ) == 0 ) && ( ( VideoTrackViewCheckItem* ) it.current() ) ->isOn() ) {
                m_videooptions ->setVideoCdRipFiles( true );
                selectedItems++;
            }
            else if ( ( ( ( VideoTrackViewCheckItem* ) it.current()) ->key( 0, false ).compare( i18n("Segments" ) ) == 0 ) && ( ( VideoTrackViewCheckItem* ) it.current() ) ->isOn() ) {
                m_videooptions ->setVideoCdRipSegments( true );
                selectedItems++;
            }
        }
    }

    if( selectedItems == 0 ) {
        KMessageBox::error( this, i18n("Please select the tracks to rip."), i18n("No Tracks Selected") );
    }
    else {
        unsigned long videocdsize = 0;
        // TODO: split SegmentSize and FileSize. Have no infos now
        if ( m_videooptions ->getVideoCdRipSegments() || m_videooptions ->getVideoCdRipFiles())
            videocdsize += m_videocddatasize;
        if ( m_videooptions ->getVideoCdRipSequences() )
            videocdsize += m_videocdmpegsize;

        kdDebug() << QString("(K3bVideoCdView::startRip())  m_videooptions ->setVideoCdSize( %1)").arg( videocdsize ) << endl;
        m_videooptions ->setVideoCdSize( videocdsize );
        K3bVideoCdRippingDialog rip( m_videooptions, this );
        rip.exec();
    }
}

void K3bVideoCdView::slotSelectAll()
{
    for ( QListViewItemIterator it( m_trackView ); it.current(); ++it )
        if ( it.current() ->isSelectable() )
            ( ( VideoTrackViewCheckItem* ) it.current() ) ->setOn( true );
}

void K3bVideoCdView::slotDeselectAll()
{
    for ( QListViewItemIterator it( m_trackView ); it.current(); ++it )
        if ( it.current() ->isSelectable() )
            ( ( VideoTrackViewCheckItem* ) it.current() ) ->setOn( false );
}

void K3bVideoCdView::slotSelect()
{
    if ( QListViewItem * sel = m_trackView->selectedItem() ) {
        ( ( VideoTrackViewCheckItem* ) sel) ->setOn( true );
        QListViewItem * item = sel ->firstChild();
        while ( item ) {
            if ( item ->isSelectable() )
                ( ( VideoTrackViewCheckItem* ) item) ->setOn( true );

            item = item->nextSibling();
        }
    }
}

void K3bVideoCdView::slotDeselect()
{
    if ( QListViewItem * sel = m_trackView->selectedItem() ) {
        ( ( VideoTrackViewCheckItem* ) sel) ->setOn( false );
        QListViewItem * item = sel ->firstChild();
        while ( item ) {
            if ( item ->isSelectable() )
                ( ( VideoTrackViewCheckItem* ) item) ->setOn( false );

            item = item->nextSibling();
        }
    }
}

void K3bVideoCdView::enableInteraction( bool b )
{
    m_trackView->setEnabled( b );
    m_toolBox->setEnabled( b );
}

void K3bVideoCdView::buildTree( QListViewItem *parentItem, const QDomElement &parentElement, QString pname )
{
    VideoTrackViewItem * thisItem = 0;
    QDomNode node = parentElement.firstChild();

    while ( !node.isNull() ) {
        if ( node.isElement() && node.nodeName() == "folder" || node.nodeName() == "file" ) {
            if ( parentItem == 0 )
                thisItem = new VideoTrackViewItem( m_trackView, thisItem );
            else
                thisItem = new VideoTrackViewItem( parentItem, thisItem );

            QString txt = node.firstChild().toElement().text();
            thisItem->setText( 0, txt);
            if ( node.nodeName() == "folder" ) {
                pname += "_" + txt.lower();
            }
            else {
                thisItem->setText( 1, pname + "_" + txt.lower() );
            }

            buildTree( thisItem, node.toElement(), pname );
        } else if ( node.isElement() && node.nodeName() == "segment-item" || node.nodeName() == "sequence-item" ) {
            if ( parentItem == 0 )
                thisItem = new VideoTrackViewItem( m_trackView, thisItem );
            else
                thisItem = new VideoTrackViewItem( parentItem, thisItem );

            thisItem->setText( 0, node.toElement().attribute( "src" ) );

            buildTree( thisItem, node.toElement() );
        }

        node = node.nextSibling();
    }
}

#include "k3bvideocdview.moc"
