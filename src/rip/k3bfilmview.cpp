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
#include "k3bdvdripperwidget.h"
#include "../device/k3bdevice.h"

#include <qstring.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qvgroupbox.h>
#include <qhgroupbox.h>
#include <qlabel.h>
#include <qvaluelist.h>
#include <qstringlist.h>
#include <qlistview.h>
#include <qmessagebox.h>
#include <qpushbutton.h>

#include <kiconloader.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h>
#include <kprocess.h>
#include <klocale.h>
#include <klistview.h>
#include <kdialog.h>
#include <kapp.h>
#include <kconfig.h>
#include <kaction.h>
#include <kdebug.h>


K3bFilmView::K3bFilmView(QWidget *parent, const char *name )
 : K3bCdContentsView( parent, name )
{
  m_tcWrapper = new K3bTcWrapper( this );
  connect( m_tcWrapper, SIGNAL( successfulDvdCheck( bool ) ), this, SLOT( slotDvdChecked( bool )) );
  //    connect( m_tcWrapper, SIGNAL( notSupportedDisc() ), this, SLOT( slotNotSupportedDisc( )) );
  setupGui();
}

K3bFilmView::~K3bFilmView(){
    delete m_tcWrapper;
}

void K3bFilmView::setupGui(){
    KToolBar *toolBar = new KToolBar( k3bMain(), this, "filmviewtoolbar" );

    KAction *grab = new KAction(i18n("&Rip CD"), "editcopy", 0, this,
                               SLOT( slotRip()), this);
    grab->plug( toolBar );

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
    QVGroupBox *_groupAudioTracks = new QVGroupBox( i18n("Tracks"), _groupAudio, "videogrouptitle" );
    m_audioView = new KListView( _groupAudioTracks );
    m_audioView->addColumn( i18n("Language" ) );
    m_audioView->setItemsRenameable( false );
    m_audioView->setSorting( -1 );
    m_audioView->setColumnWidthMode( 0, QListView::Maximum );
    //m_audioView->setSelectionMode(QListView::Multi);
    // we need the right order
    m_audioView->setShowSortIndicator(false);
    m_audioView->setAllColumnsShowFocus(true);
    QHGroupBox *_groupChapterButton = new QHGroupBox( _groupVideoChapter, "buttonvideogroup" );
    _groupChapterButton->setFrameStyle( QFrame::NoFrame );
    _groupChapterButton->layout()->setSpacing(0);
    _groupChapterButton->layout()->setMargin(0);
    _groupChapterButton->addSpace(0);
    QHGroupBox *_groupAudioButton = new QHGroupBox( _groupAudioTracks, "buttonaudiogroup" );
    _groupAudioButton->setFrameStyle( QFrame::NoFrame );
    _groupAudioButton->layout()->setSpacing(0);
    _groupAudioButton->layout()->setMargin(0);
    _groupAudioButton->addSpace(0);


    //FIXME: boese: don't use fixed size if avoidable!

    QPushButton *_audioAllButton = new QPushButton( i18n("all"),  _groupAudioButton );
    _audioAllButton->setFixedSize( 60, 20 );
    QPushButton *_audioNoneButton = new QPushButton( i18n("none"),  _groupAudioButton );
    _audioNoneButton->setFixedSize( 60, 20 );
    QPushButton *_chapterAllButton = new QPushButton( i18n("all"),  _groupChapterButton );
    _chapterAllButton->setFixedSize( 60, 20 );
    QPushButton *_chapterNoneButton = new QPushButton( i18n("none"),  _groupChapterButton );
    _chapterNoneButton->setFixedSize( 60, 20 );

    QGroupBox *_groupInfo = new QGroupBox( i18n("General Information"), mainAVGroup, "infogroup" );
    _groupInfo->setColumnLayout(0, Qt::Vertical );
    QGridLayout *infoLayout = new QGridLayout( _groupInfo->layout() );
    QLabel *input = new QLabel( i18n("Input format"), _groupInfo );
    QLabel *mode = new QLabel( i18n("Video norm"), _groupInfo );
    QLabel *res = new QLabel( i18n("Resolution"), _groupInfo );
    QLabel *aspect = new QLabel( i18n("Aspect ratio"), _groupInfo );
    QLabel *time = new QLabel( i18n("Time"), _groupInfo );
    //QLabel *video = new QLabel( i18n("Video"), _groupInfo );
    //QLabel *audio = new QLabel( i18n("Audio"), _groupInfo );
    QLabel *frames = new QLabel( i18n("Frames"), _groupInfo );
    QLabel *framerate = new QLabel( i18n("Framerate"), _groupInfo );
    m_input = new QLabel( "", _groupInfo );
    m_mode = new QLabel( "", _groupInfo );
    m_res = new QLabel( "", _groupInfo );
    m_aspect = new QLabel( "", _groupInfo );
    m_time = new QLabel( "", _groupInfo );
    //m_video = new QLabel( "", _groupInfo );
    //m_audio = new QLabel( "", _groupInfo );
    m_frames = new QLabel( "", _groupInfo );
    m_framerate = new QLabel( "", _groupInfo );
    infoLayout->addMultiCellWidget( input, 0, 0, 0, 0 );
    infoLayout->addMultiCellWidget( mode, 1, 1, 0, 0 );
    infoLayout->addMultiCellWidget( res, 2, 2, 0, 0 );
    infoLayout->addMultiCellWidget( aspect, 3, 3, 0, 0 );
    //infoLayout->addMultiCellWidget( video, 0, 0, 2, 2 );
    //infoLayout->addMultiCellWidget( audio, 1, 1, 2, 2 );
    infoLayout->addMultiCellWidget( m_input, 0, 0, 1, 1 );
    infoLayout->addMultiCellWidget( m_mode, 1, 1, 1, 1 );
    infoLayout->addMultiCellWidget( m_res, 2, 2, 1, 1 );
    infoLayout->addMultiCellWidget( m_aspect, 3, 3, 1, 1 );
    //infoLayout->addMultiCellWidget( m_video, 0, 0, 3, 3 );
    //infoLayout->addMultiCellWidget( m_audio, 1, 1, 3, 3 );
    infoLayout->addMultiCellWidget( time, 0, 0, 2, 2 );
    infoLayout->addMultiCellWidget( frames, 1, 1, 2, 2 );
    infoLayout->addMultiCellWidget( framerate, 2, 2, 2, 2 );
    infoLayout->addMultiCellWidget( m_time, 0, 0, 3, 3 );
    infoLayout->addMultiCellWidget( m_frames, 1, 1, 3, 3 );
    infoLayout->addMultiCellWidget( m_framerate, 2, 2, 3, 3 );

    // TODO

    _groupAudio->setDisabled(true);
    _groupVideoChapter->setDisabled(true);


    //_layout->addMultiCellWidget( toolBar, 0, 0, 0, 1 );
    _layout->addMultiCellWidget( _groupVideo, 0, 0, 0, 2 );
    _layout->addMultiCellWidget( _groupAudio, 0, 0, 3, 3 );
    _layout->addMultiCellWidget( _groupInfo, 1, 1, 0, 3 );
    _layout->setColStretch( 1, 4 );
    _layout->setColStretch( 3, 2 );
    // connect to the actions
    //connect( _buttonReload, SIGNAL(clicked()), this, SLOT(reload()) );
    connect( m_titleView, SIGNAL( executed( QListViewItem* ) ), this, SLOT( slotTitleSelected( QListViewItem* ) ) );
    //connect( m_chapterView, SIGNAL( executed( QListViewItem* ) ), this, SLOT( slotChapterSelected( QListViewItem* ) ) );
    //connect( m_audioView, SIGNAL( executed( QListViewItem* ) ), this, SLOT( slotAudioSelected( QListViewItem* ) ) );
    connect( _audioAllButton, SIGNAL( clicked( ) ), this, SLOT( slotAudioButtonAll( ) ) );
    connect( _audioNoneButton, SIGNAL( clicked( ) ), this, SLOT( slotAudioButtonNone( ) ) );
    connect( _chapterAllButton, SIGNAL( clicked( ) ), this, SLOT( slotChapterButtonAll( ) ) );
    connect( _chapterNoneButton, SIGNAL( clicked( ) ), this, SLOT( slotChapterButtonNone( ) ) );
}


