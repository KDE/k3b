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


#include "k3bprojectburndialog.h"
#include "k3bdoc.h"
#include "k3bburnprogressdialog.h"
#include "k3bjob.h"
#include "k3btempdirselectionwidget.h"
#include "k3bwriterselectionwidget.h"
#include "k3bstdguiitems.h"
#include "k3bwritingmodewidget.h"
#include "k3bapplication.h"
#include "k3bmediacache.h"
#include "k3bmedium.h"

#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bglobals.h>
#include <k3bcore.h>

#include <qstring.h>
#include <qpushbutton.h>
#include <qtooltip.h>

#include <qlayout.h>
#include <QWhatsThis>
#include <qcheckbox.h>
#include <qtabwidget.h>
#include <qgroupbox.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <QGridLayout>
#include <QVBoxLayout>
#include <klocale.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kguiitem.h>
#include <kstdguiitem.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <kvbox.h>


K3bProjectBurnDialog::K3bProjectBurnDialog( K3bDoc* doc, QWidget *parent )
    : K3bInteractionDialog( parent,
                            i18n("Project"),
                            QString(),
                            START_BUTTON|SAVE_BUTTON|CANCEL_BUTTON,
                            START_BUTTON,
                            "default " + doc->typeString() + " settings" ),
      m_writerSelectionWidget(0),
      m_tempDirSelectionWidget(0)
{
    m_doc = doc;

    /**
     * There is at least one scenario in which this is useful: change the volume id (or rename a file)
     * without explicit confirmation (by pressing enter for example). Then click the "burn" button.
     * The "focus out" event which results in a rename in the listviewitem will be processed after the
     * initialization of the burn dialog. Thus, the burn dialog will read the old volume id.
     */
    setDelayedInitialization( true );

    setButtonGui( SAVE_BUTTON,
                  KStandardGuiItem::close() );
    setButtonText( SAVE_BUTTON,
                   i18n("Close"),
                   i18n("Save Settings and close"),
                   i18n("Saves the settings to the project and closes the dialog.") );
    setButtonGui( CANCEL_BUTTON, KStandardGuiItem::cancel() );
    setButtonText( CANCEL_BUTTON,
                   i18n("Cancel"),
                   i18n("Discard all changes and close"),
                   i18n("Discards all changes made in the dialog and closes it.") );

    m_job = 0;
}


K3bProjectBurnDialog::~K3bProjectBurnDialog(){
}


void K3bProjectBurnDialog::init()
{
    readSettings();
//   if( !m_writerSelectionWidget->writerDevice() )
//     m_checkOnlyCreateImage->setChecked(true);
}


void K3bProjectBurnDialog::slotWriterChanged()
{
    slotToggleAll();
}


void K3bProjectBurnDialog::slotWritingAppChanged( K3b::WritingApp )
{
    slotToggleAll();
}


void K3bProjectBurnDialog::toggleAll()
{
    K3bDevice::Device* dev = m_writerSelectionWidget->writerDevice();
    if( dev ) {
        K3bMedium burnMedium = k3bappcore->mediaCache()->medium( dev );

        if( burnMedium.diskInfo().mediaType() & K3bDevice::MEDIA_DVD_PLUS_ALL ) {
            // no simulation support for DVD+R(W)
            m_checkSimulate->setChecked(false);
            m_checkSimulate->setEnabled(false);
        }
        else {
            m_checkSimulate->setEnabled(true);
        }

        setButtonEnabled( START_BUTTON, true );
    }
    else
        setButtonEnabled( START_BUTTON, false );

    m_writingModeWidget->determineSupportedModesFromMedium( dev );

    m_writingModeWidget->setDisabled( m_checkOnlyCreateImage->isChecked() );
    m_checkSimulate->setDisabled( m_checkOnlyCreateImage->isChecked() );
    m_checkCacheImage->setDisabled( m_checkOnlyCreateImage->isChecked() );
    m_checkRemoveBufferFiles->setDisabled( m_checkOnlyCreateImage->isChecked() || !m_checkCacheImage->isChecked() );
    if( m_checkOnlyCreateImage->isChecked() ) {
        m_checkRemoveBufferFiles->setChecked(false);
        setButtonEnabled( START_BUTTON, true );
    }
    m_tempDirSelectionWidget->setDisabled( !m_checkCacheImage->isChecked() && !m_checkOnlyCreateImage->isChecked() );
    m_writerSelectionWidget->setDisabled( m_checkOnlyCreateImage->isChecked() );
    m_spinCopies->setDisabled( m_checkSimulate->isChecked() || m_checkOnlyCreateImage->isChecked() );

    // we only support DAO with cdrdao
    if( m_writerSelectionWidget->writingApp() == K3b::WRITING_APP_CDRDAO )
        m_writingModeWidget->setSupportedModes( K3b::WRITING_MODE_DAO );

    if( m_checkOnlyCreateImage->isChecked() )
        setButtonText( START_BUTTON,
                       i18n("Start"),
                       i18n("Start the image creation") );
    else
        setButtonText( START_BUTTON, i18n("Burn"),
                       i18n("Start the burning process") );
}


