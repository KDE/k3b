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

#include <klistview.h>
#include <klocale.h>

#include <qcombobox.h>
#include <qspinbox.h>


// FIXME: these are mp3 bitrates. Do they also apply to AC3?
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