void K3bFilmView::setDevice( K3bDevice* device ){
    m_device = device;
}

void K3bFilmView::reload()
{
  // check if transcode tool installed
  if( !m_tcWrapper->supportDvd() ) {
    emit notSupportedDisc( m_device->devicename() );
    return;
  }
  KDialog* infoDialog = new KDialog( this, "waitForDiskInfoDialog", true, WDestructiveClose );
  infoDialog->setCaption( i18n("Please wait...") );
  QHBoxLayout* infoLayout = new QHBoxLayout( infoDialog );
  infoLayout->setSpacing( KDialog::spacingHint() );
  infoLayout->setMargin( KDialog::marginHint() );
  infoLayout->setAutoAdd( true );
  QLabel* picLabel = new QLabel( infoDialog );
  picLabel->setPixmap( DesktopIcon( "cdwriter_unmount" ) );
  QLabel* infoLabel = new QLabel( i18n("K3b is fetching information about title "), infoDialog );
  m_fetchingInfoLabel = new QLabel( "<1>", infoDialog );
  connect( m_tcWrapper, SIGNAL(tcprobeTitleParsed( int )), this, SLOT( slotUpdateInfoDialog( int )) );
  connect( m_tcWrapper, SIGNAL( successfulDvdCheck( bool )), infoDialog, SLOT( close() ));
  infoDialog->show();

  m_tcWrapper->checkDvdContent( m_device );
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
            // angle parsing not possible at the moment, more than one angle not supported
            // get Dvdcontent from dvdtitles->getMaxAngle()
            QCheckListItem *itemChild = new QCheckListItem( item, i18n("Angle 1"), QCheckListItem::CheckBox );
        }
        if( m_titleView->childCount() > 0 ) {
            m_titleView->setCurrentItem( m_titleView->itemAtIndex( 0 ) );
            slotTitleSelected( m_titleView->currentItem() );
            setCheckBoxes( m_audioView, TRUE );
            setCheckBoxes( m_chapterView, TRUE );
        }
        QWidget::show();
    } else {
        // error during parsing
      emit notSupportedDisc( m_device->devicename() );
    }
}

