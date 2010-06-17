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

#include "k3bmixedburndialog.h"
#include "k3bmixeddoc.h"
#include "k3bmixedview.h"

#include "k3bdataimagesettingswidget.h"
#include "k3bdatadoc.h"
#include "k3baudiodoc.h"
#include "k3bdevice.h"
#include "k3bwriterselectionwidget.h"
#include "k3btempdirselectionwidget.h"
#include "k3bisooptions.h"
#include "k3bglobals.h"
#include "k3baudiocdtextwidget.h"
#include "k3bdatamodewidget.h"
#include "k3bmsf.h"
#include "k3bstdguiitems.h"
#include "k3bwritingmodewidget.h"
#include "k3bexternalbinmanager.h"
#include "k3bversion.h"
#include "k3bcore.h"
//#include "k3baudiotrackplayer.h"
#include "k3bintmapcombobox.h"

#include <KApplication>
#include <KConfig>
#include <KDebug>
#include <KLocale>
#include <KMessageBox>

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QTabWidget>
#include <QToolButton>
#include <QToolTip>
#include <QVariant>


K3b::MixedBurnDialog::MixedBurnDialog( K3b::MixedDoc* doc, QWidget *parent )
    : K3b::ProjectBurnDialog( doc, parent ),
      m_doc(doc)
{
    prepareGui();

    setTitle( i18n("Mixed Project"), i18np("1 track (%2 minutes)",
                                           "%1 tracks (%2 minutes)",
                                           m_doc->numOfTracks(),m_doc->length().toString()) );

    m_checkOnlyCreateImage->hide();

    // create cd-text page
    m_cdtextWidget = new K3b::AudioCdTextWidget( this );
    addPage( m_cdtextWidget, i18n("CD-Text") );

    // create image settings tab
    m_imageSettingsWidget = new K3b::DataImageSettingsWidget( this );
    addPage( m_imageSettingsWidget, i18n("Filesystem") );

    setupSettingsPage();

    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    m_optionGroupLayout->addItem( spacer );

    connect( m_checkNormalize, SIGNAL(toggled(bool)), this, SLOT(slotNormalizeToggled(bool)) );
    connect( m_checkCacheImage, SIGNAL(toggled(bool)), this, SLOT(slotCacheImageToggled(bool)) );
    connect( m_writerSelectionWidget, SIGNAL(writingAppChanged(K3b::WritingApp)), this, SLOT(slotToggleAll()) );
    connect( m_writingModeWidget, SIGNAL(writingModeChanged(WritingMode)), this, SLOT(slotToggleAll()) );
}


void K3b::MixedBurnDialog::setupSettingsPage()
{
    QWidget* w = new QWidget( this );

    QGroupBox* groupDataMode = new QGroupBox( i18n("Datatrack Mode"), w );
    m_dataModeWidget = new K3b::DataModeWidget( groupDataMode );

    QGroupBox* groupNormalize = new QGroupBox( i18n("Misc"), w );
    m_checkNormalize = K3b::StdGuiItems::normalizeCheckBox( groupNormalize );

    QGroupBox* groupMixedType = new QGroupBox( i18n("Mixed Mode Type"), w );
    m_comboMixedModeType = new K3b::IntMapComboBox( groupMixedType );

    m_comboMixedModeType->insertItem( K3b::MixedDoc::DATA_SECOND_SESSION,
                                      i18n("Data in second session (CD-Extra)"),
                                      i18n("<em>Blue book CD</em>"
                                           "<br>K3b will create a multisession CD with "
                                           "2 sessions. The first session will contain all "
                                           "audio tracks and the second session will contain "
                                           "a mode 2 form 1 data track."
                                           "<br>This mode is based on the <em>Blue book</em> "
                                           "standard (also known as <em>Extended Audio CD</em>, "
                                           "<em>CD-Extra</em>, or <em>CD Plus</em>) "
                                           "and has the advantage that a hifi audio "
                                           "CD player will only recognize the first session "
                                           "and ignore the second session with the data track."
                                           "<br>If the CD is intended to be used in a hifi audio CD player "
                                           "this is the recommended mode."
                                           "<br>Some older CD-ROMs may have problems reading "
                                           "a blue book CD since it is a multisession CD.") );
    m_comboMixedModeType->insertItem( K3b::MixedDoc::DATA_FIRST_TRACK,
                                      i18n("Data in first track"),
                                      i18n("K3b will write the data track before all "
                                           "audio tracks.") );
    m_comboMixedModeType->insertItem( K3b::MixedDoc::DATA_LAST_TRACK,
                                      i18n("Data in last track"),
                                      i18n("K3b will write the data track after all "
                                           "audio tracks.") );
    m_comboMixedModeType->addGlobalWhatsThisText( QString(),
                                                  i18n("<b>Caution:</b> The last two modes should only be used for CDs that are unlikely to "
                                                       "be played on a hifi audio CD player."
                                                       "<br>It could lead to problems with some older "
                                                       "hifi audio CD players that try to play the data track.") );

    QGridLayout* grid = new QGridLayout( w );
    grid->addWidget( groupMixedType, 0, 0 );
    grid->addWidget( groupDataMode, 1, 0 );
    grid->addWidget( groupNormalize, 2, 0 );
    grid->setRowStretch( 3, 1 );

    addPage( w, i18n("Misc") );
}


