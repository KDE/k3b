/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baudiotracksplitdialog.h"
#include "k3baudiotrack.h"
#include "k3baudioeditorwidget.h"

#include <k3bmsf.h>
#include <k3bmsfedit.h>

#include <klocale.h>

#include <qlabel.h>
#include <qframe.h>
#include <qlayout.h>


K3bAudioTrackSplitDialog::K3bAudioTrackSplitDialog( K3bAudioTrack* track, QWidget* parent, const char* name )
  : KDialogBase( KDialogBase::Plain, i18n("Split Audio Track"), 
		 KDialogBase::Ok|KDialogBase::Cancel,
		 KDialogBase::Ok, parent, name ),
    m_track(track)
{
  QFrame* frame = plainPage();
  
  m_editorWidget = new K3bAudioEditorWidget( frame );
  m_msfEdit = new K3bMsfEdit( frame );

  QGridLayout* layout = new QGridLayout( frame );
  layout->setMargin( 0 );
  layout->setSpacing( spacingHint() );

  layout->addMultiCellWidget( new QLabel( i18n("Please select the position where the track should be split."),
			      frame ), 0, 0, 0, 1 );
  layout->addMultiCellWidget( m_editorWidget, 1, 1, 0, 1 );
  layout->addWidget( m_msfEdit, 2, 1 );
  layout->addWidget( new QLabel( i18n("Remaining length of split track:"), frame ), 2, 0 );
  layout->setColStretch( 0, 1 );

  // load the track
  m_editorWidget->setLength( m_track->length() );
  m_msfEdit->setValue( m_track->length().lba() / 2 );
  m_firstRange = m_editorWidget->addRange( 0, m_track->length().lba() / 2-1, 
					   true, false, Qt::red );
  m_secondRange = m_editorWidget->addRange( m_track->length().lba() / 2, m_track->length()-1, 
					    true, false, Qt::blue );


  connect( m_editorWidget, SIGNAL(rangeChanged(int, const K3b::Msf&, const K3b::Msf&)),
	   this, SLOT(slotRangeModified(int, const K3b::Msf&, const K3b::Msf&)) );
  connect( m_msfEdit, SIGNAL(valueChanged(const K3b::Msf&)),
	   this, SLOT(slotMsfChanged(const K3b::Msf&)) );
}


K3bAudioTrackSplitDialog::~K3bAudioTrackSplitDialog()
{
}


void K3bAudioTrackSplitDialog::slotRangeModified( int id, const K3b::Msf& start, const K3b::Msf& end )
{
  if( id == m_firstRange ) {
    m_editorWidget->modifyRange( m_secondRange, end+1, m_track->length()-1 );
    m_msfEdit->setMsfValue( end+1 ); // start of next track
  }
  else {
    m_msfEdit->setMsfValue( start-1 );
    m_editorWidget->modifyRange( m_firstRange, 0, start-1 );
  }
}


void K3bAudioTrackSplitDialog::slotMsfChanged( const K3b::Msf& msf )
{
  m_editorWidget->modifyRange( m_firstRange, 0, msf-1 );
  m_editorWidget->modifyRange( m_secondRange, msf, m_track->length()-1 );
}


K3b::Msf K3bAudioTrackSplitDialog::currentSplitPos() const
{
  return m_msfEdit->msfValue();
}


bool K3bAudioTrackSplitDialog::getSplitPos( K3bAudioTrack* track, K3b::Msf& val, 
					    QWidget* parent, const char* name )
{
  K3bAudioTrackSplitDialog d( track, parent, name );
  if( d.exec() == QDialog::Accepted ) {
    val = d.currentSplitPos();
    return true;
  }
  else
    return false;
}

#include "k3baudiotracksplitdialog.moc"
