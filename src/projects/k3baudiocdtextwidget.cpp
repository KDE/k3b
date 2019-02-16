/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3baudiocdtextwidget.h"
#include "ui_base_k3baudiocdtextallfieldsdialog.h"

#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "k3bcdtextvalidator.h"

#include <KLineEdit>
#include <KLocalizedString>

#include <QIcon>
#include <QCheckBox>
#include <QToolButton>
#include <QPushButton>
#include <QLayout>
#include <QGroupBox>


class K3b::AudioCdTextWidget::AllFieldsDialog : public QDialog, public Ui::base_K3bAudioCdTextAllFieldsDialog
{
public:
    AllFieldsDialog( QWidget* parent )
        : QDialog( parent) {
        setModal(true);
        setWindowTitle(i18n("CD-Text"));
        setupUi( this );
    }
};


K3b::AudioCdTextWidget::AudioCdTextWidget( QWidget* parent )
    : QWidget( parent ),
      m_doc(0)
{
    setupUi( this );

    m_allFieldsDlg = new AllFieldsDialog( this );

    m_buttonCopyTitle->setIcon( QIcon::fromTheme( "edit-copy" ) );
    m_buttonCopyPerformer->setIcon( QIcon::fromTheme( "edit-copy" ) );

    m_allFieldsDlg->m_buttonCopyTitle->setIcon( QIcon::fromTheme( "edit-copy" ) );
    m_allFieldsDlg->m_buttonCopyPerformer->setIcon( QIcon::fromTheme( "edit-copy" ) );
    m_allFieldsDlg->m_buttonCopySongwriter->setIcon( QIcon::fromTheme( "edit-copy" ) );
    m_allFieldsDlg->m_buttonCopyComposer->setIcon( QIcon::fromTheme( "edit-copy" ) );
    m_allFieldsDlg->m_buttonCopyArranger->setIcon( QIcon::fromTheme( "edit-copy" ) );

    QValidator* cdTextVal = new K3b::CdTextValidator( this );
    m_editTitle->setValidator( cdTextVal );
    m_editPerformer->setValidator( cdTextVal );

    m_allFieldsDlg->m_editTitle->setValidator( cdTextVal );
    m_allFieldsDlg->m_editPerformer->setValidator( cdTextVal );
    m_allFieldsDlg->m_editDisc_id->setValidator( cdTextVal );
    m_allFieldsDlg->m_editUpc_ean->setValidator( cdTextVal );
    m_allFieldsDlg->m_editMessage->setValidator( cdTextVal );
    m_allFieldsDlg->m_editArranger->setValidator( cdTextVal );
    m_allFieldsDlg->m_editSongwriter->setValidator( cdTextVal );
    m_allFieldsDlg->m_editComposer->setValidator( cdTextVal );

    connect( m_allFieldsDlg->m_buttonCopyTitle, SIGNAL(clicked()),
             this, SLOT(slotCopyTitle()) );
    connect( m_allFieldsDlg->m_buttonCopyPerformer, SIGNAL(clicked()),
             this, SLOT(slotCopyPerformer()) );
    connect( m_allFieldsDlg->m_buttonCopyArranger, SIGNAL(clicked()),
             this, SLOT(slotCopyArranger()) );
    connect( m_allFieldsDlg->m_buttonCopySongwriter, SIGNAL(clicked()),
             this, SLOT(slotCopySongwriter()) );
    connect( m_allFieldsDlg->m_buttonCopyComposer, SIGNAL(clicked()),
             this, SLOT(slotCopyComposer()) );
    connect(m_buttonCopyTitle, SIGNAL(clicked()),
             this, SLOT(slotCopyTitle()) );
    connect(m_buttonCopyPerformer, SIGNAL(clicked()),
             this, SLOT(slotCopyPerformer()) );

    connect( m_buttonMoreFields, SIGNAL(clicked()),
             this, SLOT(slotMoreFields()) );
}


K3b::AudioCdTextWidget::~AudioCdTextWidget()
{
}

void K3b::AudioCdTextWidget::load( K3b::AudioDoc* doc )
{
    m_doc = doc;
    m_groupCdText->setChecked( doc->cdText() );

    m_editTitle->setText( doc->title() );
    m_editPerformer->setText( doc->artist() );

    m_allFieldsDlg->m_editTitle->setText( doc->title() );
    m_allFieldsDlg->m_editPerformer->setText( doc->artist() );
    m_allFieldsDlg->m_editDisc_id->setText( doc->disc_id() );
    m_allFieldsDlg->m_editUpc_ean->setText( doc->upc_ean() );
    m_allFieldsDlg->m_editArranger->setText( doc->arranger() );
    m_allFieldsDlg->m_editSongwriter->setText( doc->songwriter() );
    m_allFieldsDlg->m_editComposer->setText( doc->composer() );
    m_allFieldsDlg->m_editMessage->setText( doc->cdTextMessage() );
}

