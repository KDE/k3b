/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bvideodvdrippingdialog.h"
#include "k3bvideodvdrippingwidget.h"
#include <k3bjobprogressdialog.h>

#include <klocale.h>
#include <klistview.h>
#include <klocale.h>
#include <kglobal.h>

#include <qlayout.h>
#include <qcheckbox.h>
#include <qspinbox.h>



K3bVideoDVDRippingDialog::K3bVideoDVDRippingDialog( const K3bVideoDVD::VideoDVD& dvd, 
						    const QValueList<int>& titles,
						    QWidget* parent, const char* name )
  : K3bInteractionDialog( parent, name,
			  i18n("Video DVD Ripping"),
			  QString::null,
			  START_BUTTON|CANCEL_BUTTON,
			  START_BUTTON,
			  "VideoDVD Ripping" ), // config group 
    m_dvd( dvd )
{
  QWidget* frame = mainWidget();
  QHBoxLayout* frameLayout = new QHBoxLayout( frame );
  frameLayout->setMargin( 0 );
  frameLayout->setAutoAdd( true );
  m_w = new K3bVideoDVDRippingWidget( frame );

  setTitle( i18n("Video DVD Ripping"), 
	    i18n("1 title from %1", "%n titles from %1", titles.count()).arg(m_dvd.volumeIdentifier()) );

  // populate list map
  populateTitleView( titles );
}


K3bVideoDVDRippingDialog::~K3bVideoDVDRippingDialog()
{
}


void K3bVideoDVDRippingDialog::populateTitleView( const QValueList<int>& titles )
{
  m_w->m_titleView->clear();
  m_titleRipInfos.clear();

  for( QValueList<int>::const_iterator it = titles.begin(); it != titles.end(); ++it ) {
    QCheckListItem* rootItem = new QCheckListItem( m_w->m_titleView, 
						   m_w->m_titleView->lastItem(),
						   i18n("Title %1")
						   .arg(*it),
						   QCheckListItem::RadioButtonController );
    rootItem->setText( 1, QString("%1x%2")
		       .arg(m_dvd[*it-1].videoStream().pictureWidth())
		       .arg(m_dvd[*it-1].videoStream().pictureHeight()) );
    rootItem->setText( 3, QString("%1 Title %2.avi").arg(m_dvd.volumeIdentifier()).arg(titles[*it]) );

    // now for the rip info
    K3bVideoDVDRippingJob::TitleRipInfo ri( *it );

    //
    // Determine default language selection:
    // first try the configured locale, if that fails, fall back to the first audio stream
    //
    ri.audioStream = 0;
    for( unsigned int i = 0; i < m_dvd[*it-1].numAudioStreams(); ++i ) {
      if( m_dvd[*it-1].audioStream(i).langCode() == KGlobal::locale()->language() ) {
	ri.audioStream = i;
	break;
      }
    }

    for( unsigned int i = 0; i < m_dvd[*it-1].numAudioStreams(); ++i ) {
      QCheckListItem* asI = new QCheckListItem( rootItem,
						i18n("%1 %2Ch (%3%4)")
						.arg( K3bVideoDVD::audioFormatString( m_dvd[*it-1].audioStream(i).format() ) )
						.arg( m_dvd[*it-1].audioStream(i).channels() )
						.arg( m_dvd[*it-1].audioStream(i).langCode().isEmpty()
						      ? i18n("unknown language")
						      : KGlobal::locale()->twoAlphaToLanguageName( m_dvd[*it-1].audioStream(i).langCode() ) )
						.arg( m_dvd[*it-1].audioStream(i).codeExtension() != K3bVideoDVD::AUDIO_CODE_EXT_UNSPECIFIED 
						      ? QString(" ") + K3bVideoDVD::audioCodeExtensionString( m_dvd[*it-1].audioStream(i).codeExtension() )
						      : QString::null ),
						QCheckListItem::RadioButton );
      if( ri.audioStream == (int)i )
	asI->setState( QCheckListItem::On );
    }

    rootItem->setOpen( true );

    m_titleRipInfos[rootItem] = ri;
  }
}


void K3bVideoDVDRippingDialog::setBaseDir( const QString& path )
{
}


void K3bVideoDVDRippingDialog::loadK3bDefaults()
{
}


void K3bVideoDVDRippingDialog::loadUserDefaults( KConfigBase* )
{
}


void K3bVideoDVDRippingDialog::saveUserDefaults( KConfigBase* )
{
}


void K3bVideoDVDRippingDialog::slotStartClicked()
{
  // FIXME: prepare title infos, audio streams and clipping
  int i = 0;
  QValueVector<K3bVideoDVDRippingJob::TitleRipInfo> titles( m_titleRipInfos.count() );
  for( QMapConstIterator<QCheckListItem*, K3bVideoDVDRippingJob::TitleRipInfo> it = m_titleRipInfos.begin();
       it != m_titleRipInfos.end(); ++it )
    titles[i++] = it.data();

  // start the job
  K3bJobProgressDialog dlg( this );
  K3bVideoDVDRippingJob* job = new K3bVideoDVDRippingJob( &dlg, &dlg );
  job->setVideoDVD( m_dvd );
  job->setTitles( titles );

  job->setVideoBitrate( m_w->m_spinVideoBitrate->value() );
  job->setTwoPassEncoding( m_w->m_checkTwoPassEncoding->isChecked() );
  job->setResampleAudioTo44100( m_w->m_checkAudioResampling->isChecked() );
  job->setAutoClipping( m_w->m_checkAutoClipping->isChecked() );
  job->setVideoCodec( m_w->selectedVideoCodec() );
  job->setAudioCodec( m_w->selectedAudioCodec() );
  job->setLowPriority( m_w->m_checkLowPriority->isChecked() );
  job->setAudioBitrate( m_w->selectedAudioBitrate() );
  job->setAudioVBR( m_w->m_checkAudioVBR->isChecked() );

  hide();
  dlg.startJob( job );
  close();
}

#include "k3bvideodvdrippingdialog.moc"
