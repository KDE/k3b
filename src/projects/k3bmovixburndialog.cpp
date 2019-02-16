/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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


#include "k3bmovixburndialog.h"
#include "k3bmovixdoc.h"
#include "k3bmovixprogram.h"
#include "k3bmovixoptionswidget.h"
#include "../k3bapplication.h"

#include "k3bdataimagesettingswidget.h"
#include "k3bexternalbinmanager.h"
#include "k3bwriterselectionwidget.h"
#include "k3btempdirselectionwidget.h"
#include "k3bstdguiitems.h"
#include "k3bglobals.h"
#include "k3bdatamodewidget.h"
#include "k3bisooptions.h"
#include "k3bwritingmodewidget.h"
#include "k3bcore.h"
#include "k3bmediacache.h"

#include <KConfig>
#include <KIO/Global>
#include <KLocalizedString>
#include <KMessageBox>

#include <QDebug>
#include <QFileInfo>
#include <QCheckBox>
#include <QLayout>
#include <QGroupBox>
#include <QGridLayout>


K3b::MovixBurnDialog::MovixBurnDialog( K3b::MovixDoc* doc, QWidget* parent )
    : K3b::ProjectBurnDialog( doc, parent ),
      m_doc(doc)
{
    prepareGui();

    m_tempDirSelectionWidget->setSelectionMode( K3b::TempDirSelectionWidget::FILE );

    setTitle( i18n("eMovix Project"),
              i18np("One file (%2)", "%1 files (%2)", m_doc->movixFileItems().count(),KIO::convertSize(m_doc->size())) );

    m_movixOptionsWidget = new K3b::MovixOptionsWidget( this );
    addPage( m_movixOptionsWidget, i18n("eMovix") );

    // create image settings tab
    m_imageSettingsWidget = new K3b::DataImageSettingsWidget( this );
    addPage( m_imageSettingsWidget, i18n("Filesystem") );

    setupSettingsPage();

    // for now we just put the verify checkbox on the main page...
    m_checkVerify = K3b::StdGuiItems::verifyCheckBox( m_optionGroup );
    m_optionGroupLayout->addWidget( m_checkVerify );

    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    m_optionGroupLayout->addItem( spacer );

    m_tempDirSelectionWidget->setSelectionMode( K3b::TempDirSelectionWidget::FILE );
    QString path = m_doc->tempDir();
    if( !path.isEmpty() ) {
        m_tempDirSelectionWidget->setTempPath( path );
    }
    if( !m_doc->isoOptions().volumeID().isEmpty() ) {
        m_tempDirSelectionWidget->setDefaultImageFileName( m_doc->isoOptions().volumeID() + ".iso" );
    }

    connect( m_imageSettingsWidget->m_editVolumeName, SIGNAL(textChanged(QString)),
             m_tempDirSelectionWidget, SLOT(setDefaultImageFileName(QString)) );
}


K3b::MovixBurnDialog::~MovixBurnDialog()
{
}


void K3b::MovixBurnDialog::setupSettingsPage()
{
    QWidget* frame = new QWidget( this );
    QGridLayout* frameLayout = new QGridLayout( frame );

    QGroupBox* groupDataMode = new QGroupBox( i18n("Datatrack Mode"), frame );
    m_dataModeWidget = new K3b::DataModeWidget( groupDataMode );
    QVBoxLayout* groupDataModeLayout = new QVBoxLayout( groupDataMode );
    groupDataModeLayout->addWidget( m_dataModeWidget );

    QGroupBox* groupMultisession = new QGroupBox( i18n("Multisession"), frame );
    m_checkStartMultiSesssion = K3b::StdGuiItems::startMultisessionCheckBox( groupMultisession );
    QVBoxLayout* groupMultisessionLayout = new QVBoxLayout( groupMultisession );
    groupMultisessionLayout->addWidget( m_checkStartMultiSesssion );

    frameLayout->addWidget( groupDataMode, 0, 0 );
    frameLayout->addWidget( groupMultisession, 1, 0 );
    frameLayout->setRowStretch( 2, 1 );

    addPage( frame, i18n("Misc") );
}


void K3b::MovixBurnDialog::loadSettings( const KConfigGroup& c )
{
    K3b::ProjectBurnDialog::loadSettings(c);

    m_checkStartMultiSesssion->setChecked( c.readEntry( "start_multisession", false ) );

    m_dataModeWidget->loadConfig(c);

    K3b::IsoOptions o = K3b::IsoOptions::load( c );
    m_imageSettingsWidget->load( o );

    m_movixOptionsWidget->loadConfig(c);

    m_checkVerify->setChecked( c.readEntry( "verify data", false ) );

    toggleAll();
}