void K3bFilmView::slotTitleSelected(QListViewItem*item){
    int row = m_titleView->itemIndex( item );
    QString tmp = item->text(0);
    if( tmp.contains( i18n("Title") ) ){
        row = tmp.right(1).toInt()-1;
    } else {
        if( item->text(0).right(1).toInt() != 1){
            QMessageBox::critical( this, i18n("DVD Error"), i18n("Angle information not supported, title selection always use angle 1."), i18n("Ok") );
        } else {
            row = item->parent()->text(0).right(1).toInt() - 1;
        }
    }
    kdDebug() << "(K3bFilmView) Show title information for entry <" << row << ">" << endl;
    if( row >= 0 ){
        K3bDvdContent *title = &(m_dvdTitles[ row ]);
        // get data from tcwrapper
        m_input->setText( title->getInput() );
        m_mode->setText( title->getMode() );
        m_res->setText( title->getStrRes() );
        m_aspect->setText( title->getStrAspect() );
        m_time->setText( title->getStrTime() );
        //m_video->setText( title->getVideo() );
        //m_audio->setText( title->getAudio() );
        m_frames->setText( title->getStrFrames() );
        m_framerate->setText( title->getStrFramerate() );
	
        m_chapterView->clear();
        m_audioView->clear();
        int end = title->getAudioList()->count();
        QStringList::Iterator str;
        for( int i = end-1; i >= 0; i-- ) {
            str = title->getAudioList()->at( i );
            QCheckListItem *item = new QCheckListItem( m_audioView, (*str), QCheckListItem::CheckBox );
        }
        for( int i= title->getMaxChapters(); i> 0; i-- ){
            QCheckListItem *item = new QCheckListItem( m_chapterView, i18n("Chapter ")+QString::number(i), QCheckListItem::CheckBox );
        }
    }
}

