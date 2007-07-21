/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bvideodvdrippingview.h"
#include "k3bvideodvdrippingtitlelistview.h"
#include "k3bvideodvdrippingdialog.h"

#include <k3bvideodvd.h>
#include <k3bvideodvdtitletranscodingjob.h>
#include <k3btoolbox.h>
#include <k3bthememanager.h>
#include <k3bglobals.h>
#include <k3blibdvdcss.h>
#include <k3bcore.h>
#include <k3bexternalbinmanager.h>

#include <qcursor.h>
#include <qlayout.h>
#include <qlabel.h>

#include <kapplication.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kaction.h>
#include <kconfig.h>


K3bVideoDVDRippingView::K3bVideoDVDRippingView( QWidget* parent, const char * name )
  : K3bMediaContentsView( true,
			  K3bMedium::CONTENT_VIDEO_DVD,
			  K3bDevice::MEDIA_DVD_ALL,
			  K3bDevice::STATE_INCOMPLETE|K3bDevice::STATE_COMPLETE,
			  parent, name )
{
  QGridLayout* mainGrid = new QGridLayout( mainWidget() );

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


  // the title view
  // ----------------------------------------------------------------------------------
  m_titleView = new K3bVideoDVDRippingTitleListView( mainWidget() );

  connect( m_titleView, SIGNAL(contextMenu(KListView*, QListViewItem*, const QPoint&)),
	   this, SLOT(slotContextMenu(KListView*, QListViewItem*, const QPoint&)) );

  // general layout
  // ----------------------------------------------------------------------------------
  mainGrid->addLayout( toolBoxLayout, 0, 0 );
  mainGrid->addWidget( m_titleView, 1, 0 );


  initActions();

  m_toolBox->addButton( actionCollection()->action("start_rip"), true );

  setLeftPixmap( K3bTheme::MEDIA_LEFT );
  setRightPixmap( K3bTheme::MEDIA_VIDEO );
}


K3bVideoDVDRippingView::~K3bVideoDVDRippingView()
{
}


void K3bVideoDVDRippingView::reloadMedium()
{
  //
  // For VideoDVD reading it is important that the DVD is not mounted
  //
  if( K3b::isMounted( device() ) && !K3b::unmount( device() ) ) {
    KMessageBox::error( this,
			i18n("K3b was unable to unmount device '%1' containing medium '%2'. "
			     "Video DVD ripping will not work if the device is mounted. "
			     "Please unmount manually."),
			i18n("Unmounting failed") );
  }

  //
  // K3bVideoDVD::open does not necessarily fail on encrypted DVDs if dvdcss is not
  // available. Thus, we test the availability of libdvdcss here
  //
  if( device()->copyrightProtectionSystemType() == 1 ) {
    K3bLibDvdCss* css = K3bLibDvdCss::create();
    if( !css ) {
      KMessageBox::error( this, i18n("<p>Unable to read Video DVD contents: Found encrypted Video DVD."
				     "<p>Install <i>libdvdcss</i> to get Video DVD decryption support.") );
      return;
    }
    else
      delete css;
  }

  QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

  if( m_dvd.open( device() ) ) {
    setTitle( medium().beautifiedVolumeId() + " (" + i18n("Video DVD") + ")" );
    m_labelLength->setText( i18n("%n title", "%n titles", m_dvd.numTitles() ) );
    m_titleView->setVideoDVD( m_dvd );
    QApplication::restoreOverrideCursor();

    bool transcodeUsable = true;

    if( !k3bcore ->externalBinManager() ->foundBin( "transcode" ) ) {
      KMessageBox::sorry( this,
			  i18n("K3b uses transcode to rip Video DVDs. "
			       "Please make sure it is installed.") );
      transcodeUsable = false;
    }
    else {
      int vc = 0, ac = 0;
      for( int i = 0; i < K3bVideoDVDTitleTranscodingJob::VIDEO_CODEC_NUM_ENTRIES; ++i )
	if( K3bVideoDVDTitleTranscodingJob::transcodeBinaryHasSupportFor( (K3bVideoDVDTitleTranscodingJob::VideoCodec)i ) )
	  ++vc;
      for( int i = 0; i < K3bVideoDVDTitleTranscodingJob::AUDIO_CODEC_NUM_ENTRIES; ++i )
	if( K3bVideoDVDTitleTranscodingJob::transcodeBinaryHasSupportFor( (K3bVideoDVDTitleTranscodingJob::AudioCodec)i ) )
	  ++ac;
      if( !ac || !vc ) {
	KMessageBox::sorry( this,
			    i18n("<p>K3b uses transcode to rip Video DVDs. "
				 "Your installation of transcode (<em>%1</em>) lacks support for any of the "
				 "codecs supported by K3b."
				 "<p>Please make sure it is installed properly.") );
	transcodeUsable = false;
      }
    }

    actionCollection()->action("start_rip")->setEnabled( transcodeUsable );
  }
  else {
    QApplication::restoreOverrideCursor();

    KMessageBox::error( this, i18n("Unable to read Video DVD contents.") );
  }
}


