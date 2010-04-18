/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3baudiotrackdialog.h"
#include "k3baudiotrackwidget.h"
#include "k3baudiotrack.h"
#include "k3bmsf.h"
#include "k3bmsfedit.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>

#include <KLocale>
#include <kio/global.h>


// TODO: three modes:
//    1. Only one track with only one source
//         show decoder tech info, cdtext, options and the track editor without showing anything
//         about sources
//    2. Only one track with multiple sources
//         like the above but with the possibility to edit the sources
//    3. multiple tracks
//         do only show cd-text and options (eventuelle index0)


K3b::AudioTrackDialog::AudioTrackDialog( const QList<K3b::AudioTrack*>& tracks, QWidget *parent )
    : KDialog( parent)
{
    m_tracks = tracks;

    setCaption(i18n("Audio Track Properties"));
    setButtons(Ok|Cancel|Apply);
    setDefaultButton(Ok);
    setModal(true);
    connect(this,SIGNAL(okClicked()), this, SLOT(slotOk()));
    connect(this,SIGNAL(applyClicked()),this,SLOT(slotApply()));

    setupGui();
}


K3b::AudioTrackDialog::~AudioTrackDialog()
{
}


void K3b::AudioTrackDialog::slotOk()
{
    slotApply();
    done(0);
}


void K3b::AudioTrackDialog::slotApply()
{
    m_audioTrackWidget->save();
}


void K3b::AudioTrackDialog::setupGui()
{
    QFrame* frame = new QFrame();
    setMainWidget( frame );

    QVBoxLayout* mainLayout = new QVBoxLayout( frame );
    mainLayout->setContentsMargins( 0, 0, 0, 0 );

    m_audioTrackWidget = new K3b::AudioTrackWidget( m_tracks, frame );
    mainLayout->addWidget( m_audioTrackWidget );
}


void K3b::AudioTrackDialog::updateTrackLengthDisplay()
{
//   K3b::Msf len = m_editTrackEnd->msfValue() - m_editTrackStart->msfValue();
//   m_displayLength->setText( len.toString() );
//   m_displaySize->setText( KIO::convertSize(len.audioBytes()) );
}

#include "k3baudiotrackdialog.moc"