void K3bFilmView::slotNotSupportedDisc(){
    emit notSupportedDisc( m_device->devicename() );
}

void K3bFilmView::slotUpdateInfoDialog( int i ){
   m_fetchingInfoLabel->setText( "<"+QString::number( i )+">" );
}

void K3bFilmView::slotChapterButtonAll(){
    setCheckBoxes( m_chapterView, TRUE );
}
void K3bFilmView::slotChapterButtonNone(){
    setCheckBoxes( m_chapterView, FALSE );
}
void K3bFilmView::slotAudioButtonAll(){
    setCheckBoxes( m_audioView, TRUE );
}
void K3bFilmView::slotAudioButtonNone(){
    setCheckBoxes( m_audioView, FALSE );
}

void K3bFilmView::slotRip(){
    K3bDvdRipperWidget *ripWidget = new K3bDvdRipperWidget( m_device->devicename(), this, "dvdrip");
    DvdTitle::Iterator dvd;
    int c = m_titleView->childCount();
    kdDebug() << "(K3bFilmView) titles " << c << endl;
    DvdTitle toRipTitles;
    QCheckListItem *item = dynamic_cast<QCheckListItem*>( m_titleView->firstChild( ) );
    int title = 0;
    bool addToRipTitles = false;
    // clear old angle selection if already used

    while( item !=0 ){
        if( item->type() == QCheckListItem::Controller ){
            //kdDebug() << "item " << item->text(0) << endl;
            dvd = m_dvdTitles.at( title );
            (*dvd).getSelectedAngle()->clear();
            int c  = item->childCount();
            if( c > 0 ){
                QCheckListItem *child = dynamic_cast<QCheckListItem*>(item->firstChild( ) );
                //kdDebug() << "child " << child->text() << endl;
                if( child->isOn() ){
                    kdDebug() << "(K3bDvdFilmView) Add title " << title+1 << " with angle " << 1 << "." << endl;
                    ( *dvd ).addAngle( QString::number( 1 ) );
                    addToRipTitles = true;
                }
                for( int i=1; i < c; i++){
                    if( dynamic_cast<QCheckListItem*>(item->nextSibling( ) )->isOn() ){
                        kdDebug() << "(K3bDvdFilmView) Add more title with angle " << i+1 << "." << endl;
                        ( *dvd ).addAngle( QString::number( i+1 ) );
                    }
                }
                if( addToRipTitles ) {
                    toRipTitles.append( *dvd );
                    addToRipTitles = false;
                }
            }   // end if c>0 , there are angles
        }     // end if controller
        item = dynamic_cast<QCheckListItem*>(item->nextSibling() );
        title++;
    }
    if( toRipTitles.isEmpty() ){
         QMessageBox::critical( this, i18n("Ripping Error"), i18n("Select an title/angle to rip."), i18n("Ok") );
    } else {
        ripWidget->init( toRipTitles );
        ripWidget->show();
    }
}

void K3bFilmView::setCheckBoxes( KListView *view, bool status ){
    int count =  view->childCount();
    for( int i = 0; i < count; i++) {
        dynamic_cast<QCheckListItem*> (view->itemAtIndex( i ))->setOn( status );
    }
}


#include "k3bfilmview.moc"