void K3bVideoDVDRippingView::slotStartRipping()
{
  QValueList<int> titles;
  int i = 1;
  for( QListViewItemIterator it( m_titleView ); *it; ++it, ++i )
    if( static_cast<K3bCheckListViewItem*>( *it )->isChecked() )
      titles.append( i );

  if( titles.isEmpty() ) {
    KMessageBox::error( this, i18n("Please select the titles to rip."),
			i18n("No Titles Selected") );
  }
  else {
    K3bVideoDVDRippingDialog dlg( m_dvd, titles, this );
    dlg.exec();
  }
}


void K3bVideoDVDRippingView::slotContextMenu( KListView*, QListViewItem*, const QPoint& p )
{
  m_popupMenu->popup(p);
}


void K3bVideoDVDRippingView::slotCheckAll()
{
  for( QListViewItemIterator it( m_titleView ); it.current(); ++it )
    dynamic_cast<K3bCheckListViewItem*>(it.current())->setChecked(true);
}


void K3bVideoDVDRippingView::slotUncheckAll()
{
  for( QListViewItemIterator it( m_titleView ); it.current(); ++it )
    dynamic_cast<K3bCheckListViewItem*>(it.current())->setChecked(false);
}


void K3bVideoDVDRippingView::slotCheck()
{
  QPtrList<QListViewItem> items( m_titleView->selectedItems() );
  for( QPtrListIterator<QListViewItem> it( items );
       it.current(); ++it )
    dynamic_cast<K3bCheckListViewItem*>(it.current())->setChecked(true);
}


void K3bVideoDVDRippingView::slotUncheck()
{
  QPtrList<QListViewItem> items( m_titleView->selectedItems() );
  for( QPtrListIterator<QListViewItem> it( items );
       it.current(); ++it )
    dynamic_cast<K3bCheckListViewItem*>(it.current())->setChecked(false);
}


void K3bVideoDVDRippingView::initActions()
{
  m_actionCollection = new KActionCollection( this );

  KAction* actionSelectAll = new KAction( i18n("Check All"), 0, 0, this,
					  SLOT(slotCheckAll()), actionCollection(),
					  "check_all" );
  KAction* actionDeselectAll = new KAction( i18n("Uncheck All"), 0, 0, this,
					    SLOT(slotUncheckAll()), actionCollection(),
					    "uncheck_all" );
  KAction* actionSelect = new KAction( i18n("Check Track"), 0, 0, this,
				       SLOT(slotCheck()), actionCollection(),
				       "select_track" );
  KAction* actionDeselect = new KAction( i18n("Uncheck Track"), 0, 0, this,
					 SLOT(slotUncheck()), actionCollection(),
					 "deselect_track" );
  KAction* actionStartRip = new KAction( i18n("Start Ripping"), "gear", 0, this,
					 SLOT(slotStartRipping()), m_actionCollection, "start_rip" );

  actionStartRip->setToolTip( i18n("Open the Video DVD ripping dialog") );

  // setup the popup menu
  m_popupMenu = new KActionMenu( actionCollection(), "popup_menu" );
  KAction* separator = new KActionSeparator( actionCollection(), "separator" );
  m_popupMenu->insert( actionSelect );
  m_popupMenu->insert( actionDeselect );
  m_popupMenu->insert( actionSelectAll );
  m_popupMenu->insert( actionDeselectAll );
  m_popupMenu->insert( separator );
  m_popupMenu->insert( actionStartRip );
}


void K3bVideoDVDRippingView::enableInteraction( bool enable )
{
  actionCollection()->action( "start_rip" )->setEnabled( enable );
}


#include "k3bvideodvdrippingview.moc"
