/***************************************************************************
                          k3bfilmview.cpp  -  description
                             -------------------
    begin                : Fri Feb 22 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3bfilmview.h"
#include "k3btcwrapper.h"
#include "../k3b.h"
#include "k3bdvdcontent.h"

#include <qstring.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qvgroupbox.h>
#include <qhgroupbox.h>
#include <qlabel.h>
#include <qvaluelist.h>
#include <qstringlist.h>

#include <kiconloader.h>
#include <ktoolbar.h>
#include <kprocess.h>
#include <klocale.h>
#include <klistview.h>
#include <kdialog.h>

#include <qgrid.h>

#define RELOAD_BUTTON_INDEX         0
#define GRAB_BUTTON_INDEX             1

K3bFilmView::K3bFilmView(QWidget *parent, const char *name ) : QWidget(parent,name) {
    m_tcWrapper = new K3bTcWrapper();
    connect( m_tcWrapper, SIGNAL( successfulDvdCheck( bool ) ), this, SLOT( slotDvdChecked( bool )) );
    connect( m_tcWrapper, SIGNAL( notSupportedDisc() ), this, SLOT( slotNotSupportedDisc( )) );
    setupGui();
}

K3bFilmView::~K3bFilmView(){
}

void K3bFilmView::setupGui(){
    KToolBar *toolBar = new KToolBar( k3bMain(), this, "filmviewtoolbar" );

    KIconLoader *_il = new KIconLoader("k3b");
    toolBar->insertButton( _il->iconPath("reload", KIcon::Toolbar), 	RELOAD_BUTTON_INDEX);
    toolBar->insertButton( _il->iconPath("editcopy", KIcon::Toolbar), 	GRAB_BUTTON_INDEX);
    KToolBarButton *_buttonGrab = toolBar->getButton(GRAB_BUTTON_INDEX);
    KToolBarButton *_buttonReload = toolBar->getButton(RELOAD_BUTTON_INDEX);

    QGroupBox *mainAVGroup = new QGroupBox( this );
    mainAVGroup->setColumnLayout(0, Qt::Vertical );

    QGridLayout *_mainLayout = new QGridLayout( this );
    _mainLayout->addMultiCellWidget( toolBar, 0, 0, 0, 0 );
    _mainLayout->addMultiCellWidget( mainAVGroup, 1, 1, 0, 0 );

    QGridLayout *_layout = new QGridLayout( mainAVGroup->layout() );
    _layout->setSpacing( KDialog::spacingHint() );
    //_layout->setMargin( KDialog::marginHint() );

    QHGroupBox *_groupVideo = new QHGroupBox( i18n("Video"), mainAVGroup, "videogroup" );
    QVGroupBox *_groupVideoTitle = new QVGroupBox( i18n("Title"), _groupVideo, "videogrouptitle" );
    QVGroupBox *_groupVideoChapter = new QVGroupBox( i18n("Chapter"), _groupVideo, "videogroupchapter" );
    m_titleView = new KListView( _groupVideoTitle );
    m_titleView->addColumn( i18n("Titles" ), m_titleView->width() );
    m_titleView->setColumnWidthMode( 0, QListView::Manual );
    m_titleView->setItemsRenameable( false );
    m_titleView->setSorting( -1 );
    m_chapterView = new KListView( _groupVideoChapter );
    m_chapterView->addColumn( "Chapters" );
    m_chapterView->setItemsRenameable( false );
    m_chapterView->setSorting( -1 );
    m_chapterView->setColumnWidthMode( 0, QListView::Maximum );
    QVGroupBox *_groupAudio = new QVGroupBox( i18n("Audio"), mainAVGroup, "audiogroup" );
    m_audioView = new KListView( _groupAudio );
    m_audioView->addColumn( i18n("Language" ) );
    m_audioView->setItemsRenameable( false );
    m_audioView->setSorting( -1 );
    m_audioView->setColumnWidthMode( 0, QListView::Maximum );
    //m_audioView->setSelectionMode(QListView::Multi);
    // we need the right order
    m_audioView->setShowSortIndicator(false);
    m_audioView->setAllColumnsShowFocus(true);

    QGroupBox *_groupInfo = new QGroupBox( i18n("General Information"), mainAVGroup, "infogroup" );
    _groupInfo->setColumnLayout(0, Qt::Vertical );
    QGridLayout *infoLayout = new QGridLayout( _groupInfo->layout() );
    QLabel *input = new QLabel( i18n("Input format"), _groupInfo );
    QLabel *mode = new QLabel( i18n("Video norm"), _groupInfo );
    QLabel *res = new QLabel( i18n("Resolution"), _groupInfo );
    QLabel *aspect = new QLabel( i18n("Aspect ratio"), _groupInfo );
    QLabel *time = new QLabel( i18n("Time"), _groupInfo );
    QLabel *video = new QLabel( i18n("Video"), _groupInfo );
    QLabel *audio = new QLabel( i18n("Audio"), _groupInfo );
    QLabel *frames = new QLabel( i18n("Frames"), _groupInfo );
    QLabel *framerate = new QLabel( i18n("Framerate"), _groupInfo );
    m_input = new QLabel( "", _groupInfo );
    m_mode = new QLabel( "", _groupInfo );
    m_res = new QLabel( "", _groupInfo );
    m_aspect = new QLabel( "", _groupInfo );
    m_time = new QLabel( "", _groupInfo );
    m_video = new QLabel( "", _groupInfo );
    m_audio = new QLabel( "", _groupInfo );
    m_frames = new QLabel( "", _groupInfo );
    m_framerate = new QLabel( "", _groupInfo );
    infoLayout->addMultiCellWidget( input, 0, 0, 0, 0 );
    infoLayout->addMultiCellWidget( mode, 1, 1, 0, 0 );
    infoLayout->addMultiCellWidget( res, 2, 2, 0, 0 );
    infoLayout->addMultiCellWidget( aspect, 3, 3, 0, 0 );
    infoLayout->addMultiCellWidget( time, 4, 4, 0, 0 );
    infoLayout->addMultiCellWidget( video, 0, 0, 2, 2 );
    infoLayout->addMultiCellWidget( audio, 1, 1, 2, 2 );
    infoLayout->addMultiCellWidget( frames, 2, 2, 2, 2 );
    infoLayout->addMultiCellWidget( framerate, 3, 3, 2, 2 );
    infoLayout->addMultiCellWidget( m_input, 0, 0, 1, 1 );
    infoLayout->addMultiCellWidget( m_mode, 1, 1, 1, 1 );
    infoLayout->addMultiCellWidget( m_res, 2, 2, 1, 1 );
    infoLayout->addMultiCellWidget( m_aspect, 3, 3, 1, 1 );
    infoLayout->addMultiCellWidget( m_time, 4, 4, 1, 1 );
    infoLayout->addMultiCellWidget( m_video, 0, 0, 3, 3 );
    infoLayout->addMultiCellWidget( m_audio, 1, 1, 3, 3 );
    infoLayout->addMultiCellWidget( m_frames, 2, 2, 3, 3 );
    infoLayout->addMultiCellWidget( m_framerate, 3, 3, 3, 3 );

    //_layout->addMultiCellWidget( toolBar, 0, 0, 0, 1 );
    _layout->addMultiCellWidget( _groupVideo, 0, 0, 0, 2 );
    _layout->addMultiCellWidget( _groupAudio, 0, 0, 3, 3 );
    _layout->addMultiCellWidget( _groupInfo, 1, 1, 0, 3 );
    _layout->setColStretch( 1, 4 );
    _layout->setColStretch( 3, 2 );
    // connect to the actions
    //connect( _buttonReload, SIGNAL(clicked()), this, SLOT(reload()) );
    //connect( _buttonGrab, SIGNAL(clicked()), this, SLOT(prepareRipping()) );
}


void K3bFilmView::setDevice( const QString& device ){
    m_device = device;
}

void K3bFilmView::show(){
    /*
    if( !m_initialized){
        m_initialized=true;
        setupGUI();
    }
    */
    // check if transcode tool installed
    if( !m_tcWrapper->supportDvd() ) {
        emit notSupportedDisc( m_device );
        return;
    }
    m_tcWrapper->checkDvd( m_device );
}
// ------------------------------------------
// slots
// ------------------------------------------