int K3bProjectBurnDialog::execBurnDialog( bool burn )
{
    if( burn && m_job == 0 ) {
        setButtonShown( START_BUTTON, true );
        setDefaultButton( START_BUTTON );
    }
    else {
        setButtonShown( START_BUTTON, false );
        setDefaultButton( SAVE_BUTTON );
    }

    return K3bInteractionDialog::exec();
}


void K3bProjectBurnDialog::slotSaveClicked()
{
    saveSettings();
    done( Saved );
}


void K3bProjectBurnDialog::slotCancelClicked()
{
    done( Canceled );
}


void K3bProjectBurnDialog::slotStartClicked()
{
    saveSettings();

    if( m_tempDirSelectionWidget ) {
        if( !doc()->onTheFly() || doc()->onlyCreateImages() ) {
            //
            // check if the temp dir exists
            //
            QString tempDir = m_tempDirSelectionWidget->tempDirectory();
            if( !QFile::exists( tempDir ) ) {
                if( KMessageBox::warningYesNo( this, i18n("Image folder '%1' does not exist. Do you want K3b to create it?", tempDir ) )
                    == KMessageBox::Yes ) {
                    if( !KStandardDirs::makeDir( tempDir ) ) {
                        KMessageBox::error( this, i18n("Failed to create folder '%1'.", tempDir ) );
                        return;
                    }
                }
                else
                    return;
            }

            //
            // check if enough space in tempdir if not on-the-fly
            //
            if( doc()->burningSize()/1024 > m_tempDirSelectionWidget->freeTempSpace() ) {
                if( KMessageBox::warningContinueCancel( this, i18n("There seems to be not enough free space in temporary directory. "
                                                                   "Write anyway?") ) == KMessageBox::Cancel )
                    return;
            }
        }
    }

    K3bJobProgressDialog* dlg = 0;
    if( m_checkOnlyCreateImage && m_checkOnlyCreateImage->isChecked() )
        dlg = new K3bJobProgressDialog( parentWidget() );
    else
        dlg = new K3bBurnProgressDialog( parentWidget() );

    m_job = m_doc->newBurnJob( dlg );

    if( m_writerSelectionWidget )
        m_job->setWritingApp( m_writerSelectionWidget->writingApp() );
    prepareJob( m_job );

    hideTemporarily();

    dlg->startJob(m_job);

    kDebug() << "(K3bProjectBurnDialog) job done. cleaning up.";

    delete m_job;
    m_job = 0;
    delete dlg;

    done( Burn );
}


