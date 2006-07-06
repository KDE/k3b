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

#include "k3bvideodvdrippingwidget.h"

#include <k3bvideodvdtitletranscodingjob.h>
#include <k3bglobals.h>

#include <klistview.h>
#include <klocale.h>
#include <kurlrequester.h>
#include <kio/global.h>
#include <kurllabel.h>

#include <qcombobox.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qwhatsthis.h>


// FIXME: these are mp3 bitrates. AC3 bitrates range from 32 to 640 kbit.
static const int s_bitrates[] = {
  32,
  40,
  48,
  56,
  64,
  80,
  96,
  112,
  128,
  160,
  192,
  224,
  256,
  320,
  0 // just used for the loops below
};



K3bVideoDVDRippingWidget::K3bVideoDVDRippingWidget( QWidget* parent )
  : base_K3bVideoDVDRippingWidget( parent )
{
  m_editBaseDir->setMode( KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly );

  m_titleView->addColumn( i18n("Title") );
  m_titleView->addColumn( i18n("Video Size") );
  m_titleView->addColumn( i18n("File Size") );
  m_titleView->addColumn( i18n("Filename") );
  m_titleView->setSorting( -1 );

  //
  // Example filename pattern
  //
  m_comboFilenamePattern->insertItem( QString( "%b - %1 %t (%n %a %c)" ).arg(i18n("Title") ) );
  m_comboFilenamePattern->insertItem( QString( "%{volumeid} (%{title})" ) );


  //
  // Add the Audio bitrates
  //
  for( int i = 0; s_bitrates[i]; ++i )
    m_comboAudioBitrate->insertItem( i18n("%1 kbps" ).arg(s_bitrates[i]) );


  //
  // Add the codecs (we use a direct mapping between codec and combobox index)
  //
  for( int i = 0; i < K3bVideoDVDTitleTranscodingJob::VIDEO_CODEC_NUM_ENTRIES; ++i ) {
    m_comboVideoCodec->insertItem( K3bVideoDVDTitleTranscodingJob::videoCodecString( i ) );
  }
  for( int i = 0; i < K3bVideoDVDTitleTranscodingJob::AUDIO_CODEC_NUM_ENTRIES; ++i ) {
    m_comboAudioCodec->insertItem( K3bVideoDVDTitleTranscodingJob::audioCodecString( i ) );
  }

  connect( m_comboAudioBitrate, SIGNAL(textChanged(const QString&)),
	   this, SIGNAL(changed()) );
  connect( m_spinVideoBitrate, SIGNAL(valueChanged(int)),
	   this, SIGNAL(changed()) );
  connect( m_editBaseDir, SIGNAL(textChanged(const QString&)), this, SIGNAL(changed()) );
  connect( m_specialStringsLabel, SIGNAL(leftClickedURL()),
	   this, SLOT(slotSeeSpecialStrings()) );

  // refresh every 2 seconds
  m_freeSpaceUpdateTimer = new QTimer( this );
  connect( m_freeSpaceUpdateTimer, SIGNAL(timeout()),
	   this, SLOT(slotUpdateFreeTempSpace()) );
  m_freeSpaceUpdateTimer->start(2000);
  slotUpdateFreeTempSpace();
}


K3bVideoDVDRippingWidget::~K3bVideoDVDRippingWidget()
{
}


int K3bVideoDVDRippingWidget::selectedVideoCodec() const
{
  return m_comboVideoCodec->currentItem();
}


void K3bVideoDVDRippingWidget::setSelectedVideoCodec( int codec )
{
  m_comboVideoCodec->setCurrentItem( codec );
}


int K3bVideoDVDRippingWidget::selectedAudioCodec() const
{
  return m_comboAudioCodec->currentItem();
}


void K3bVideoDVDRippingWidget::setSelectedAudioCodec( int codec )
{
  m_comboAudioCodec->setCurrentItem( codec );
}


int K3bVideoDVDRippingWidget::selectedAudioBitrate() const
{
  return s_bitrates[m_comboAudioCodec->currentItem()];
}


void K3bVideoDVDRippingWidget::setSelectedAudioBitrate( int bitrate )
{
  // select the bitrate closest to "bitrate"

  int bi = 0;
  int diff = 1000;
  for( int i = 0; s_bitrates[i]; ++i ) {
    int newDiff = s_bitrates[i] - bitrate;
    if( newDiff < 0 )
      newDiff = -1 * newDiff;
    if( newDiff < diff ) {
      diff = newDiff;
      bi = i;
    }
  }

  m_comboAudioBitrate->setCurrentItem( bi );
}


void K3bVideoDVDRippingWidget::slotUpdateFreeTempSpace()
{
  QString path = m_editBaseDir->url();

  if( !QFile::exists( path ) )
    path.truncate( path.findRev('/') );

  unsigned long size, avail;
  if( K3b::kbFreeOnFs( path, size, avail ) ) {
    m_labelFreeSpace->setText( KIO::convertSizeFromKB(avail) );
    if( avail < m_neededSize/1024 )
      m_labelNeededSpace->setPaletteForegroundColor( Qt::red );
    else
      m_labelNeededSpace->setPaletteForegroundColor( paletteForegroundColor() );
  }
  else {
    m_labelFreeSpace->setText("-");
    m_labelNeededSpace->setPaletteForegroundColor( paletteForegroundColor() );
  }
}


void K3bVideoDVDRippingWidget::setNeededSize( KIO::filesize_t size )
{
  m_neededSize = size;
  if( size > 0 )
    m_labelNeededSpace->setText( KIO::convertSize( size ) );
  else
    m_labelNeededSpace->setText( i18n("unknown") );

  slotUpdateFreeTempSpace();
}


void K3bVideoDVDRippingWidget::slotSeeSpecialStrings()
{
  QWhatsThis::display( i18n( "<p><b>Pattern special strings:</b>"
			     "<p>The following strings will be replaced with their respective meaning in every "
			     "track name.<br>"
                             "<p><table border=\"0\">"
			     "<tr><td></td><td><em>Meaning</em></td><td><em>Alternatives</em></td></tr>"
                             "<tr><td>%t</td><td>title number</td><td>%{t} or %{title_number}</td></tr>"
                             "<tr><td>%v</td><td>volume id (mostly the name of the Video DVD)</td><td>%{v} or %{volume_id}</td></tr>"
                             "<tr><td>%b</td><td>beautified volume id</td><td>%{b} or %{beautified_volume_id}</td></tr>"
                             "<tr><td>%l</td><td>two chars language code</td><td>%{l} or %{lang_code}</td></tr>"
                             "<tr><td>%n</td><td>language name</td><td>%{n} or %{lang_name}</td></tr>"
                             "<tr><td>%a</td><td>audio format (on the Video DVD)</td><td>%{a} or %{audio_format}</td></tr>"
                             "<tr><td>%c</td><td>number of audio channels (on the Video DVD)</td><td>%{c} or %{channels}</td></tr>"
                             "<tr><td>%s</td><td>size of the original video</td><td>%{s} or %{video_size}</td></tr>"
                             "<tr><td>%r</td><td>aspect ration of the original video</td><td>%{r} or %{aspect_ratio}</td></tr>"
                             "<tr><td>%d</td><td>current date</td><td>%{d} or %{date}</td></tr>"
                             "</table>"
			     "<p><em>Hint: K3b also accepts slight variantions of the long special strings. "
			     "One can, for example, leave out the underscores.</em>") );
}

#include "k3bvideodvdrippingwidget.moc"
