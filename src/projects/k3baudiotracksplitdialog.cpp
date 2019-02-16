/*
 *
 * Copyright (C) 2004-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baudiotracksplitdialog.h"
#include "k3baudiotrack.h"
#include "k3baudioeditorwidget.h"

#include "k3bmsf.h"
#include "k3bmsfedit.h"

#include <KLocalizedString>
#include <KActionCollection>

#include <QEvent>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QAction>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLayout>
#include <QMenu>


K3b::AudioTrackSplitDialog::AudioTrackSplitDialog( K3b::AudioTrack* track, QWidget* parent )
    : QDialog( parent),
      m_track(track)
{
    setWindowTitle(i18n("Split Audio Track"));

    m_editorWidget = new K3b::AudioEditorWidget( this );
    m_msfEditStart = new K3b::MsfEdit( this );
    m_msfEditEnd = new K3b::MsfEdit( this );

    QVBoxLayout* layout = new QVBoxLayout( this );

    // FIXME: After the string freeze replace the text with a better one explaning how to use this dialog
    layout->addWidget( new QLabel( i18n("Please select the position where the track should be split."),
                                   this ) );
    layout->addWidget( m_editorWidget );

    QHBoxLayout* rangeLayout = new QHBoxLayout;
    rangeLayout->addWidget( new QLabel( i18n("Split track at:"), this ), 1 );
    rangeLayout->addWidget( m_msfEditStart, 0 );
    rangeLayout->addWidget( new QLabel( " - ", this ), 0, Qt::AlignCenter );
    rangeLayout->addWidget( m_msfEditEnd, 0 );
    layout->addLayout( rangeLayout );

    m_editorWidget->setAllowOverlappingRanges( false );
    m_editorWidget->enableRangeSelection( true );
    m_editorWidget->installEventFilter( this );

    connect( m_editorWidget, SIGNAL(rangeChanged(int,K3b::Msf,K3b::Msf)),
             this, SLOT(slotRangeModified(int,K3b::Msf,K3b::Msf)) );
    connect( m_editorWidget, SIGNAL(selectedRangeChanged(int)),
             this, SLOT(slotRangeSelectionChanged(int)) );
    connect( m_msfEditStart, SIGNAL(valueChanged(K3b::Msf)),
             this, SLOT(slotMsfEditChanged(K3b::Msf)) );
    connect( m_msfEditEnd, SIGNAL(valueChanged(K3b::Msf)),
             this, SLOT(slotMsfEditChanged(K3b::Msf)) );

    setupActions();

    // load the track
    m_editorWidget->setLength( m_track->length() );

    // default split
    K3b::Msf mid = m_track->length().lba() / 2;
    m_editorWidget->addRange( 0, mid-1 );
    m_editorWidget->addRange( mid, m_track->length()-1 );

    QDialogButtonBox* buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this );
    connect( buttonBox, SIGNAL(accepted()), SLOT(accept()) );
    connect( buttonBox, SIGNAL(rejected()), SLOT(reject()) );
    layout->addWidget( buttonBox );

    slotRangeSelectionChanged( 0 );
}


K3b::AudioTrackSplitDialog::~AudioTrackSplitDialog()
{
}


void K3b::AudioTrackSplitDialog::setupActions()
{
    m_popupMenu = new QMenu( this );

    QAction* actionSplitHere = new QAction( this );
    actionSplitHere->setText( i18n("Split Here") );
    connect( actionSplitHere, SIGNAL(triggered()), this, SLOT(slotSplitHere()) );

    QAction* actionRemoveRange = new QAction( this );
    actionRemoveRange->setText( i18n("Remove part") );
    connect( actionRemoveRange, SIGNAL(triggered()), this, SLOT(slotRemoveRange()) );

    m_popupMenu->addAction( actionSplitHere );
    m_popupMenu->addAction( actionRemoveRange );
}


void K3b::AudioTrackSplitDialog::slotRangeModified( int id, const K3b::Msf& start, const K3b::Msf& end )
{
    if( id == m_editorWidget->selectedRange() ) {
        m_msfEditStart->blockSignals( true );
        m_msfEditEnd->blockSignals( true );

        m_msfEditStart->setValue( start );
        m_msfEditEnd->setValue( end );

        m_msfEditStart->blockSignals( false );
        m_msfEditEnd->blockSignals( false );
    }
}


void K3b::AudioTrackSplitDialog::slotMsfEditChanged( const K3b::Msf& )
{
    m_editorWidget->modifyRange( m_editorWidget->selectedRange(), m_msfEditStart->value(), m_msfEditEnd->value() );
}


void K3b::AudioTrackSplitDialog::slotRangeSelectionChanged( int id )
{
    if( id > 0 ) {
        m_msfEditStart->blockSignals( true );
        m_msfEditEnd->blockSignals( true );

        m_msfEditStart->setValue( m_editorWidget->rangeStart( id ) );
        m_msfEditEnd->setValue( m_editorWidget->rangeEnd( id ) );
        m_msfEditStart->setEnabled( true );
        m_msfEditEnd->setEnabled( true );

        m_msfEditStart->blockSignals( false );
        m_msfEditEnd->blockSignals( false );
    }
    else {
        m_msfEditStart->setEnabled( false );
        m_msfEditEnd->setEnabled( false );
    }
}


void K3b::AudioTrackSplitDialog::splitAt( const QPoint& p )
{
    int id = m_editorWidget->findRange( p.x() );
    if( id ) {
        K3b::Msf msf = m_editorWidget->posToMsf( p.x() );
        m_editorWidget->addRange( msf+1, m_editorWidget->rangeEnd( id ) );
        m_editorWidget->modifyRange( id, m_editorWidget->rangeStart( id ), msf );
        slotRangeSelectionChanged( m_editorWidget->selectedRange() );
    }
}


bool K3b::AudioTrackSplitDialog::eventFilter( QObject* o, QEvent* e )
{
    if( o == m_editorWidget ) {
        if( e->type() == QEvent::MouseButtonDblClick ) {
            QMouseEvent* me = static_cast<QMouseEvent*>( e );
            splitAt( me->pos() );
        }
        else if( e->type() == QEvent::ContextMenu ) {
            QContextMenuEvent* ce = static_cast<QContextMenuEvent*>( e );
            ce->accept();
            m_lastClickPosition = ce->pos();
            if( m_editorWidget->findRange( ce->pos().x() ) > 0 )
                m_popupMenu->popup( ce->globalPos() );
        }
    }

    return QDialog::eventFilter( o, e );
}


void K3b::AudioTrackSplitDialog::slotSplitHere()
{
    splitAt( m_lastClickPosition );
}


void K3b::AudioTrackSplitDialog::slotRemoveRange()
{
    m_editorWidget->removeRange( m_editorWidget->findRange( m_lastClickPosition.x() ) );
}


void K3b::AudioTrackSplitDialog::splitTrack( K3b::AudioTrack* track,
					   QWidget* parent )
{
    if ( !track ) {
        return;
    }

    K3b::AudioTrackSplitDialog d( track, parent );
    if( d.exec() == QDialog::Accepted ) {
        QList<int> ranges = d.m_editorWidget->allRanges();
        // we split the track at all range ends and just delete those that relate to the gaps in between
        K3b::Msf pos = 0;
        for( QList<int>::const_iterator it = ranges.constBegin();
             it != ranges.constEnd(); ++it ) {

            // delete the unwanted part
            if( d.m_editorWidget->rangeStart( *it ) > pos ) {
                // split so the range's start is the first frame of the new track
                K3b::AudioTrack* nextTrack = track->split( d.m_editorWidget->rangeStart( *it ) - pos );
                delete track;
                track = nextTrack;
            }

            // create a new track part for the range itself
            pos = d.m_editorWidget->rangeStart( *it );
            if( d.m_editorWidget->rangeEnd( *it ) < d.m_editorWidget->length()-1 ) {
                // split so the range's end is the last frame in the old track
                // and thus, the range's end + 1 the first frame in the new track
                track = track->split( d.m_editorWidget->rangeEnd( *it ) - pos + 1 );
            }
            pos = d.m_editorWidget->rangeEnd( *it )+1;
        }

        // remove the last unwanted part
        if( pos < d.m_editorWidget->length() ) {
            delete track;
        }
    }
}


