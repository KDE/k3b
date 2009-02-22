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


#include "k3bdataburndialog.h"
#include "k3bdataimagesettingswidget.h"
#include "k3bdatavolumedescwidget.h"
#include "k3bdatamultisessioncombobox.h"
#include "k3bdataview.h"
#include "../k3bapplication.h"

#include <k3bisooptions.h>
#include <k3bdatadoc.h>
#include <k3bdevice.h>
#include <k3bwriterselectionwidget.h>
#include <k3btempdirselectionwidget.h>
#include <k3bjob.h>
#include <k3bcore.h>
#include <k3bstdguiitems.h>
#include <k3bdatamodewidget.h>
#include <k3bglobals.h>
#include <k3bwritingmodewidget.h>
#include <k3bmediacache.h>

#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qpoint.h>
#include <qradiobutton.h>
#include <qfileinfo.h>
#include <qtabwidget.h>
#include <qspinbox.h>
#include <qfile.h>
#include <QGridLayout>

#include <kmessagebox.h>
#include <klineedit.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kfiledialog.h>
#include <kcombobox.h>
#include <kio/global.h>


#include "k3bfilecompilationsizehandler.h"


K3b::DataBurnDialog::DataBurnDialog(K3b::DataDoc* _doc, QWidget *parent )
    : K3b::ProjectBurnDialog( _doc, parent )
{
    prepareGui();

    setTitle( i18n("Data Project"), i18n("Size: %1", KIO::convertSize(_doc->size()) ) );

    // for now we just put the verify checkbox on the main page...
    m_checkVerify = K3b::StdGuiItems::verifyCheckBox( m_optionGroup );
    m_optionGroupLayout->addWidget( m_checkVerify );

    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    m_optionGroupLayout->addItem( spacer );

    // create image settings tab
    m_imageSettingsWidget = new K3b::DataImageSettingsWidget( this );
    addPage( m_imageSettingsWidget, i18n("Filesystem") );

    setupSettingsTab();

    connect( m_comboMultisession, SIGNAL(activated(int)),
             this, SLOT(slotMultiSessionModeChanged()) );

    m_writerSelectionWidget->setWantedMediumState( K3b::Device::STATE_EMPTY|K3b::Device::STATE_INCOMPLETE );

    m_tempDirSelectionWidget->setSelectionMode( K3b::TempDirSelectionWidget::FILE );
    QString path = _doc->tempDir();
    if( !path.isEmpty() ) {
        m_tempDirSelectionWidget->setTempPath( path );
    }
    if( !_doc->isoOptions().volumeID().isEmpty() ) {
        m_tempDirSelectionWidget->setDefaultImageFileName( _doc->isoOptions().volumeID() + ".iso" );
    }

    connect( m_imageSettingsWidget->m_editVolumeName, SIGNAL(textChanged(const QString&)),
             m_tempDirSelectionWidget, SLOT(setDefaultImageFileName(const QString&)) );
}

K3b::DataBurnDialog::~DataBurnDialog(){
}


void K3b::DataBurnDialog::saveSettings()
{
    K3b::ProjectBurnDialog::saveSettings();

    // save iso image settings
    K3b::IsoOptions o = ((K3b::DataDoc*)doc())->isoOptions();
    m_imageSettingsWidget->save( o );
    ((K3b::DataDoc*)doc())->setIsoOptions( o );

    // save image file path
    ((K3b::DataDoc*)doc())->setTempDir( m_tempDirSelectionWidget->tempPath() );

    // save multisession settings
    ((K3b::DataDoc*)doc())->setMultiSessionMode( m_comboMultisession->multiSessionMode() );

    ((K3b::DataDoc*)doc())->setDataMode( m_dataModeWidget->dataMode() );

    ((K3b::DataDoc*)doc())->setVerifyData( m_checkVerify->isChecked() );
}