void K3b::MovixBurnDialog::saveSettings( KConfigGroup c )
{
    K3b::ProjectBurnDialog::saveSettings(c);

    c.writeEntry( "start_multisession", m_checkStartMultiSesssion->isChecked() );

    m_dataModeWidget->saveConfig(c);

    K3b::IsoOptions o;
    m_imageSettingsWidget->save( o );
    o.save( c );

    c.writeEntry( "verify data", m_checkVerify->isChecked() );

    m_movixOptionsWidget->saveConfig(c);
}


void K3b::MovixBurnDialog::saveSettingsToProject()
{
    K3b::ProjectBurnDialog::saveSettingsToProject();

    m_movixOptionsWidget->saveSettings( m_doc );

    m_doc->setMultiSessionMode( m_checkStartMultiSesssion->isChecked() ? K3b::DataDoc::START : K3b::DataDoc::NONE );

    // save iso image settings
    K3b::IsoOptions o = m_doc->isoOptions();
    m_imageSettingsWidget->save( o );
    m_doc->setIsoOptions( o );

    m_doc->setDataMode( m_dataModeWidget->dataMode() );

    // save image file path
    m_doc->setTempDir( m_tempDirSelectionWidget->tempPath() );

    m_doc->setVerifyData( m_checkVerify->isChecked() );
}


void K3b::MovixBurnDialog::readSettingsFromProject()
{
    K3b::ProjectBurnDialog::readSettingsFromProject();

    m_checkStartMultiSesssion->setChecked( m_doc->multiSessionMode() == K3b::DataDoc::START );

    m_checkVerify->setChecked( m_doc->verifyData() );

    m_imageSettingsWidget->load( m_doc->isoOptions() );

    m_dataModeWidget->setDataMode( m_doc->dataMode() );

    if( !doc()->tempDir().isEmpty() )
        m_tempDirSelectionWidget->setTempPath( doc()->tempDir() );
    else
        m_tempDirSelectionWidget->setTempPath( K3b::defaultTempPath() + doc()->name() + ".iso" );

    // first of all we need a movix installation object
    const K3b::MovixBin* bin = dynamic_cast<const K3b::MovixBin*>( k3bcore->externalBinManager()->binObject("eMovix") );
    if( bin ) {
        m_movixOptionsWidget->init( bin );
        m_movixOptionsWidget->readSettings( m_doc );
    }
    else {
        KMessageBox::error( this, i18n("Could not find a valid eMovix installation.") );
        slotCancelClicked();
    }
}


void K3b::MovixBurnDialog::slotStartClicked()
{
    if( m_checkOnlyCreateImage->isChecked() ||
        m_checkCacheImage->isChecked() ) {
        QFileInfo fi( m_tempDirSelectionWidget->tempPath() );
        if( fi.isDir() )
            m_tempDirSelectionWidget->setTempPath( fi.filePath() + "/image.iso" );

        if( QFile::exists( m_tempDirSelectionWidget->tempPath() ) ) {
            if( KMessageBox::warningContinueCancel( this,
                                                    i18n("Do you want to overwrite %1?",m_tempDirSelectionWidget->tempPath()),
                                                    i18n("File Exists"), KStandardGuiItem::overwrite() )
                != KMessageBox::Continue )
                return;
        }
    }

    if( m_writingModeWidget->writingMode() == K3b::WritingModeSao &&
        m_checkStartMultiSesssion->isChecked() &&
        m_writerSelectionWidget->writingApp() == K3b::WritingAppCdrecord )
        if( KMessageBox::warningContinueCancel( this,
                                                i18n("Most writers do not support writing "
                                                     "multisession CDs in DAO mode.") )
            == KMessageBox::Cancel )
            return;


    K3b::ProjectBurnDialog::slotStartClicked();
}


void K3b::MovixBurnDialog::toggleAll()
{
    K3b::ProjectBurnDialog::toggleAll();

    if( m_checkSimulate->isChecked() || m_checkOnlyCreateImage->isChecked() ) {
        m_checkVerify->setChecked(false);
        m_checkVerify->setEnabled(false);
    }
    else
        m_checkVerify->setEnabled(true);

    // we can only select the data mode for CD media
    m_dataModeWidget->setDisabled( m_checkOnlyCreateImage->isChecked() ||
                                   !K3b::Device::isCdMedia( k3bappcore->mediaCache()->diskInfo( m_writerSelectionWidget->writerDevice() ).mediaType() ) );
    m_checkStartMultiSesssion->setDisabled( m_checkOnlyCreateImage->isChecked() );
}


