/*
*
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
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

// k3b includes
 #include "k3bvideocdview.h"
#include "k3bvideocdrippingdialog.h"

#include "k3bdevice.h"
#include "k3bmsf.h"
#include "k3btoc.h"
#include "k3bcore.h"
#include "k3blistview.h"
#include "k3bstdguiitems.h"
#include "k3baction.h"


// kde includes
#include <kaction.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kstandardaction.h>
#include <KActionMenu>
#include <KActionCollection>
#include <KToolBar>
#include <KMenu>

// qt includes
#include <qfont.h>
#include <q3header.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qcursor.h>
#include <qapplication.h>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QStyle>

class K3b::VideoCdView::VideoTrackViewItem : public Q3ListViewItem
{
public:
    VideoTrackViewItem( Q3ListViewItem* parent, Q3ListViewItem* after )
        : Q3ListViewItem( parent, after )
    {
        setSelectable( false );
    }

    VideoTrackViewItem( Q3ListView* parent, Q3ListViewItem* after )
        : Q3ListViewItem( parent, after )
    {
        setSelectable( false );
    }

    VideoTrackViewItem( Q3ListViewItem* parent,
                        const QString& name,
                        const QString& id,
                        int _trackNumber,
                        const K3b::Msf& length )
        : Q3ListViewItem( parent )
    {
        Q_UNUSED( name );
        setText( 0, QString( "%1. %2" ).arg( _trackNumber ).arg( id ) );
        setText( 1,"" );
        if ( length > 0 ) {
            setText( 2, length.toString() );
            setText( 3, KIO::convertSize( length.mode2Form2Bytes() ) );
        }

        trackNumber = _trackNumber;
        setSelectable( false );
    }

    int trackNumber;

    void updateData( const K3b::VideoCdInfoResultEntry& resultEntry )
    {
        setText( 0, QString( "%1. %2" ).arg( trackNumber ).arg( resultEntry.id ) );
        setText( 1, resultEntry.name );
    }

};

class K3b::VideoCdView::VideoTrackViewCheckItem : public Q3CheckListItem
{
public:
    VideoTrackViewCheckItem( Q3ListViewItem* parent,
                             const QString& desc )
        : Q3CheckListItem( parent,
                           QString(),
                           Q3CheckListItem::CheckBox )
    {
        setText( 0, desc );

        setOn( true );
    }

    VideoTrackViewCheckItem( Q3ListView* parent,
                             const QString& desc )
        : Q3CheckListItem( parent,
                           QString(),
                           Q3CheckListItem::CheckBox )
    {
        setText( 0, desc );

        setOn( true );
    }

    VideoTrackViewCheckItem( VideoTrackViewCheckItem* parent,
                             const QString& desc )
        : Q3CheckListItem( parent,
                           QString(),
                           Q3CheckListItem::CheckBox )
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

K3b::VideoCdView::VideoCdView( QWidget* parent )
    : K3b::MediaContentsView( true,
                            K3b::Medium::ContentVideoCD,
                            K3b::Device::MEDIA_CD_ALL,
                            K3b::Device::STATE_INCOMPLETE|K3b::Device::STATE_COMPLETE,
                            parent )
{
    QGridLayout * mainGrid = new QGridLayout( mainWidget() );

    // toolbox
    // ----------------------------------------------------------------------------------
    QHBoxLayout* toolBoxLayout = new QHBoxLayout;
    m_toolBox = new KToolBar( mainWidget() );
    toolBoxLayout->addWidget( m_toolBox );
    toolBoxLayout->addStretch( 0 );
    m_labelLength = new QLabel( mainWidget() );
    m_labelLength->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
    toolBoxLayout->addWidget( m_labelLength );
    toolBoxLayout->addSpacing( style()->pixelMetric( QStyle::PM_LayoutRightMargin ) );

    // the track view
    // ----------------------------------------------------------------------------------
    m_trackView = new K3b::ListView( mainWidget() );
    m_trackView->setFullWidth( true );
    m_trackView->setAllColumnsShowFocus( true );
    m_trackView->setSelectionMode( Q3ListView::Single );
    m_trackView->setDragEnabled( true );
    m_trackView->addColumn( i18n( "Item Name" ) );
    m_trackView->addColumn( i18n( "Extracted Name" ) );
    m_trackView->addColumn( i18n( "Length" ) );
    m_trackView->addColumn( i18n( "Size" ) );

    m_trackView->header() ->setClickEnabled( false );

    m_trackView->setItemsRenameable( false );
    m_trackView->setRootIsDecorated( true );

    connect( m_trackView, SIGNAL( contextMenu( K3ListView*, Q3ListViewItem*, const QPoint& ) ),
             this, SLOT( slotContextMenu( K3ListView*, Q3ListViewItem*, const QPoint& ) ) );
    connect( m_trackView, SIGNAL( selectionChanged( Q3ListViewItem* ) ),
             this, SLOT( slotTrackSelectionChanged( Q3ListViewItem* ) ) );
    connect( m_trackView, SIGNAL( clicked( Q3ListViewItem* ) ),
             this, SLOT( slotStateChanged( Q3ListViewItem* ) ) );
    connect( m_trackView, SIGNAL( spacePressed( Q3ListViewItem* ) ),
             this, SLOT( slotStateChanged( Q3ListViewItem* ) ) );


    mainGrid->addLayout( toolBoxLayout, 0, 0 );
    mainGrid->addWidget( m_trackView, 1, 0 );
    mainGrid->setSpacing( 0 );
    mainGrid->setMargin( 0 );

    initActions();
    slotTrackSelectionChanged( 0 );

    m_videocdinfo = 0L;
    m_videooptions = new K3b::VideoCdRippingOptions();

    m_contentList.clear();
}


K3b::VideoCdView::~VideoCdView()
{
    delete m_videocdinfo;
    delete m_videooptions;
}


void K3b::VideoCdView::reloadMedium()
{
    m_toc = medium().toc();

    m_trackView->clear();

    m_trackView->setEnabled( false );
    m_toolBox->setEnabled( false );
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    m_contentList.clear();
    m_contentList.append( new VideoTrackViewCheckItem( m_trackView, i18n("Video CD MPEG tracks") ) );
    m_contentList.append( new VideoTrackViewCheckItem( m_trackView, i18n("Video CD DATA track" ) ) );

    ( ( VideoTrackViewCheckItem* ) m_contentList[ 0 ] ) ->setOpen( true );

    // create a listviewItem for every video track
    int index = 0;
    m_videocddatasize = 0;
    m_videocdmpegsize = 0;

    K3b::Msf sequenceSize;

    for ( K3b::Device::Toc::const_iterator it = m_toc.constBegin();
          it != m_toc.constEnd(); ++it ) {

        if ( index > 0 ) {
            K3b::Msf length( ( *it ).length() );
            sequenceSize += length;
            m_videocdmpegsize += length.mode2Form2Bytes();
            ( void ) new VideoTrackViewItem( ( VideoTrackViewCheckItem* ) m_contentList[ 0 ], i18n( "Sequence-%1" , index ), "", index, length );
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

    m_videooptions ->setVideoCdSource( device()->blockDeviceName() );

    m_videocdinfo = new K3b::VideoCdInfo( this );
    m_videocdinfo->info( device()->blockDeviceName() );

    connect( m_videocdinfo, SIGNAL( infoFinished( bool ) ),
             this, SLOT( slotVideoCdInfoFinished( bool ) ) );

}

void K3b::VideoCdView::slotVideoCdInfoFinished( bool success )
{
    if ( success ) {
        m_videocdinfoResult = m_videocdinfo->result();
        updateDisplay();
    }

    m_trackView->setEnabled( true );
    m_toolBox->setEnabled( true );
    QApplication::restoreOverrideCursor();

}

void K3b::VideoCdView::updateDisplay()
{
    // update the listview

    VideoTrackViewItem * item = ( VideoTrackViewItem* ) m_contentList[ 0 ] ->firstChild();
    int index = 0;
    while ( item ) {
        item->updateData( m_videocdinfoResult.entry( index, K3b::VideoCdInfoResult::SEQUENCE ) );
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
            for ( index = 0; index < m_videocdinfoResult.foundEntries( K3b::VideoCdInfoResult::SEGMENT ); index++ ) {
                ( void ) new VideoTrackViewItem( check_item, m_videocdinfoResult.entry( index, K3b::VideoCdInfoResult::SEGMENT ).name, m_videocdinfoResult.entry( index, K3b::VideoCdInfoResult::SEGMENT ).id , index + 1, 0 );
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

    m_labelLength->setText( i18np( "1 track (%2)", "%1 tracks (%2)", m_toc.count(), K3b::Msf( m_toc.length() ).toString() ) );
}


void K3b::VideoCdView::initActions()
{
    m_actionCollection = new KActionCollection( this );

    KAction* actionSelectAll = KStandardAction::selectAll( this, SLOT( slotSelectAll() ),this);
    m_actionCollection->addAction("select_all",actionSelectAll);
    KAction* actionDeselectAll = KStandardAction::deselect( this, SLOT( slotDeselectAll() ),this);
    m_actionCollection->addAction("deselect_all",actionDeselectAll);
    actionDeselectAll->setText( i18n( "Dese&lect All" ) );
    KAction* actionSelect = K3b::createAction(this, i18n( "Select Track" ), 0, 0, this,
                                              SLOT( slotSelect() ), actionCollection(),
                                              "select_track" );
    KAction* actionDeselect = K3b::createAction(this, i18n( "Deselect Track" ), 0, 0, this,
                                                SLOT( slotDeselect() ), actionCollection(),
                                                "deselect_track" );

    KAction* actionStartRip = K3b::createAction(this, i18n( "Start Ripping" ), "tools-rip-video-cd", 0, this,
                                                SLOT( startRip() ), actionCollection(), "start_rip" );

    // TODO: set the actions tooltips and whatsthis infos

    // setup the popup menu
    m_popupMenu = new KActionMenu( actionCollection() );
    m_popupMenu->addAction( actionSelect );
    m_popupMenu->addAction( actionDeselect );
    m_popupMenu->addAction( actionSelectAll );
    m_popupMenu->addAction( actionDeselectAll );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( actionStartRip );

    // setup the toolbox
    m_toolBox->addAction( actionStartRip );
}


void K3b::VideoCdView::slotContextMenu( K3ListView*, Q3ListViewItem*, const QPoint& p )
{
    m_popupMenu->menu()->popup( p );
}


void K3b::VideoCdView::slotTrackSelectionChanged( Q3ListViewItem* item )
{
    actionCollection() ->action( "select_track" ) ->setEnabled( item != 0 );
    actionCollection() ->action( "deselect_track" ) ->setEnabled( item != 0 );
}

void K3b::VideoCdView::slotStateChanged( Q3ListViewItem* item )
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

void K3b::VideoCdView::startRip()
{

    int selectedItems  = 0;
    for ( Q3ListViewItemIterator it( m_trackView ); it.current(); ++it ) {
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

        kDebug() << QString("(K3b::VideoCdView::startRip())  m_videooptions ->setVideoCdSize( %1)").arg( videocdsize );
        m_videooptions ->setVideoCdSize( videocdsize );
        K3b::VideoCdRippingDialog rip( m_videooptions, this );
        rip.exec();
    }
}

void K3b::VideoCdView::slotSelectAll()
{
    for ( Q3ListViewItemIterator it( m_trackView ); it.current(); ++it )
        if ( it.current() ->isSelectable() )
            ( ( VideoTrackViewCheckItem* ) it.current() ) ->setOn( true );
}

void K3b::VideoCdView::slotDeselectAll()
{
    for ( Q3ListViewItemIterator it( m_trackView ); it.current(); ++it )
        if ( it.current() ->isSelectable() )
            ( ( VideoTrackViewCheckItem* ) it.current() ) ->setOn( false );
}

void K3b::VideoCdView::slotSelect()
{
    if ( Q3ListViewItem * sel = m_trackView->selectedItem() ) {
        ( ( VideoTrackViewCheckItem* ) sel) ->setOn( true );
        Q3ListViewItem * item = sel ->firstChild();
        while ( item ) {
            if ( item ->isSelectable() )
                ( ( VideoTrackViewCheckItem* ) item) ->setOn( true );

            item = item->nextSibling();
        }
    }
}

void K3b::VideoCdView::slotDeselect()
{
    if ( Q3ListViewItem * sel = m_trackView->selectedItem() ) {
        ( ( VideoTrackViewCheckItem* ) sel) ->setOn( false );
        Q3ListViewItem * item = sel ->firstChild();
        while ( item ) {
            if ( item ->isSelectable() )
                ( ( VideoTrackViewCheckItem* ) item) ->setOn( false );

            item = item->nextSibling();
        }
    }
}

void K3b::VideoCdView::enableInteraction( bool b )
{
    actionCollection()->action( "start_rip" )->setEnabled( b );
}

void K3b::VideoCdView::buildTree( Q3ListViewItem *parentItem, const QDomElement &parentElement, const QString& pname )
{
    VideoTrackViewItem * thisItem = 0;
    QDomNode node = parentElement.firstChild();

    while ( !node.isNull() ) {
        if ( (node.isElement() && node.nodeName() == "folder") || node.nodeName() == "file" ) {
            if ( parentItem == 0 )
                thisItem = new VideoTrackViewItem( m_trackView, thisItem );
            else
                thisItem = new VideoTrackViewItem( parentItem, thisItem );

            QString txt = node.firstChild().toElement().text();
            thisItem->setText( 0, txt);
            if ( node.nodeName() == "folder" ) {
                buildTree( thisItem, node.toElement(), pname + "_" + txt.toLower() );
            }
            else {
                thisItem->setText( 1, pname + "_" + txt.toLower() );
                buildTree( thisItem, node.toElement(), pname );
            }
        } else if ( (node.isElement() && node.nodeName() == "segment-item") || node.nodeName() == "sequence-item" ) {
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