void K3b::DataBurnDialog::readSettings()
{
    K3b::ProjectBurnDialog::readSettings();

    // read multisession
    m_comboMultisession->setMultiSessionMode( ((K3b::DataDoc*)doc())->multiSessionMode() );

    if( !doc()->tempDir().isEmpty() )
        m_tempDirSelectionWidget->setTempPath( doc()->tempDir() );
    else
        m_tempDirSelectionWidget->setTempPath( K3b::defaultTempPath() + doc()->name() + ".iso" );

    m_checkVerify->setChecked( ((K3b::DataDoc*)doc())->verifyData() );

    m_imageSettingsWidget->load( ((K3b::DataDoc*)doc())->isoOptions() );

    m_dataModeWidget->setDataMode( ((K3b::DataDoc*)doc())->dataMode() );

    toggleAll();
    slotMultiSessionModeChanged();
}


void K3b::DataBurnDialog::setupSettingsTab()
{
    QWidget* frame = new QWidget( this );
    QGridLayout* frameLayout = new QGridLayout( frame );
    frameLayout->setSpacing( spacingHint() );
    frameLayout->setMargin( marginHint() );

    m_groupDataMode = new Q3GroupBox( 1, Qt::Vertical, i18n("Datatrack Mode"), frame );
    m_dataModeWidget = new K3b::DataModeWidget( m_groupDataMode );

    QGroupBox* groupMultiSession = new QGroupBox( i18n("Multisession Mode"), frame );
    m_comboMultisession = new K3b::DataMultiSessionCombobox( groupMultiSession );
    QVBoxLayout* groupMultiSessionLayout = new QVBoxLayout( groupMultiSession );
    groupMultiSessionLayout->setMargin( marginHint() );
    groupMultiSessionLayout->setSpacing( spacingHint() );
    groupMultiSessionLayout->addWidget( m_comboMultisession );

    frameLayout->addWidget( m_groupDataMode, 0, 0 );
    frameLayout->addWidget( groupMultiSession, 1, 0 );
    frameLayout->setRowStretch( 2, 1 );

    addPage( frame, i18n("Misc") );
}


void K3b::DataBurnDialog::slotStartClicked()
{
    if( m_checkOnlyCreateImage->isChecked() ||
        m_checkCacheImage->isChecked() ) {
        QFileInfo fi( m_tempDirSelectionWidget->tempPath() );
        if( fi.isDir() )
            m_tempDirSelectionWidget->setTempPath( fi.filePath() + "/image.iso" );

        if( QFile::exists( m_tempDirSelectionWidget->tempPath() ) ) {
            if( KMessageBox::warningContinueCancel( this,
                                                    i18n("Do you want to overwrite %1?",m_tempDirSelectionWidget->tempPath()),
                                                    i18n("File Exists"), KGuiItem(i18n("Overwrite")) )
                == KMessageBox::Continue ) {
                // delete the file here to avoid problems with free space in K3b::ProjectBurnDialog::slotStartClicked
                QFile::remove( m_tempDirSelectionWidget->tempPath() );
            }
            else
                return;
        }
    }

    if( m_writingModeWidget->writingMode() == K3b::WRITING_MODE_DAO &&
        m_comboMultisession->multiSessionMode() != K3b::DataDoc::NONE &&
        m_writerSelectionWidget->writingApp() == K3b::WRITING_APP_CDRECORD )
        if( KMessageBox::warningContinueCancel( this,
                                                i18n("Most writers do not support writing "
                                                     "multisession CDs in DAO mode.") )
            == KMessageBox::Cancel )
            return;


    K3b::ProjectBurnDialog::slotStartClicked();
}


void K3b::DataBurnDialog::loadK3bDefaults()
{
    K3b::ProjectBurnDialog::loadK3bDefaults();

    m_dataModeWidget->setDataMode( K3b::DATA_MODE_AUTO );

    m_imageSettingsWidget->load( K3b::IsoOptions::defaults() );
    m_comboMultisession->setMultiSessionMode( K3b::DataDoc::AUTO );
    m_checkVerify->setChecked( false );

    toggleAll();
}