void K3bProjectBurnDialog::prepareGui()
{
    QVBoxLayout* mainLay = new QVBoxLayout( mainWidget() );
    mainLay->setMargin( 0 );
    mainLay->setSpacing( KDialog::spacingHint() );

    m_writerSelectionWidget = new K3bWriterSelectionWidget( mainWidget() );
    m_writerSelectionWidget->setWantedMediumType( m_doc->supportedMediaTypes() );
    m_writerSelectionWidget->setWantedMediumState( K3bDevice::STATE_EMPTY );
    m_writerSelectionWidget->setWantedMediumSize( m_doc->length() );
    mainLay->addWidget( m_writerSelectionWidget );

    m_tabWidget = new QTabWidget( mainWidget() );
    mainLay->addWidget( m_tabWidget );

    QWidget* w = new QWidget( m_tabWidget );
    m_tabWidget->addTab( w, i18n("Writing") );

    QGroupBox* groupWritingMode = new QGroupBox( i18n("Writing Mode"), w );
    m_writingModeWidget = new K3bWritingModeWidget( groupWritingMode );
    QVBoxLayout* groupWritingModeLayout = new QVBoxLayout( groupWritingMode );
    groupWritingModeLayout->setMargin( marginHint() );
    groupWritingModeLayout->setSpacing( spacingHint() );
    groupWritingModeLayout->addWidget( m_writingModeWidget );

    m_optionGroup = new QGroupBox( i18n("Settings"), w );
    m_optionGroupLayout = new QVBoxLayout( m_optionGroup );
    m_optionGroupLayout->setMargin( marginHint() );
    m_optionGroupLayout->setSpacing( KDialog::spacingHint() );

    // add the options
    m_checkCacheImage = K3bStdGuiItems::createCacheImageCheckbox( m_optionGroup );
    m_checkSimulate = K3bStdGuiItems::simulateCheckbox( m_optionGroup );
    m_checkRemoveBufferFiles = K3bStdGuiItems::removeImagesCheckbox( m_optionGroup );
    m_checkOnlyCreateImage = K3bStdGuiItems::onlyCreateImagesCheckbox( m_optionGroup );

    m_optionGroupLayout->addWidget(m_checkSimulate);
    m_optionGroupLayout->addWidget(m_checkCacheImage);
    m_optionGroupLayout->addWidget(m_checkOnlyCreateImage);
    m_optionGroupLayout->addWidget(m_checkRemoveBufferFiles);

    QGroupBox* groupCopies = new QGroupBox( i18n("Copies"), w );
    QLabel* pixLabel = new QLabel( groupCopies );
    pixLabel->setPixmap( SmallIcon( "tools-media-optical-copy", KIconLoader::SizeMedium ) );
    pixLabel->setScaledContents( false );
    m_spinCopies = new QSpinBox( groupCopies );
    m_spinCopies->setRange( 1, 999 );
    QHBoxLayout* groupCopiesLayout = new QHBoxLayout( groupCopies );
    groupCopiesLayout->setSpacing( spacingHint() );
    groupCopiesLayout->setMargin( marginHint() );
    groupCopiesLayout->addWidget( pixLabel );
    groupCopiesLayout->addWidget( m_spinCopies );

    // arrange it
    QGridLayout* grid = new QGridLayout( w );
    grid->setMargin( KDialog::marginHint() );
    grid->setSpacing( KDialog::spacingHint() );

    grid->addWidget( groupWritingMode, 0, 0 );
    grid->addWidget( m_optionGroup, 0, 1, 3, 1 );
    grid->addWidget( groupCopies, 2, 0 );
    //  grid->addWidget( m_tempDirSelectionWidget, 1, 1, 3, 1 );
    grid->setRowStretch( 1, 1 );
    grid->setColumnStretch( 1, 1 );

    QWidget* tempW = new QWidget( m_tabWidget );
    grid = new QGridLayout( tempW );
    grid->setMargin( KDialog::marginHint() );
    grid->setSpacing( KDialog::spacingHint() );
    m_tabWidget->addTab( tempW, i18n("Image") );
    m_tempDirSelectionWidget = new K3bTempDirSelectionWidget( tempW );
    grid->addWidget( m_tempDirSelectionWidget, 0, 0 );
    m_tempDirSelectionWidget->setNeededSize( doc()->size() );

    // tab order
    setTabOrder( m_writerSelectionWidget, m_writingModeWidget );
    setTabOrder( m_writingModeWidget, groupCopies );
    setTabOrder( groupCopies, m_optionGroup );

    // some default connections that should always be useful
    connect( m_writerSelectionWidget, SIGNAL(writerChanged()), this, SLOT(slotWriterChanged()) );
    connect( m_writerSelectionWidget, SIGNAL(writerChanged(K3bDevice::Device*)),
             m_writingModeWidget, SLOT(determineSupportedModesFromMedium(K3bDevice::Device*)) );
    connect( m_writerSelectionWidget, SIGNAL(writingAppChanged(K3b::WritingApp)), this, SLOT(slotWritingAppChanged(K3b::WritingApp)) );
    connect( m_checkCacheImage, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );
    connect( m_checkSimulate, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );
    connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );
    connect( m_writingModeWidget, SIGNAL(writingModeChanged(K3b::WritingMode)), this, SLOT(slotToggleAll()) );

    connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), this, SLOT(slotShowImageTip(bool)) );
    connect( m_checkCacheImage, SIGNAL(toggled(bool)), this, SLOT(slotShowImageTip(bool)) );
}


