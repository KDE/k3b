/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baudiodatasourceeditwidget.h"
#include "k3baudioeditorwidget.h"
#include "k3bmsfedit.h"

#include "k3baudiodatasource.h"

#include <KLocalizedString>

#include <QLayout>
#include <QLabel>
#include <QToolTip>
#include <QGridLayout>


K3b::AudioDataSourceEditWidget::AudioDataSourceEditWidget( QWidget* parent )
    : QWidget( parent ),
      m_source(0)
{
    m_editor = new K3b::AudioEditorWidget( this );
    m_editStartOffset = new K3b::MsfEdit( this );
    m_editEndOffset = new K3b::MsfEdit( this );

    QLabel* startLabel = new QLabel( i18n("Start Offset:"), this );
    QLabel* endLabel = new QLabel( i18n("End Offset:"), this );
    endLabel->setAlignment( Qt::AlignRight );

    QGridLayout* grid = new QGridLayout( this );
    grid->setContentsMargins( 0, 0, 0, 0 );

    grid->addWidget( startLabel, 0, 0 );
    grid->addWidget( m_editStartOffset, 1, 0 );
    grid->addWidget( m_editor, 0, 1, 2, 1 );
    grid->addWidget( endLabel, 0, 2 );
    grid->addWidget( m_editEndOffset, 1, 2 );
    grid->setColumnStretch( 1, 1 );

    // setup connections between the msfedits and the editor
    connect( m_editor, SIGNAL(rangeChanged(int,K3b::Msf,K3b::Msf)),
             this, SLOT(slotRangeModified(int,K3b::Msf,K3b::Msf)) );

    connect( m_editStartOffset, SIGNAL(valueChanged(K3b::Msf)),
             this, SLOT(slotStartOffsetEdited(K3b::Msf)) );

    connect( m_editEndOffset, SIGNAL(valueChanged(K3b::Msf)),
             this, SLOT(slotEndOffsetEdited(K3b::Msf)) );

    m_editor->setToolTip( i18n("Drag the edges of the highlighted area to define the portion of the "
                               "audio source you want to include in the Audio CD track. "
                               "You can also use the input windows to fine-tune your selection.") );
}


K3b::AudioDataSourceEditWidget::~AudioDataSourceEditWidget()
{
}


K3b::Msf K3b::AudioDataSourceEditWidget::startOffset() const
{
    return m_editor->rangeStart( m_rangeId );
}


K3b::Msf K3b::AudioDataSourceEditWidget::endOffset() const
{
    return m_editor->rangeEnd( m_rangeId );
}


void K3b::AudioDataSourceEditWidget::loadSource( K3b::AudioDataSource* source )
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
                                    palette().highlight() );

    m_editStartOffset->setMaximum( source->originalLength() );
    m_editEndOffset->setMaximum( source->originalLength() );

    m_editStartOffset->setValue( startOffset() );
    m_editEndOffset->setValue( endOffset() );
}


void K3b::AudioDataSourceEditWidget::saveSource()
{
    if( m_source ) {
        m_source->setStartOffset( startOffset() );
        // the source's end offset points after the last sector while
        // the editor widget returns the last used sector
        m_source->setEndOffset( endOffset()+1 );
    }
}


void K3b::AudioDataSourceEditWidget::setStartOffset( const K3b::Msf& msf )
{
    if( m_source ) {
        m_editor->modifyRange( m_rangeId,
                               msf,
                               endOffset() );
    }
}


void K3b::AudioDataSourceEditWidget::setEndOffset( const K3b::Msf& msf )
{
    if( m_source ) {
        m_editor->modifyRange( m_rangeId,
                               startOffset(),
                               msf );
    }
}


void K3b::AudioDataSourceEditWidget::slotRangeModified( int, const K3b::Msf& start, const K3b::Msf& end )
{
    m_editStartOffset->setValue( start );
    m_editEndOffset->setValue( end );
}

void K3b::AudioDataSourceEditWidget::slotStartOffsetEdited( const K3b::Msf& msf )
{
    if( m_source ) {
        m_editor->modifyRange( m_rangeId, msf, endOffset() );
    }
}


void K3b::AudioDataSourceEditWidget::slotEndOffsetEdited( const K3b::Msf& msf )
{
    if( m_source ) {
        m_editor->modifyRange( m_rangeId, startOffset(), msf );
    }
}