void K3b::DataBurnDialog::loadUserDefaults( const KConfigGroup& c )
{
    K3b::ProjectBurnDialog::loadUserDefaults(c);

    m_dataModeWidget->loadConfig(c);
    m_comboMultisession->loadConfig( c );

    K3b::IsoOptions o = K3b::IsoOptions::load( c );
    m_imageSettingsWidget->load( o );

    m_checkVerify->setChecked( c.readEntry( "verify data", false ) );

    toggleAll();
}


void K3b::DataBurnDialog::saveUserDefaults( KConfigGroup c )
{
    K3b::ProjectBurnDialog::saveUserDefaults(c);

    m_dataModeWidget->saveConfig(c);
    m_comboMultisession->saveConfig( c );

    K3b::IsoOptions o;
    m_imageSettingsWidget->save( o );
    o.save( c );

    c.writeEntry( "verify data", m_checkVerify->isChecked() );
}


void K3b::DataBurnDialog::toggleAll()
{
    K3b::ProjectBurnDialog::toggleAll();

    if( m_checkSimulate->isChecked() || m_checkOnlyCreateImage->isChecked() ) {
        m_checkVerify->setChecked(false);
        m_checkVerify->setEnabled(false);
    }
    else
        m_checkVerify->setEnabled(true);

    m_comboMultisession->setDisabled( m_checkOnlyCreateImage->isChecked() );
    // we can only select the data mode for CD media
    // IDEA: why not give GUI elements like this a slot that reacts on media changes?
    m_dataModeWidget->setDisabled( m_checkOnlyCreateImage->isChecked() ||
                                   !K3b::Device::isCdMedia( k3bappcore->mediaCache()->diskInfo( m_writerSelectionWidget->writerDevice() ).mediaType() ) );

    // Multisession in DAO is not possible
    if( m_writingModeWidget->writingMode() == K3b::WRITING_MODE_DAO ) {
        if( m_comboMultisession->multiSessionMode() == K3b::DataDoc::START ||
            m_comboMultisession->multiSessionMode() == K3b::DataDoc::CONTINUE ||
            m_comboMultisession->multiSessionMode() == K3b::DataDoc::FINISH )
            KMessageBox::information( this, i18n("It is not possible to write multisession media in DAO mode. "
                                                 "Multisession has been disabled."),
                                      i18n("Multisession Problem"),
                                      "multisession_no_dao" );

        m_comboMultisession->setEnabled(false);
    }
    else {
        m_comboMultisession->setEnabled(true);
    }
}


void K3b::DataBurnDialog::slotMultiSessionModeChanged()
{
    if( m_comboMultisession->multiSessionMode() == K3b::DataDoc::CONTINUE ||
        m_comboMultisession->multiSessionMode() == K3b::DataDoc::FINISH )
        m_spinCopies->setEnabled(false);

    // wait for the proper medium
    // we have to do this in another slot than toggleAll to avoid an endless loop
    // FIXME: K3b::InteractionDialog::slotToggleAll is endless loop protected
    if( m_comboMultisession->multiSessionMode() == K3b::DataDoc::NONE )
        m_writerSelectionWidget->setWantedMediumState( K3b::Device::STATE_EMPTY );
    else if( m_comboMultisession->multiSessionMode() == K3b::DataDoc::CONTINUE ||
             m_comboMultisession->multiSessionMode() == K3b::DataDoc::FINISH )
        m_writerSelectionWidget->setWantedMediumState( K3b::Device::STATE_INCOMPLETE );
    else
        m_writerSelectionWidget->setWantedMediumState( K3b::Device::STATE_EMPTY|K3b::Device::STATE_INCOMPLETE );
}


#include "k3bdataburndialog.moc"
