/*
 *
 * Copyright (C) 2007-2009 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bmediacopydialog.h"

#include "k3bmediaselectioncombobox.h"

#include "k3bcdcopyjob.h"
#include "k3bclonejob.h"
#include "k3bdvdcopyjob.h"

#include "k3bwriterselectionwidget.h"
#include "k3btempdirselectionwidget.h"
#include "k3bcore.h"
#include "k3bstdguiitems.h"
#include "k3bdevice.h"
#include "k3bdevicemanager.h"
#include "k3bburnprogressdialog.h"
#include "k3bglobals.h"
#include "k3bexternalbinmanager.h"
#include "k3bthememanager.h"
#include "k3bwritingmodewidget.h"
#include "k3bapplication.h"
#include "k3bmediacache.h"

#include <KConfig>
#include <KSharedConfig>
#include <KIconLoader>
#include <KLocalizedString>
#include <KGuiItem>
#include <KMessageBox>
#include <KStandardGuiItem>

#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QToolTip>
#include <QTabWidget>


K3b::MediaCopyDialog::MediaCopyDialog( QWidget *parent )
    : K3b::InteractionDialog( parent,
                            i18n("Disk Copy"),
                            i18n("and CD Cloning"),
                            START_BUTTON|CANCEL_BUTTON,
                            START_BUTTON,
                            "Disk Copy" )
{
    QWidget* main = mainWidget();

    QGridLayout* mainGrid = new QGridLayout( main );
    mainGrid->setContentsMargins( 0, 0, 0, 0 );

    QGroupBox* groupSource = new QGroupBox( i18n("Source Medium"), main );
    m_comboSourceDevice = new K3b::MediaSelectionComboBox( groupSource );
    m_comboSourceDevice->setWantedMediumType( K3b::Device::MEDIA_ALL );
    m_comboSourceDevice->setWantedMediumState( K3b::Device::STATE_COMPLETE|K3b::Device::STATE_INCOMPLETE );
    QHBoxLayout* groupSourceLayout = new QHBoxLayout( groupSource );
    groupSourceLayout->addWidget( m_comboSourceDevice );

    m_writerSelectionWidget = new K3b::WriterSelectionWidget( main );
    m_writerSelectionWidget->setWantedMediumState( K3b::Device::STATE_EMPTY );

    // tab widget --------------------
    QTabWidget* tabWidget = new QTabWidget( main );

    //
    // option tab --------------------
    //
    QWidget* optionTab = new QWidget( tabWidget );
    QGridLayout* optionTabGrid = new QGridLayout( optionTab );

    QGroupBox* groupCopyMode = new QGroupBox( i18n("Copy Mode"), optionTab );
    m_comboCopyMode = new QComboBox( groupCopyMode );
    m_comboCopyMode->addItem( i18n("Normal Copy") );
    m_comboCopyMode->addItem( i18n("Clone Copy") );
    QHBoxLayout* groupCopyModeLayout = new QHBoxLayout( groupCopyMode );
    groupCopyModeLayout->addWidget( m_comboCopyMode );

    QGroupBox* groupWritingMode = new QGroupBox( i18n("Writing Mode"), optionTab );
    m_writingModeWidget = new K3b::WritingModeWidget( groupWritingMode );
    QHBoxLayout* groupWritingModeLayout = new QHBoxLayout( groupWritingMode );
    groupWritingModeLayout->addWidget( m_writingModeWidget );

    QGroupBox* groupCopies = new QGroupBox( i18n("Copies"), optionTab );
    QLabel* pixLabel = new QLabel( groupCopies );
    pixLabel->setPixmap( SmallIcon( "tools-media-optical-copy", KIconLoader::SizeMedium ) );
    pixLabel->setScaledContents( false );
    m_spinCopies = new QSpinBox( groupCopies );
    m_spinCopies->setRange( 1, 999 );
    QHBoxLayout* groupCopiesLayout = new QHBoxLayout( groupCopies );
    groupCopiesLayout->addWidget( pixLabel );
    groupCopiesLayout->addWidget( m_spinCopies );

    QGroupBox* groupOptions = new QGroupBox( i18n("Settings"), optionTab );
    m_checkSimulate = K3b::StdGuiItems::simulateCheckbox( groupOptions );
    m_checkCacheImage = K3b::StdGuiItems::createCacheImageCheckbox( groupOptions );
    m_checkOnlyCreateImage = K3b::StdGuiItems::onlyCreateImagesCheckbox( groupOptions );
    m_checkDeleteImages = K3b::StdGuiItems::removeImagesCheckbox( groupOptions );
    m_checkVerifyData = K3b::StdGuiItems::verifyCheckBox( groupOptions );
    QVBoxLayout* groupOptionsLayout = new QVBoxLayout( groupOptions );
    groupOptionsLayout->addWidget( m_checkSimulate );
    groupOptionsLayout->addWidget( m_checkCacheImage );
    groupOptionsLayout->addWidget( m_checkOnlyCreateImage );
    groupOptionsLayout->addWidget( m_checkDeleteImages );
    groupOptionsLayout->addWidget( m_checkVerifyData );
    groupOptionsLayout->addStretch( 1 );

    optionTabGrid->addWidget( groupCopyMode, 0, 0 );
    optionTabGrid->addWidget( groupWritingMode, 1, 0 );
    optionTabGrid->addWidget( groupOptions, 0, 1, 3, 1 );
    optionTabGrid->addWidget( groupCopies, 2, 0 );
    optionTabGrid->setRowStretch( 2, 1 );
    optionTabGrid->setColumnStretch( 1, 1 );

    tabWidget->addTab( optionTab, i18n("&Options") );


    //
    // image tab ------------------
    //
    QWidget* imageTab = new QWidget( tabWidget );
    QGridLayout* imageTabGrid = new QGridLayout( imageTab );

    m_tempDirSelectionWidget = new K3b::TempDirSelectionWidget( imageTab );

    imageTabGrid->addWidget( m_tempDirSelectionWidget, 0, 0 );

    tabWidget->addTab( imageTab, i18n("&Image") );


    //
    // advanced tab ------------------
    //
    QWidget* advancedTab = new QWidget( tabWidget );
    QGridLayout* advancedTabGrid = new QGridLayout( advancedTab );

    m_groupAdvancedDataOptions = new QGroupBox( i18n("Data"), advancedTab );
    QGridLayout* groupAdvancedDataOptionsLayout = new QGridLayout( m_groupAdvancedDataOptions );
    m_spinDataRetries = new QSpinBox( m_groupAdvancedDataOptions );
    m_spinDataRetries->setRange( 1, 128 );
    m_checkIgnoreDataReadErrors = K3b::StdGuiItems::ignoreAudioReadErrorsCheckBox( m_groupAdvancedDataOptions );
    m_checkNoCorrection = new QCheckBox( i18n("No error correction"), m_groupAdvancedDataOptions );
    groupAdvancedDataOptionsLayout->addWidget( new QLabel( i18n("Read retries:"), m_groupAdvancedDataOptions ), 0, 0 );
    groupAdvancedDataOptionsLayout->addWidget( m_spinDataRetries, 0, 1 );
    groupAdvancedDataOptionsLayout->addWidget( m_checkIgnoreDataReadErrors, 1, 0, 1, 2 );
    groupAdvancedDataOptionsLayout->addWidget( m_checkNoCorrection, 2, 0, 1, 2 );
    groupAdvancedDataOptionsLayout->setRowStretch( 4, 1 );

    m_groupAdvancedAudioOptions = new QGroupBox( i18n("Audio"), advancedTab );
    QGridLayout* groupAdvancedAudioOptionsLayout = new QGridLayout( m_groupAdvancedAudioOptions );
    m_spinAudioRetries = new QSpinBox( m_groupAdvancedAudioOptions );
    m_spinAudioRetries->setRange( 1, 128 );
    m_checkIgnoreAudioReadErrors = K3b::StdGuiItems::ignoreAudioReadErrorsCheckBox( m_groupAdvancedAudioOptions );
    m_comboParanoiaMode = K3b::StdGuiItems::paranoiaModeComboBox( m_groupAdvancedAudioOptions );
    m_checkReadCdText = new QCheckBox( i18n("Copy CD-Text"), m_groupAdvancedAudioOptions );
    groupAdvancedAudioOptionsLayout->addWidget( new QLabel( i18n("Read retries:"), m_groupAdvancedAudioOptions ), 0, 0 );
    groupAdvancedAudioOptionsLayout->addWidget( m_spinAudioRetries, 0, 1 );
    groupAdvancedAudioOptionsLayout->addWidget( m_checkIgnoreAudioReadErrors, 1, 0, 1, 2 );
    groupAdvancedAudioOptionsLayout->addWidget( new QLabel( i18n("Paranoia mode:"), m_groupAdvancedAudioOptions ), 2, 0 );
    groupAdvancedAudioOptionsLayout->addWidget( m_comboParanoiaMode, 2, 1 );
    groupAdvancedAudioOptionsLayout->addWidget( m_checkReadCdText, 3, 0, 1, 2 );
    groupAdvancedAudioOptionsLayout->setRowStretch( 4, 1 );

    advancedTabGrid->addWidget( m_groupAdvancedDataOptions, 0, 1 );
    advancedTabGrid->addWidget( m_groupAdvancedAudioOptions, 0, 0 );

    tabWidget->addTab( advancedTab, i18n("&Advanced") );

    mainGrid->addWidget( groupSource, 0, 0  );
    mainGrid->addWidget( m_writerSelectionWidget, 1, 0  );
    mainGrid->addWidget( tabWidget, 2, 0 );
    mainGrid->setRowStretch( 2, 1 );


    connect( m_comboSourceDevice, SIGNAL(selectionChanged(K3b::Device::Device*)), this, SLOT(slotToggleAll()) );
    connect( m_comboSourceDevice, SIGNAL(selectionChanged(K3b::Device::Device*)),
             this, SLOT(slotToggleAll()) );
    connect( m_writerSelectionWidget, SIGNAL(writerChanged()), this, SLOT(slotToggleAll()) );
    connect( m_writerSelectionWidget, SIGNAL(writerChanged(K3b::Device::Device*)),
             m_writingModeWidget, SLOT(setDevice(K3b::Device::Device*)) );
    connect( m_writingModeWidget, SIGNAL(writingModeChanged(WritingMode)), this, SLOT(slotToggleAll()) );
    connect( m_checkCacheImage, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );
    connect( m_checkSimulate, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );
    connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );
    connect( m_comboCopyMode, SIGNAL(activated(int)), this, SLOT(slotToggleAll()) );
    connect( m_checkReadCdText, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );

    m_checkIgnoreDataReadErrors->setToolTip( i18n("Skip unreadable data sectors") );
    m_checkNoCorrection->setToolTip( i18n("Disable the source drive's error correction") );
    m_checkReadCdText->setToolTip( i18n("Copy CD-Text from the source CD if available.") );

    m_checkNoCorrection->setWhatsThis( i18n("<p>If this option is checked K3b will disable the "
                                            "source drive's ECC/EDC error correction. This way sectors "
                                            "that are unreadable by intention can be read."
                                            "<p>This may be useful for cloning CDs with copy "
                                            "protection based on corrupted sectors.") );
    m_checkReadCdText->setWhatsThis( i18n("<p>If this option is checked K3b will search for CD-Text on the source CD. "
                                          "Disable it if your CD drive has problems with reading CD-Text or you want "
                                          "to stick to CDDB info.") );
    m_checkIgnoreDataReadErrors->setWhatsThis( i18n("<p>If this option is checked and K3b is not able to read a data sector from the "
                                                    "source medium it will be replaced with zeros on the resulting copy.") );

    m_comboCopyMode->setWhatsThis(
        "<p><b>" + i18n("Normal Copy") + "</b>"
       + i18n("<p>This is the normal copy mode for DVD, Blu-ray, and most CD media types. "
               "It allows copying Audio CDs, multi and single session Data Media, and "
               "Enhanced Audio CDs (an Audio CD containing an additional data session)."
               "<p>For Video CDs please use the CD Cloning mode.")
        + "<p><b>" + i18n("Clone Copy") + "</b>"
        + i18n("<p>In CD Cloning mode K3b performs a raw copy of the CD. That means it does "
               "not care about the content but simply copies the CD bit by bit. It may be used "
               "to copy Video CDs or CDs which contain erroneous sectors."
               "<p><b>Caution:</b> Only single session CDs can be cloned.") );
}


K3b::MediaCopyDialog::~MediaCopyDialog()
{
}


void K3b::MediaCopyDialog::init()
{
    slotToggleAll();
}


void K3b::MediaCopyDialog::setReadingDevice( K3b::Device::Device* dev )
{
    m_comboSourceDevice->setSelectedDevice( dev );
}


K3b::Device::Device* K3b::MediaCopyDialog::readingDevice() const
{
    return m_comboSourceDevice->selectedDevice();
}


void K3b::MediaCopyDialog::slotStartClicked()
{
    //
    // Let's check the available size
    //
    if( m_checkCacheImage->isChecked() || m_checkOnlyCreateImage->isChecked() ) {
        if( neededSize() > m_tempDirSelectionWidget->freeTempSpace() ) {
            if( KMessageBox::warningContinueCancel( this, i18n("There does not seem to be enough free space in the temporary folder. "
                                                               "Write anyway?") ) == KMessageBox::Cancel )
                return;
        }
    }

    K3b::Device::Device* readDev = m_comboSourceDevice->selectedDevice();
    K3b::Device::Device* burnDev = m_writerSelectionWidget->writerDevice();
    K3b::Medium sourceMedium = k3bappcore->mediaCache()->medium( readDev );
    K3b::Medium burnMedium = k3bappcore->mediaCache()->medium( burnDev );

    K3b::JobProgressDialog* dlg = 0;
    if (m_checkOnlyCreateImage->isChecked())
        dlg = new K3b::JobProgressDialog(parentWidget());
    else
        dlg = new K3b::BurnProgressDialog(parentWidget());
    dlg->setAttribute(Qt::WA_DeleteOnClose);    // Memory-leak issue fixed!

    K3b::BurnJob* burnJob = 0;

    if( m_comboCopyMode->currentIndex() == 1 ) {
        //
        // check for m_tempDirSelectionWidget->tempPath() and
        // m_tempDirSelectionWidget-tempPath() + ".toc"
        //
        if( QFileInfo( m_tempDirSelectionWidget->tempPath() ).isFile() ) {
            if( KMessageBox::warningContinueCancel( this,
                                                    i18n("Do you want to overwrite %1?",m_tempDirSelectionWidget->tempPath()),
                                                    i18n("File Exists"),
                                                    KStandardGuiItem::overwrite() )
                != KMessageBox::Continue )
                return;
        }

        if( QFileInfo( m_tempDirSelectionWidget->tempPath() + ".toc" ).isFile() ) {
            if( KMessageBox::warningContinueCancel( this,
                                                    i18n("Do you want to overwrite %1?",m_tempDirSelectionWidget->tempPath() + ".toc"),
                                                    i18n("File Exists"),
                                                    KStandardGuiItem::overwrite() )
                != KMessageBox::Continue )
                return;
        }

        K3b::CloneJob* job = new K3b::CloneJob( dlg, this );

        job->setWriterDevice( m_writerSelectionWidget->writerDevice() );
        job->setReaderDevice( m_comboSourceDevice->selectedDevice() );
        job->setImagePath( m_tempDirSelectionWidget->tempPath() );
        job->setNoCorrection( m_checkNoCorrection->isChecked() );
        job->setRemoveImageFiles( m_checkDeleteImages->isChecked() && !m_checkOnlyCreateImage->isChecked() );
        job->setOnlyCreateImage( m_checkOnlyCreateImage->isChecked() );
        job->setSimulate( m_checkSimulate->isChecked() );
        job->setWriteSpeed( m_writerSelectionWidget->writerSpeed() );
        job->setCopies( m_checkSimulate->isChecked() ? 1 : m_spinCopies->value() );
        job->setReadRetries( m_spinDataRetries->value() );

        burnJob = job;
    }
    else if ( sourceMedium.diskInfo().mediaType() & K3b::Device::MEDIA_CD_ALL ) {
        K3b::CdCopyJob* job = new K3b::CdCopyJob( dlg, this );

        job->setWriterDevice( m_writerSelectionWidget->writerDevice() );
        job->setReaderDevice( m_comboSourceDevice->selectedDevice() );
        job->setSpeed( m_writerSelectionWidget->writerSpeed() );
        job->setSimulate( m_checkSimulate->isChecked() );
        job->setOnTheFly( !m_checkCacheImage->isChecked() );
        job->setKeepImage( !m_checkDeleteImages->isChecked() || m_checkOnlyCreateImage->isChecked() );
        job->setOnlyCreateImage( m_checkOnlyCreateImage->isChecked() );
        job->setTempPath( m_tempDirSelectionWidget->plainTempPath() );
        job->setCopies( m_checkSimulate->isChecked() ? 1 : m_spinCopies->value() );
        job->setParanoiaMode( m_comboParanoiaMode->currentText().toInt() );
        job->setDataReadRetries( m_spinDataRetries->value() );
        job->setAudioReadRetries( m_spinAudioRetries->value() );
        job->setCopyCdText( m_checkReadCdText->isChecked() );
        job->setIgnoreDataReadErrors( m_checkIgnoreDataReadErrors->isChecked() );
        job->setIgnoreAudioReadErrors( m_checkIgnoreAudioReadErrors->isChecked() );
        job->setNoCorrection( m_checkNoCorrection->isChecked() );
        job->setWritingMode( m_writingModeWidget->writingMode() );

        burnJob = job;
    }
    else if ( sourceMedium.diskInfo().mediaType() & ( K3b::Device::MEDIA_DVD_ALL|K3b::Device::MEDIA_BD_ALL ) ) {
        K3b::DvdCopyJob* job = new K3b::DvdCopyJob( dlg, this );

        job->setWriterDevice( m_writerSelectionWidget->writerDevice() );
        job->setReaderDevice( m_comboSourceDevice->selectedDevice() );
        job->setImagePath( m_tempDirSelectionWidget->tempPath() );
        job->setRemoveImageFiles( m_checkDeleteImages->isChecked() && !m_checkOnlyCreateImage->isChecked() );
        job->setOnlyCreateImage( m_checkOnlyCreateImage->isChecked() );
        job->setSimulate( m_checkSimulate->isChecked() );
        job->setOnTheFly( !m_checkCacheImage->isChecked() );
        job->setWriteSpeed( m_writerSelectionWidget->writerSpeed() );
        job->setCopies( m_checkSimulate->isChecked() ? 1 : m_spinCopies->value() );
        job->setWritingMode( m_writingModeWidget->writingMode() );
        job->setIgnoreReadErrors( m_checkIgnoreDataReadErrors->isChecked() );
        job->setReadRetries( m_spinDataRetries->value() );
        job->setVerifyData( m_checkVerifyData->isChecked() );

        burnJob = job;
    }
    else {
        // do not translate this as it is not intended to be included in the stable version!
        KMessageBox::sorry( this, "Ups", "No copy support for this source media type yet." );
        return;
    }

    hide();

    dlg->startJob( burnJob );

    delete burnJob;

    if( KConfigGroup( KSharedConfig::openConfig(), "General Options" ).readEntry( "keep action dialogs open", false ) )
        show();
    else
        close();
}


void K3b::MediaCopyDialog::toggleAll()
{
    updateOverrideDevice();

    K3b::Device::Device* readDev = m_comboSourceDevice->selectedDevice();
    K3b::Device::Device* burnDev = m_writerSelectionWidget->writerDevice();
    K3b::Medium sourceMedium = k3bappcore->mediaCache()->medium( readDev );
    K3b::Medium burnMedium = k3bappcore->mediaCache()->medium( burnDev );

    if ( burnDev ) {
        if( readDev != burnDev &&
            burnMedium.diskInfo().mediaType() & K3b::Device::MEDIA_DVD_PLUS_ALL ) {
            // no simulation support for DVD+R(W) media
            m_checkSimulate->setChecked(false);
            m_checkSimulate->setEnabled(false);
        }
        else {
            m_checkSimulate->setDisabled( m_checkOnlyCreateImage->isChecked() );
        }
    }
    else {
        m_checkSimulate->setEnabled( !m_checkOnlyCreateImage->isChecked() );
    }

    m_checkDeleteImages->setEnabled( !m_checkOnlyCreateImage->isChecked() && m_checkCacheImage->isChecked() );
    m_spinCopies->setDisabled( m_checkSimulate->isChecked() || m_checkOnlyCreateImage->isChecked() );
    m_tempDirSelectionWidget->setDisabled( !m_checkCacheImage->isChecked() && !m_checkOnlyCreateImage->isChecked() );
    m_writerSelectionWidget->setDisabled( m_checkOnlyCreateImage->isChecked() );
    m_checkCacheImage->setEnabled( !m_checkOnlyCreateImage->isChecked() );
    m_writingModeWidget->setEnabled( !m_checkOnlyCreateImage->isChecked() );

    // FIXME: no verification for CD yet
    m_checkVerifyData->setDisabled( sourceMedium.diskInfo().mediaType() & K3b::Device::MEDIA_CD_ALL ||
                                    sourceMedium.content() & K3b::Medium::ContentAudio ||
                                    m_checkSimulate->isChecked() );

    // we can only clone single session CDs
    if( K3b::Device::isCdMedia( sourceMedium.diskInfo().mediaType() ) ) {
        m_writerSelectionWidget->setWantedMediumType( K3b::Device::MEDIA_WRITABLE_CD );
        m_writerSelectionWidget->setSupportedWritingApps( K3b::WritingAppCdrecord );

        if ( sourceMedium.toc().sessions() == 1 ) {
            m_comboCopyMode->setEnabled( true );
        }
        else {
            m_comboCopyMode->setEnabled( false );
            m_comboCopyMode->setCurrentIndex( 0 );
        }
    }
    else {
        m_writerSelectionWidget->setSupportedWritingApps( K3b::WritingAppGrowisofs|K3b::WritingAppCdrecord );

        m_comboCopyMode->setEnabled( false );
        m_comboCopyMode->setCurrentIndex( 0 );

        // FIXME: at some point the media combo should also handle media sizes!

        if ( K3b::Device::isDvdMedia( sourceMedium.diskInfo().mediaType() ) ) {
            m_writerSelectionWidget->setWantedMediumType( sourceMedium.diskInfo().numLayers() > 1 &&
                                                          sourceMedium.diskInfo().size() > MediaSizeDvd4Gb
                                                          ? K3b::Device::MEDIA_WRITABLE_DVD_DL
                                                          : K3b::Device::MEDIA_WRITABLE_DVD );
        }
        else if( Device::isBdMedia( sourceMedium.diskInfo().mediaType() ) ) {
            // FIXME: do the same single layer/dual layer thing like with DVD
            m_writerSelectionWidget->setWantedMediumType( K3b::Device::MEDIA_WRITABLE_BD );
        }
        else {
            // generic media request in case we have no source medium
            m_writerSelectionWidget->setWantedMediumType( K3b::Device::MEDIA_WRITABLE );
        }
    }

    // CD Cloning
    if( m_comboCopyMode->currentIndex() == 1 ) {
        // cdrecord does not support cloning on-the-fly
        m_checkCacheImage->setChecked(true);
        m_checkCacheImage->setEnabled(false);

        m_writingModeWidget->setSupportedModes( K3b::WritingModeRaw );
    }

    // Normal CD/DVD/Blu-ray copy
    else {
        //
        // If the same device is used for reading and writing all we can present is a fuzzy
        // selection of the writing mode
        //
        if( burnDev == readDev ) {
            K3b::WritingModes modes = 0;
            if ( sourceMedium.diskInfo().mediaType() & K3b::Device::MEDIA_CD_ALL ) {
                modes = K3b::WritingModeTao|K3b::WritingModeSao|K3b::WritingModeRaw;
            }
            else if ( sourceMedium.diskInfo().mediaType() & K3b::Device::MEDIA_DVD_ALL ) {
                // only auto for DVD+R(W)
                if (burnDev) {
                    if( burnDev->writeCapabilities() & (K3b::Device::MEDIA_DVD_R|K3b::Device::MEDIA_DVD_RW) ) {
                        modes |= K3b::WritingModeSao|K3b::WritingModeRestrictedOverwrite;
                        if( burnDev->featureCurrent( K3b::Device::FEATURE_INCREMENTAL_STREAMING_WRITABLE ) != 0 )
                            modes |= K3b::WritingModeIncrementalSequential;
                    }

                    // TODO: once we have layer jump support: this is where it goes
                    //if ( burnDev->supportsWritingMode( K3b::Device::WRITING_MODE_LAYER_JUMP ) )
                    //     modes |= K3b::Device::WRITING_MODE_LAYER_JUMP;
                }
            }
            else if ( sourceMedium.diskInfo().mediaType() & K3b::Device::MEDIA_BD_ALL ) {
                // no modes, only auto
            }

            m_writingModeWidget->setSupportedModes( modes );
        }
        else {
            m_writingModeWidget->determineSupportedModesFromMedium( burnDev );
        }
    }

    m_tempDirSelectionWidget->setNeededSize( neededSize() );

    if( sourceMedium.toc().contentType() == K3b::Device::DATA &&
        sourceMedium.toc().count() == 1 ) {
        m_tempDirSelectionWidget->setSelectionMode( K3b::TempDirSelectionWidget::FILE );
        QString mediumName = sourceMedium.volumeId().toLower();
        if ( mediumName.isEmpty() )
            mediumName = "k3bimage";
        m_tempDirSelectionWidget->setDefaultImageFileName( mediumName + QLatin1String(".iso"), true );
    }
    else {
        m_tempDirSelectionWidget->setSelectionMode( K3b::TempDirSelectionWidget::DIR );

        if ( sourceMedium.content() & K3b::Medium::ContentData && !sourceMedium.volumeId().isEmpty() ) {
            m_tempDirSelectionWidget->setTempPath( m_tempDirSelectionWidget->tempDirectory() + sourceMedium.volumeId().toLower() );
        }
        else if ( sourceMedium.content() & K3b::Medium::ContentAudio && !sourceMedium.cdText().title().isEmpty() ) {
            m_tempDirSelectionWidget->setTempPath( m_tempDirSelectionWidget->tempDirectory() + sourceMedium.cdText().title() );
        }
        else {
            m_tempDirSelectionWidget->setTempPath( m_tempDirSelectionWidget->tempDirectory() ); // let the copy job figure it out
        }
    }

    m_groupAdvancedAudioOptions->setEnabled( sourceMedium.content() & K3b::Medium::ContentAudio && m_comboCopyMode->currentIndex() == 0 );
    m_groupAdvancedDataOptions->setEnabled( sourceMedium.content() & K3b::Medium::ContentData );

    setButtonEnabled( START_BUTTON,
                      m_comboSourceDevice->selectedDevice() &&
                      (burnDev || m_checkOnlyCreateImage->isChecked()) );

    K3b::InteractionDialog::toggleAll();
}


void K3b::MediaCopyDialog::updateOverrideDevice()
{
    if( !m_checkCacheImage->isChecked() ) {
        m_writerSelectionWidget->setOverrideDevice( 0 );
        m_writerSelectionWidget->setIgnoreDevice( m_comboSourceDevice->selectedDevice() );
    }
    else {
        m_writerSelectionWidget->setOverrideDevice( m_comboSourceDevice->selectedDevice(),
                                                    i18n("Use the same device for burning"),
                                                    i18n("<qt>Use the same device for burning <i>(Or insert another medium)</i>") );
        m_writerSelectionWidget->setIgnoreDevice( 0 );
    }
}


void K3b::MediaCopyDialog::loadSettings( const KConfigGroup& c )
{
    m_writerSelectionWidget->loadConfig( c );
    m_comboSourceDevice->setSelectedDevice( k3bcore->deviceManager()->findDevice( c.readEntry( "source_device" ) ) );
    m_writingModeWidget->loadConfig( c );
    m_checkSimulate->setChecked( c.readEntry( "simulate", false ) );
    m_checkCacheImage->setChecked( !c.readEntry( "on_the_fly", false ) );
    m_checkDeleteImages->setChecked( c.readEntry( "delete_images", true ) );
    m_checkOnlyCreateImage->setChecked( c.readEntry( "only_create_image", false ) );
    m_comboParanoiaMode->setCurrentIndex( c.readEntry( "paranoia_mode", 0 ) );
    m_checkVerifyData->setChecked( c.readEntry( "verify data", false ) );

    m_spinCopies->setValue( c.readEntry( "copies", 1 ) );

    m_tempDirSelectionWidget->readConfig( c );

    if( c.readEntry( "copy mode", "normal" ) == "normal" )
        m_comboCopyMode->setCurrentIndex( 0 );
    else
        m_comboCopyMode->setCurrentIndex( 1 );

    m_checkReadCdText->setChecked( c.readEntry( "copy cdtext", true ) );
    m_checkIgnoreDataReadErrors->setChecked( c.readEntry( "ignore data read errors", false ) );
    m_checkIgnoreAudioReadErrors->setChecked( c.readEntry( "ignore audio read errors", true ) );
    m_checkNoCorrection->setChecked( c.readEntry( "no correction", false ) );

    m_spinDataRetries->setValue( c.readEntry( "data retries", 128 ) );
    m_spinAudioRetries->setValue( c.readEntry( "audio retries", 5 ) );

    slotToggleAll();
}


void K3b::MediaCopyDialog::saveSettings( KConfigGroup c )
{
    m_writingModeWidget->saveConfig( c );
    c.writeEntry( "simulate", m_checkSimulate->isChecked() );
    c.writeEntry( "on_the_fly", !m_checkCacheImage->isChecked() );
    c.writeEntry( "delete_images", m_checkDeleteImages->isChecked() );
    c.writeEntry( "only_create_image", m_checkOnlyCreateImage->isChecked() );
    c.writeEntry( "paranoia_mode", m_comboParanoiaMode->currentText().toInt() );
    c.writeEntry( "copies", m_spinCopies->value() );
    c.writeEntry( "verify data", m_checkVerifyData->isChecked() );

    m_writerSelectionWidget->saveConfig( c );
    m_tempDirSelectionWidget->saveConfig( c );

    c.writeEntry( "source_device", m_comboSourceDevice->selectedDevice() ? m_comboSourceDevice->selectedDevice()->blockDeviceName() : QString() );

    c.writeEntry( "copy cdtext", m_checkReadCdText->isChecked() );
    c.writeEntry( "ignore data read errors", m_checkIgnoreDataReadErrors->isChecked() );
    c.writeEntry( "ignore audio read errors", m_checkIgnoreAudioReadErrors->isChecked() );
    c.writeEntry( "no correction", m_checkNoCorrection->isChecked() );
    c.writeEntry( "data retries", m_spinDataRetries->value() );
    c.writeEntry( "audio retries", m_spinAudioRetries->value() );

    QString s;
    if( m_comboCopyMode->currentIndex() == 1 )
        s = "clone";
    else
        s = "normal";
    c.writeEntry( "copy mode", s );
}


KIO::filesize_t K3b::MediaCopyDialog::neededSize() const
{
    K3b::Medium medium = k3bappcore->mediaCache()->medium( m_comboSourceDevice->selectedDevice() );

    if( medium.diskInfo().diskState() == K3b::Device::STATE_NO_MEDIA )
        return 0;
    else if( medium.diskInfo().mediaType() & (K3b::Device::MEDIA_DVD_RW_OVWR|K3b::Device::MEDIA_DVD_PLUS_RW) )
        return (KIO::filesize_t)medium.iso9660Descriptor().volumeSpaceSize * (KIO::filesize_t)2048;
    else if ( m_comboCopyMode->currentIndex() == 0 )
        return medium.diskInfo().size().mode1Bytes();
    else
        return medium.diskInfo().size().rawBytes();
}


