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


#include "k3baudioburndialog.h"
#include "k3baudioview.h"
#include "k3baudiotrack.h"
#include "k3baudiocdtracksource.h"
#include "k3bcore.h"
#include "k3baudiodoc.h"
#include "k3bdevice.h"
#include "k3bwriterselectionwidget.h"
#include "k3btempdirselectionwidget.h"
#include "k3baudiocdtextwidget.h"
#include "k3bglobals.h"
#include "k3bstdguiitems.h"
#include "k3bwritingmodewidget.h"
#include "k3bexternalbinmanager.h"

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>

#include <qtoolbutton.h>
#include <QtCore/QStringList>
#include <qpoint.h>

#include <qspinbox.h>
#include <QShowEvent>
#include <QGridLayout>

#include <KI18n/KLocalizedString>
#include <KDELibs4Support/KDE/KStandardDirs>
#include <KConfigCore/KConfig>
#include <kmessagebox.h>
#include <kvbox.h>


K3b::AudioBurnDialog::AudioBurnDialog(K3b::AudioDoc* _doc, QWidget *parent )
    : K3b::ProjectBurnDialog( _doc, parent ),
      m_doc(_doc)
{
    prepareGui();

    setTitle( i18n("Audio Project"),
              i18np("1 track (%2 minutes)", "%1 tracks (%2 minutes)",
                    m_doc->numOfTracks(),m_doc->length().toString()) );

    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    m_optionGroupLayout->addItem( spacer );

    // create cd-text page
    m_cdtextWidget = new K3b::AudioCdTextWidget( this );
    addPage( m_cdtextWidget, i18n("CD-Text") );

    // create advanced tab
    // ----------------------------------------------------------
    QWidget* advancedTab = new QWidget( this );
    QGridLayout* advancedTabGrid = new QGridLayout( advancedTab );

    QGroupBox* advancedSettingsGroup = new QGroupBox( i18n("Settings"), advancedTab );
    m_checkNormalize = K3b::StdGuiItems::normalizeCheckBox( advancedSettingsGroup );
    QVBoxLayout* advancedSettingsGroupLayout = new QVBoxLayout( advancedSettingsGroup );
    advancedSettingsGroupLayout->addWidget( m_checkNormalize );

    QGroupBox* advancedGimmickGroup = new QGroupBox( i18n("Gimmicks"), advancedTab );
    m_checkHideFirstTrack = new QCheckBox( i18n( "Hide first track" ), advancedGimmickGroup );
    QVBoxLayout* advancedGimmickGroupLayout = new QVBoxLayout( advancedGimmickGroup );
    advancedGimmickGroupLayout->addWidget( m_checkHideFirstTrack );

    m_audioRippingGroup = new QGroupBox( i18n("Audio Ripping"), advancedTab );
    KHBox* box = new KHBox( m_audioRippingGroup );
    box->setStretchFactor(new QLabel( i18n("Paranoia mode:"), box ), 1 );
    m_comboParanoiaMode = K3b::StdGuiItems::paranoiaModeComboBox( box );
    box = new KHBox( m_audioRippingGroup );
    box->setStretchFactor( new QLabel( i18n("Read retries:"), box ), 1 );
    m_spinAudioRippingReadRetries = new QSpinBox( box );
    m_spinAudioRippingReadRetries->setRange( 1, 128 );
    m_checkAudioRippingIgnoreReadErrors = new QCheckBox( i18n("Ignore read errors"), m_audioRippingGroup );
    QVBoxLayout* audioRippingGroupLayout = new QVBoxLayout( m_audioRippingGroup );
    audioRippingGroupLayout->addWidget( box );
    audioRippingGroupLayout->addWidget( m_checkAudioRippingIgnoreReadErrors );

    advancedTabGrid->addWidget( advancedSettingsGroup, 0, 0 );
    advancedTabGrid->addWidget( advancedGimmickGroup, 1, 0 );
    advancedTabGrid->addWidget( m_audioRippingGroup, 2, 0 );
    advancedTabGrid->setRowStretch( 3, 1 );

    addPage( advancedTab, i18n("Advanced") );

    connect( m_checkNormalize, SIGNAL(toggled(bool)), this, SLOT(slotNormalizeToggled(bool)) );
    connect( m_checkCacheImage, SIGNAL(toggled(bool)), this, SLOT(slotCacheImageToggled(bool)) );

    // ToolTips
    // -------------------------------------------------------------------------
    m_checkHideFirstTrack->setToolTip( i18n("Hide the first track in the first pregap") );

    // What's This info
    // -------------------------------------------------------------------------
    m_checkHideFirstTrack->setWhatsThis(
        i18n("<p>If this option is checked K3b will <em>hide</em> the first track."
             "<p>The audio CD standard uses pregaps before every track on the CD. "
             "By default these last for 2 seconds and are silent. In DAO mode it "
             "is possible to have longer pregaps that contain some audio. In this case "
             "the first pregap will contain the complete first track."
             "<p>You will need to seek back from the beginning of the CD to listen to "
             "the first track. Try it, it is quite amusing."
             "<p><b>This feature is only available in DAO mode when writing with cdrdao.") );
}