void K3b::MixedBurnDialog::slotStartClicked()
{
    // FIXME: this should not be done via the doc. So remove all gui stuff from the doc
//  static_cast<K3b::MixedView*>(m_doc->view())->player()->stop();
    K3b::ProjectBurnDialog::slotStartClicked();
}


void K3b::MixedBurnDialog::saveSettingsToProject()
{
    K3b::ProjectBurnDialog::saveSettingsToProject();

    m_doc->setMixedType( (K3b::MixedDoc::MixedType)m_comboMixedModeType->selectedValue() );

    m_cdtextWidget->save( m_doc->audioDoc() );

    m_doc->audioDoc()->setNormalize( m_checkNormalize->isChecked() );

    // save iso image settings
    K3b::IsoOptions o = m_doc->dataDoc()->isoOptions();
    m_imageSettingsWidget->save( o );
    m_doc->dataDoc()->setIsoOptions( o );

    m_doc->dataDoc()->setDataMode( m_dataModeWidget->dataMode() );

    // save image file path
    m_doc->setTempDir( m_tempDirSelectionWidget->tempPath() );
}


void K3b::MixedBurnDialog::readSettingsFromProject()
{
    K3b::ProjectBurnDialog::readSettingsFromProject();

    m_checkNormalize->setChecked( m_doc->audioDoc()->normalize() );

    if( !m_doc->tempDir().isEmpty() )
        m_tempDirSelectionWidget->setTempPath( m_doc->tempDir() );

    m_comboMixedModeType->setSelectedValue( m_doc->mixedType() );

    m_cdtextWidget->load( m_doc->audioDoc() );

    m_imageSettingsWidget->load( m_doc->dataDoc()->isoOptions() );

    m_dataModeWidget->setDataMode( m_doc->dataDoc()->dataMode() );

    toggleAll();
}


void K3b::MixedBurnDialog::loadSettings( const KConfigGroup& c )
{
    K3b::ProjectBurnDialog::loadSettings( c );

    m_cdtextWidget->setChecked( c.readEntry( "cd_text", false ) );
    m_checkNormalize->setChecked( c.readEntry( "normalize", false ) );

    // load mixed type
    if( c.readEntry( "mixed_type" ) == "last_track" )
        m_comboMixedModeType->setSelectedValue( K3b::MixedDoc::DATA_LAST_TRACK );
    else if( c.readEntry( "mixed_type" ) == "first_track" )
        m_comboMixedModeType->setSelectedValue( K3b::MixedDoc::DATA_FIRST_TRACK );
    else
        m_comboMixedModeType->setSelectedValue( K3b::MixedDoc::DATA_SECOND_SESSION );

    m_dataModeWidget->loadConfig(c);

    K3b::IsoOptions o = K3b::IsoOptions::load( c );
    m_imageSettingsWidget->load( o );

    toggleAll();
}


void K3b::MixedBurnDialog::saveSettings( KConfigGroup c )
{
    K3b::ProjectBurnDialog::saveSettings(c);

    c.writeEntry( "cd_text", m_cdtextWidget->isChecked() );
    c.writeEntry( "normalize", m_checkNormalize->isChecked() );

    // save mixed type
    switch( m_comboMixedModeType->selectedValue() ) {
    case K3b::MixedDoc::DATA_LAST_TRACK:
        c.writeEntry( "mixed_type", "last_track" );
        break;
    case K3b::MixedDoc::DATA_FIRST_TRACK:
        c.writeEntry( "mixed_type", "first_track" );
        break;
    default:
        c.writeEntry( "mixed_type", "second_session" );
    }

    m_dataModeWidget->saveConfig(c);

    K3b::IsoOptions o;
    m_imageSettingsWidget->save( o );
    o.save( c );

    if( m_tempDirSelectionWidget->isEnabled() ) {
        m_tempDirSelectionWidget->saveConfig();
    }
}


void K3b::MixedBurnDialog::toggleAll()
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
    }

    m_checkCacheImage->setEnabled( !m_checkOnlyCreateImage->isChecked() &&
                                   onTheFly );
    if( !onTheFly )
        m_checkCacheImage->setChecked( true );

    m_cdtextWidget->setEnabled( !m_checkOnlyCreateImage->isChecked() &&
                                cdText &&
                                m_writingModeWidget->writingMode() != K3b::WritingModeTao );
    if( !cdText || m_writingModeWidget->writingMode() == K3b::WritingModeTao  )
        m_cdtextWidget->setChecked( false );
}


void K3b::MixedBurnDialog::slotNormalizeToggled( bool on )
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


void K3b::MixedBurnDialog::slotCacheImageToggled( bool on )
{
    if( on ) {
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


#include "k3bmixedburndialog.moc"