void K3b::AudioCdTextWidget::save( K3b::AudioDoc* doc )
{
    m_doc = doc;
    doc->writeCdText( m_groupCdText->isChecked() );

    // we save the title and artist values from the main view since
    // the dialog is only updated before it is shown
    doc->setTitle( m_editTitle->text() );
    doc->setArtist( m_editPerformer->text() );
    doc->setDisc_id( m_allFieldsDlg->m_editDisc_id->text() );
    doc->setUpc_ean( m_allFieldsDlg->m_editUpc_ean->text() );
    doc->setArranger( m_allFieldsDlg->m_editArranger->text() );
    doc->setSongwriter( m_allFieldsDlg->m_editSongwriter->text() );
    doc->setComposer( m_allFieldsDlg->m_editComposer->text() );
    doc->setCdTextMessage( m_allFieldsDlg->m_editMessage->text() );
}


void K3b::AudioCdTextWidget::slotMoreFields()
{
    // update dlg to current state
    m_allFieldsDlg->m_editTitle->setText( m_editTitle->text() );
    m_allFieldsDlg->m_editPerformer->setText( m_editPerformer->text() );

    // save old settings
    QString title = m_allFieldsDlg->m_editTitle->text();
    QString performer = m_allFieldsDlg->m_editPerformer->text();
    QString disc_id = m_allFieldsDlg->m_editDisc_id->text();
    QString upc_ean = m_allFieldsDlg->m_editUpc_ean->text();
    QString arranger = m_allFieldsDlg->m_editArranger->text();
    QString songwriter = m_allFieldsDlg->m_editSongwriter->text();
    QString composer = m_allFieldsDlg->m_editComposer->text();
    QString message = m_allFieldsDlg->m_editMessage->text();

    m_allFieldsDlg->m_editTitle->setFocus( Qt::PopupFocusReason );

    // exec dlg
    if( m_allFieldsDlg->exec() == QDialog::Accepted ) {
        // accept new entries
        m_editTitle->setText( m_allFieldsDlg->m_editTitle->text() );
        m_editPerformer->setText( m_allFieldsDlg->m_editPerformer->text() );
    }
    else {
        // reset
        m_allFieldsDlg->m_editTitle->setText( title );
        m_allFieldsDlg->m_editPerformer->setText( performer );
        m_allFieldsDlg->m_editDisc_id->setText( disc_id );
        m_allFieldsDlg->m_editUpc_ean->setText( upc_ean );
        m_allFieldsDlg->m_editArranger->setText( arranger );
        m_allFieldsDlg->m_editSongwriter->setText( songwriter );
        m_allFieldsDlg->m_editComposer->setText( composer );
        m_allFieldsDlg->m_editMessage->setText( message );
    }
}


void K3b::AudioCdTextWidget::setChecked( bool b )
{
    m_groupCdText->setChecked( b );
}

bool K3b::AudioCdTextWidget::isChecked() const
{
    return m_groupCdText->isChecked();
}


void K3b::AudioCdTextWidget::slotCopyTitle()
{
    K3b::AudioTrack* track = m_doc->firstTrack();
    while( track ) {
        track->setTitle( m_allFieldsDlg->isVisible()
                         ? m_allFieldsDlg->m_editTitle->text()
                         : m_editTitle->text() );
        track = track->next();
    }
}

void K3b::AudioCdTextWidget::slotCopyPerformer()
{
    K3b::AudioTrack* track = m_doc->firstTrack();
    while( track ) {
        track->setPerformer( m_allFieldsDlg->isVisible()
                             ? m_allFieldsDlg->m_editPerformer->text()
                             : m_editPerformer->text() );
        track = track->next();
    }
}

void K3b::AudioCdTextWidget::slotCopyArranger()
{
    K3b::AudioTrack* track = m_doc->firstTrack();
    while( track ) {
        track->setArranger( m_allFieldsDlg->m_editArranger->text() );
        track = track->next();
    }
}

void K3b::AudioCdTextWidget::slotCopySongwriter()
{
    K3b::AudioTrack* track = m_doc->firstTrack();
    while( track ) {
        track->setSongwriter( m_allFieldsDlg->m_editSongwriter->text() );
        track = track->next();
    }
}

void K3b::AudioCdTextWidget::slotCopyComposer()
{
    K3b::AudioTrack* track = m_doc->firstTrack();
    while( track ) {
        track->setComposer( m_allFieldsDlg->m_editComposer->text() );
        track = track->next();
    }
}