K3b::AudioBurnDialog::~AudioBurnDialog(){
}


void K3b::AudioBurnDialog::slotStartClicked()
{
//  static_cast<K3b::AudioView*>(m_doc->view())->player()->stop();
    K3b::ProjectBurnDialog::slotStartClicked();
}


void K3b::AudioBurnDialog::saveSettingsToProject()
{
    K3b::ProjectBurnDialog::saveSettingsToProject();

    m_doc->setTempDir( m_tempDirSelectionWidget->tempPath() );
    m_doc->setHideFirstTrack( m_checkHideFirstTrack->isChecked() );
    m_doc->setNormalize( m_checkNormalize->isChecked() );

    // -- save Cd-Text ------------------------------------------------
    m_cdtextWidget->save( m_doc );

    // audio ripping
    m_doc->setAudioRippingParanoiaMode( m_comboParanoiaMode->currentText().toInt() );
    m_doc->setAudioRippingRetries( m_spinAudioRippingReadRetries->value() );
    m_doc->setAudioRippingIgnoreReadErrors( m_checkAudioRippingIgnoreReadErrors->isChecked() );

    doc()->setTempDir( m_tempDirSelectionWidget->tempPath() );
}


void K3b::AudioBurnDialog::readSettingsFromProject()
{
    K3b::ProjectBurnDialog::readSettingsFromProject();

    m_checkHideFirstTrack->setChecked( m_doc->hideFirstTrack() );
    m_checkNormalize->setChecked( m_doc->normalize() );

    // read CD-Text ------------------------------------------------------------
    m_cdtextWidget->load( m_doc );

    // audio ripping
    m_comboParanoiaMode->setCurrentIndex( m_doc->audioRippingParanoiaMode() );
    m_checkAudioRippingIgnoreReadErrors->setChecked( m_doc->audioRippingIgnoreReadErrors() );
    m_spinAudioRippingReadRetries->setValue( m_doc->audioRippingRetries() );

    if( !doc()->tempDir().isEmpty() )
        m_tempDirSelectionWidget->setTempPath( doc()->tempDir() );

    toggleAll();
}


void K3b::AudioBurnDialog::loadSettings( const KConfigGroup& c )
{
    K3b::ProjectBurnDialog::loadSettings( c );

    m_cdtextWidget->setChecked( c.readEntry( "cd_text", true ) );
    m_checkHideFirstTrack->setChecked( c.readEntry( "hide_first_track", false ) );
    m_checkNormalize->setChecked( c.readEntry( "normalize", false ) );

    m_comboParanoiaMode->setCurrentIndex( c.readEntry( "paranoia mode", 0 ) );
    m_checkAudioRippingIgnoreReadErrors->setChecked( c.readEntry( "ignore read errors", true ) );
    m_spinAudioRippingReadRetries->setValue( c.readEntry( "read retries", 5 ) );

    toggleAll();
}


void K3b::AudioBurnDialog::saveSettings( KConfigGroup c )
{
    K3b::ProjectBurnDialog::saveSettings( c );

    c.writeEntry( "cd_text", m_cdtextWidget->isChecked() );
    c.writeEntry( "hide_first_track", m_checkHideFirstTrack->isChecked() );
    c.writeEntry( "normalize", m_checkNormalize->isChecked() );

    c.writeEntry( "paranoia mode", m_comboParanoiaMode->currentText() );
    c.writeEntry( "ignore read errors", m_checkAudioRippingIgnoreReadErrors->isChecked() );
    c.writeEntry( "read retries", m_spinAudioRippingReadRetries->value() );
}