void K3bFilmView::slotDvdChecked( bool successful ){
    m_dvdTitles.clear();
    if( successful ){
        m_dvdTitles = m_tcWrapper->getDvdTitles();
        m_titleView->clear();
        for( unsigned int i= m_dvdTitles.count(); i > 0; i--){
            QCheckListItem *item = new QCheckListItem( m_titleView, i18n("Title ") + QString::number( i ), QCheckListItem::Controller );
            item->setOpen( true );
            QCheckListItem *itemChild = new QCheckListItem( item, i18n("Angle 1"), QCheckListItem::CheckBox );
        }
        if( m_titleView->childCount() > 0 ) {
            m_titleView->setCurrentItem( m_titleView->itemAtIndex( 0 ) );
            slotTitleSelected( 0 );
        }
        qDebug("(K3bFilmView) show");
        QWidget::show();
    } else {
        // error during parsing
    }
}

void K3bFilmView::slotTitleSelected( int row ){
        K3bDvdContent *title = &(m_dvdTitles[ row ]);
        // get data from tcwrapper
        m_input->setText( title->m_input );
        m_mode->setText( title->m_mode );
        m_res->setText( title->m_res );
        m_aspect->setText( title->m_aspect );
        m_time->setText( title->m_time );
        m_video->setText( title->m_video );
        m_audio->setText( title->m_audio );
        m_frames->setText( title->m_frames );
        m_framerate->setText( title->m_framerate );
	
        m_chapterView->clear();
        m_audioView->clear();
        int end = title->m_audioList.count();
        QStringList::Iterator str;
        for( int i = end-1; i >= 0; i-- ) {
            str = title->m_audioList.at( i );
            QCheckListItem *item = new QCheckListItem( m_audioView, (*str), QCheckListItem::CheckBox );
        }
        for( int i= title->chapters; i> 0; i-- ){
            QCheckListItem *item = new QCheckListItem( m_chapterView, i18n("Chapter ")+QString::number(i), QCheckListItem::CheckBox );
        }
}

void K3bFilmView::slotNotSupportedDisc(){
    emit notSupportedDisc( m_device );
}

#include "k3bfilmview.moc"