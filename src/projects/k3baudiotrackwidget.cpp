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

#include "k3baudiotrackwidget.h"
#include "k3baudioeditorwidget.h"
#include "k3baudiotrack.h"

#include <k3bmsfedit.h>
#include <k3bvalidators.h>
#include <k3bcdtextvalidator.h>

#include <qlabel.h>
#include <qcheckbox.h>
#include <qtooltip.h>

#include <q3groupbox.h>
#include <qtabwidget.h>
#include <klineedit.h>
#include <klocale.h>
#include <kdebug.h>



K3bAudioTrackWidget::K3bAudioTrackWidget( const QList<K3bAudioTrack*>& tracks,
                                          QWidget* parent )
    : QWidget( parent ),
      m_tracks(tracks)
{
    setupUi( this );

    m_labelPostGap->setBuddy( m_editPostGap );
    //TODO kde4 port me
    //m_labelPostGap->setToolTip( QToolTip::textFor( m_editPostGap ) );
    //m_labelPostGap->setWhatsThis( Q3WhatsThis::textFor( m_editPostGap ) );

    // no post-gap for the last track
    m_editPostGap->setDisabled( tracks.count() == 1 && !tracks.first()->next() );

    K3bCdTextValidator* val = new K3bCdTextValidator( this );
    m_editSongwriter->setValidator( val );
    m_editArranger->setValidator( val );
    m_editComposer->setValidator( val );
    m_editMessage->setValidator( val );
    m_editTitle->setValidator( val );
    m_editPerformer->setValidator( val );
    m_editIsrc->setValidator( K3bValidators::isrcValidator( this ) );

    load();
}


K3bAudioTrackWidget::~K3bAudioTrackWidget()
{
}


void K3bAudioTrackWidget::load()
{
    if( !m_tracks.isEmpty() ) {

        K3bAudioTrack* track = m_tracks.first();

        m_editPostGap->setMsfValue( track->postGap() );

        m_editTitle->setText( track->title() );
        m_editPerformer->setText( track->artist() );
        m_editArranger->setText( track->arranger() );
        m_editSongwriter->setText( track->songwriter() );
        m_editComposer->setText( track->composer() );
        m_editIsrc->setText( track->isrc() );
        m_editMessage->setText( track->cdTextMessage() );

        m_checkCopyPermitted->setChecked( !track->copyProtection() );
        m_checkPreemphasis->setChecked( track->preEmp() );

        // load CD-Text for all other tracks
        for( int i = 1; i < m_tracks.count(); ++i ) {
            track = m_tracks[i];

            // FIXME: handle different post-gaps
            // m_editPostGap->setMsfValue( track->postGap() );

            if( track->title() != m_editTitle->text() )
                m_editTitle->setText( QString() );

            if( track->artist() != m_editPerformer->text() )
                m_editPerformer->setText( QString() );

            if( track->arranger() != m_editArranger->text() )
                m_editArranger->setText( QString() );

            if( track->songwriter() != m_editSongwriter->text() )
                m_editSongwriter->setText( QString() );

            if( track->composer() != m_editComposer->text() )
                m_editComposer->setText( QString() );

            if( track->isrc() != m_editIsrc->text() )
                m_editIsrc->setText( QString() );

            if( track->cdTextMessage() != m_editMessage->text() )
                m_editMessage->setText( QString() );
        }

        if( m_tracks.count() > 1 ) {
            m_checkCopyPermitted->setNoChange();
            m_checkPreemphasis->setNoChange();
        }
    }

    m_editTitle->setFocus();
}


void K3bAudioTrackWidget::save()
{
    // save CD-Text, preemphasis, and copy protection for all tracks. no problem
    for( int i = 0; i < m_tracks.count(); ++i ) {
        K3bAudioTrack* track = m_tracks[i];

        if( m_editTitle->isModified() )
            track->setTitle( m_editTitle->text() );

        if( m_editPerformer->isModified() )
            track->setArtist( m_editPerformer->text() );

        if( m_editArranger->isModified() )
            track->setArranger( m_editArranger->text() );

        if( m_editSongwriter->isModified() )
            track->setSongwriter( m_editSongwriter->text() );

        if( m_editComposer->isModified() )
            track->setComposer( m_editComposer->text() );

        if( m_editIsrc->isModified() )
            track->setIsrc( m_editIsrc->text() );

        if( m_editMessage->isModified() )
            track->setCdTextMessage( m_editMessage->text() );

        if( m_checkCopyPermitted->state() != QCheckBox::NoChange )
            track->setCopyProtection( !m_checkCopyPermitted->isChecked() );

        if( m_checkPreemphasis->state() != QCheckBox::NoChange )
            track->setPreEmp( m_checkPreemphasis->isChecked() );

        track->setIndex0( track->length() - m_editPostGap->msfValue() );
    }
}

#include "k3baudiotrackwidget.moc"