void K3b::AudioBurnDialog::toggleAll()
{
    K3b::ProjectBurnDialog::toggleAll();

    bool cdrecordOnTheFly = false;
    bool cdrecordCdText = false;
    if ( k3bcore->externalBinManager()->binObject("cdrecord") ) {
        cdrecordOnTheFly = k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "audio-stdin" );
        cdrecordCdText = k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "cdtext" );
    }

    // cdrdao always knows onthefly and cdtext
    bool onTheFly = true;
    bool cdText = true;
    if( m_writingModeWidget->writingMode() == K3b::WritingModeTao ||
        m_writingModeWidget->writingMode() == K3b::WritingModeRaw ||
        m_writerSelectionWidget->writingApp() == K3b::WritingAppCdrecord ) {
        onTheFly = cdrecordOnTheFly;
        cdText = cdrecordCdText;
        m_checkHideFirstTrack->setChecked(false);
        m_checkHideFirstTrack->setEnabled(false);
    }
    else {
        m_checkHideFirstTrack->setEnabled( !m_checkOnlyCreateImage->isChecked() );
        m_cdtextWidget->setEnabled( !m_checkOnlyCreateImage->isChecked() );
    }

    m_checkCacheImage->setEnabled( !m_checkOnlyCreateImage->isChecked() &&
                                   onTheFly );
    if( !onTheFly )
        m_checkCacheImage->setChecked( true );
    m_cdtextWidget->setEnabled( !m_checkOnlyCreateImage->isChecked() &&
                                cdText &&
                                m_writingModeWidget->writingMode() != K3b::WritingModeTao );
    if( !cdText || m_writingModeWidget->writingMode() == K3b::WritingModeTao )
        m_cdtextWidget->setChecked(false);
}


void K3b::AudioBurnDialog::showEvent( QShowEvent* e )
{
    // we only show the audio ripping options when there are audio cd track sources
    bool showRipOptions = false;
    if( m_doc->firstTrack() ) {
        K3b::AudioTrack* track = m_doc->firstTrack();
        K3b::AudioDataSource* source = track->firstSource();

        while( source ) {

            if( dynamic_cast<K3b::AudioCdTrackSource*>(source) ) {
                showRipOptions = true;
                break;
            }

            // next source
            source = source->next();
            if( !source ) {
                track = track->next();
                if( track )
                    source = track->firstSource();
            }
        }
    }

    m_audioRippingGroup->setVisible( showRipOptions );

    K3b::ProjectBurnDialog::showEvent(e);
}


void K3b::AudioBurnDialog::slotNormalizeToggled( bool on )
{
    if( on ) {
        // we are not able to normalize in on-the-fly mode
        if( !k3bcore->externalBinManager()->foundBin( "normalize" ) ) {
            KMessageBox::sorry( this, i18n("<p><b>External program <em>normalize</em> is not installed.</b>"
                                           "<p>K3b uses <em>normalize</em> (http://normalize.nongnu.org/) "
                                           "to normalize audio tracks. In order to "
                                           "use this functionality, please install it first.") );
            m_checkNormalize->setChecked( false );
        }
        else if( !m_checkCacheImage->isChecked() && !m_checkOnlyCreateImage->isChecked() ) {
            if( KMessageBox::warningYesNo( this, i18n("<p>K3b is not able to normalize audio tracks when burning on-the-fly. "
                                                      "The external program used for this task only supports normalizing a set "
                                                      "of audio files."),
                                           QString(),
                                           KGuiItem( i18n("Disable normalization") ),
                                           KGuiItem( i18n("Disable on-the-fly burning") ),
                                           "audioProjectNormalizeOrOnTheFly" ) == KMessageBox::Yes )
                m_checkNormalize->setChecked( false );
            else
                m_checkCacheImage->setChecked( true );
        }
    }
}


void K3b::AudioBurnDialog::slotCacheImageToggled( bool on )
{
    if( !on ) {
        if( m_checkNormalize->isChecked() ) {
            if( KMessageBox::warningYesNo( this, i18n("<p>K3b is not able to normalize audio tracks when burning on-the-fly. "
                                                      "The external program used for this task only supports normalizing a set "
                                                      "of audio files."),
                                           QString(),
                                           KGuiItem( i18n("Disable normalization") ),
                                           KGuiItem( i18n("Disable on-the-fly burning") ),
                                           "audioProjectNormalizeOrOnTheFly" ) == KMessageBox::Yes )
                m_checkNormalize->setChecked( false );
            else
                m_checkCacheImage->setChecked( true );
        }
    }
}


