/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baudiodatasourceeditwidget.h"
#include "k3baudioeditorwidget.h"
#include <k3bmsfedit.h>

#include <k3baudiodatasource.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qtooltip.h>

#include <klocale.h>
#include <kdialog.h>


K3bAudioDataSourceEditWidget::K3bAudioDataSourceEditWidget( QWidget* parent, const char* name )
  : QWidget( parent, name ),
    m_source(0)
{
  m_editor = new K3bAudioEditorWidget( this );
  m_editStartOffset = new K3bMsfEdit( this );
  m_editEndOffset = new K3bMsfEdit( this );

  QLabel* startLabel = new QLabel( i18n("Start Offset") + ":", this );
  QLabel* endLabel = new QLabel( i18n("End Offset") + ":", this );
  endLabel->setAlignment( Qt::AlignRight );

  QGridLayout* grid = new QGridLayout( this );
  grid->setMargin( 0 );
  grid->setSpacing( KDialog::spacingHint() );

  grid->addWidget( startLabel, 0, 0 );
  grid->addWidget( m_editStartOffset, 1, 0 );
  grid->addMultiCellWidget( m_editor, 0, 1, 1, 1 );
  grid->addWidget( endLabel, 0, 2 );
  grid->addWidget( m_editEndOffset, 1, 2 );
  grid->setColStretch( 1, 1 );

  // setup connections between the msfedits and the editor
  connect( m_editor, SIGNAL(rangeChanged(int, const K3b::Msf&, const K3b::Msf&)),
	   this, SLOT(slotRangeModified(int, const K3b::Msf&, const K3b::Msf&)) );

  connect( m_editStartOffset, SIGNAL(valueChanged(const K3b::Msf&)),
	   this, SLOT(slotStartOffsetEdited(const K3b::Msf&)) );

  connect( m_editEndOffset, SIGNAL(valueChanged(const K3b::Msf&)),
	   this, SLOT(slotEndOffsetEdited(const K3b::Msf&)) );

  QToolTip::add( m_editor, i18n("Drag the edges of the highlighted area to define the portion of the "
				"audio source you want to include in the Audio CD track. "
				"You can also use the input windows to fine-tune your selection.") );
}


K3bAudioDataSourceEditWidget::~K3bAudioDataSourceEditWidget()
{
}


K3b::Msf K3bAudioDataSourceEditWidget::startOffset() const
{
  return m_editor->rangeStart( m_rangeId );
}


K3b::Msf K3bAudioDataSourceEditWidget::endOffset() const
{
  return m_editor->rangeEnd( m_rangeId );
}


void K3bAudioDataSourceEditWidget::loadSource( K3bAudioDataSource* source )
{
  m_source = source;

  // remove old range
  m_editor->removeRange( m_rangeId );

  // and add the proper new range
  // the source's end offset points after the last sector while 
  // the editor widget returns the last used sector
  m_editor->setLength( source->originalLength() );
  m_rangeId = m_editor->addRange( source->startOffset(),
				  source->endOffset() == 0 
				  ? source->originalLength()-1
				  : source->endOffset()-1,
				  false, 
				  false,
				  i18n("Used part of the audio source"),
				  colorGroup().highlight() );

  m_editStartOffset->setMaxValue( source->originalLength().lba() );
  m_editEndOffset->setMaxValue( source->originalLength().lba() );

  m_editStartOffset->setMsfValue( startOffset() );
  m_editEndOffset->setMsfValue( endOffset() );
}


void K3bAudioDataSourceEditWidget::saveSource()
{
  if( m_source ) {
    m_source->setStartOffset( startOffset() );
    // the source's end offset points after the last sector while 
    // the editor widget returns the last used sector
    m_source->setEndOffset( endOffset()+1 );
  }
}


void K3bAudioDataSourceEditWidget::setStartOffset( const K3b::Msf& msf )
{
  if( m_source ) {
    m_editor->modifyRange( m_rangeId,
			   msf,
			   endOffset() );
  }
}


void K3bAudioDataSourceEditWidget::setEndOffset( const K3b::Msf& msf )
{
  if( m_source ) {
    m_editor->modifyRange( m_rangeId,
			   startOffset(),
			   msf );
  }
}


void K3bAudioDataSourceEditWidget::slotRangeModified( int, const K3b::Msf& start, const K3b::Msf& end )
{
  m_editStartOffset->setMsfValue( start );
  m_editEndOffset->setMsfValue( end );
}

void K3bAudioDataSourceEditWidget::slotStartOffsetEdited( const K3b::Msf& msf )
{
  if( m_source ) {
    m_editor->modifyRange( m_rangeId, msf, endOffset() );
  }
}


void K3bAudioDataSourceEditWidget::slotEndOffsetEdited( const K3b::Msf& msf )
{
  if( m_source ) {
    m_editor->modifyRange( m_rangeId, startOffset(), msf );
  }
}

#include "k3baudiodatasourceeditwidget.moc"