void K3bProjectBurnDialog::addPage( QWidget* page, const QString& title )
{
    m_tabWidget->addTab( page, title );
}


void K3bProjectBurnDialog::saveSettings()
{
    m_doc->setDummy( m_checkSimulate->isChecked() );
    m_doc->setOnTheFly( !m_checkCacheImage->isChecked() );
    m_doc->setOnlyCreateImages( m_checkOnlyCreateImage->isChecked() );
    m_doc->setRemoveImages( m_checkRemoveBufferFiles->isChecked() );
    m_doc->setSpeed( m_writerSelectionWidget->writerSpeed() );
    m_doc->setBurner( m_writerSelectionWidget->writerDevice() );
    m_doc->setWritingMode( m_writingModeWidget->writingMode() );
    m_doc->setWritingApp( m_writerSelectionWidget->writingApp() );
    m_doc->setCopies( m_spinCopies->value() );
}


void K3bProjectBurnDialog::readSettings()
{
    m_checkSimulate->setChecked( doc()->dummy() );
    m_checkCacheImage->setChecked( !doc()->onTheFly() );
    m_checkOnlyCreateImage->setChecked( m_doc->onlyCreateImages() );
    m_checkRemoveBufferFiles->setChecked( m_doc->removeImages() );
    m_writingModeWidget->setWritingMode( doc()->writingMode() );
    m_writerSelectionWidget->setWriterDevice( doc()->burner() );
    m_writerSelectionWidget->setSpeed( doc()->speed() );
    m_writerSelectionWidget->setWritingApp( doc()->writingApp() );
    m_writerSelectionWidget->setWantedMediumType( doc()->supportedMediaTypes() );
    m_spinCopies->setValue( m_doc->copies() );
}


void K3bProjectBurnDialog::saveUserDefaults( KConfigGroup& c )
{
    m_writingModeWidget->saveConfig( c );
    c.writeEntry( "simulate", m_checkSimulate->isChecked() );
    c.writeEntry( "on_the_fly", !m_checkCacheImage->isChecked() );
    c.writeEntry( "remove_image", m_checkRemoveBufferFiles->isChecked() );
    c.writeEntry( "only_create_image", m_checkOnlyCreateImage->isChecked() );
    c.writeEntry( "copies", m_spinCopies->value() );

    m_tempDirSelectionWidget->saveConfig( c );
    m_writerSelectionWidget->saveConfig( c );
}


void K3bProjectBurnDialog::loadUserDefaults( const KConfigGroup& c )
{
    m_writingModeWidget->loadConfig( c );
    m_checkSimulate->setChecked( c.readEntry( "simulate", false ) );
    m_checkCacheImage->setChecked( !c.readEntry( "on_the_fly", true ) );
    m_checkRemoveBufferFiles->setChecked( c.readEntry( "remove_image", true ) );
    m_checkOnlyCreateImage->setChecked( c.readEntry( "only_create_image", false ) );
    m_spinCopies->setValue( c.readEntry( "copies", 1 ) );

    m_tempDirSelectionWidget->readConfig( c );
    m_writerSelectionWidget->loadConfig( c );
}


void K3bProjectBurnDialog::loadK3bDefaults()
{
    m_writerSelectionWidget->loadDefaults();
    m_writingModeWidget->setWritingMode( K3b::WRITING_MODE_AUTO );
    m_checkSimulate->setChecked( false );
    m_checkCacheImage->setChecked( false );
    m_checkRemoveBufferFiles->setChecked( true );
    m_checkOnlyCreateImage->setChecked( false );
    m_spinCopies->setValue( 1 );

    if( m_tempDirSelectionWidget->selectionMode() == K3bTempDirSelectionWidget::DIR )
        m_tempDirSelectionWidget->setTempPath( K3b::defaultTempPath() );
    else
        m_tempDirSelectionWidget->setTempPath( K3b::defaultTempPath() + doc()->name() + ".iso" );
}


void K3bProjectBurnDialog::slotShowImageTip( bool buttonActivated )
{
    if ( buttonActivated ) {
        // FIXME: use the tab bar's position
        QWhatsThis::showText( mapToGlobal( QPoint( rect().center().x(), rect().top() ) ),i18n( "Use the 'Image' tab to optionally adjust the path of the image." ));
    }
}

#include "k3bprojectburndialog.moc"
