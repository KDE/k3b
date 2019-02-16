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

#include "k3badvancedoptiontab.h"
#include "k3bmsfedit.h"
#include "k3bcore.h"
#include "k3bstdguiitems.h"
#include "k3bglobalsettings.h"

#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KLineEdit>
#include <KLocalizedString>

#include <QValidator>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QRadioButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QToolTip>


K3b::AdvancedOptionTab::AdvancedOptionTab( QWidget* parent )
    : QWidget( parent )
{
    setupGui();
}


K3b::AdvancedOptionTab::~AdvancedOptionTab()
{
}


void K3b::AdvancedOptionTab::setupGui()
{
    QGridLayout* groupAdvancedLayout = new QGridLayout( this );
    groupAdvancedLayout->setAlignment( Qt::AlignTop );
    groupAdvancedLayout->setContentsMargins( 0, 0, 0, 0 );


    QGroupBox* groupWritingApp = new QGroupBox( i18n("Burning"), this );
    QGridLayout* bufferLayout = new QGridLayout( groupWritingApp );

    m_checkBurnfree = K3b::StdGuiItems::burnproofCheckbox( groupWritingApp );
    m_checkOverburn = new QCheckBox( i18n("Allow &overburning"), groupWritingApp );
    m_checkForceUnsafeOperations = new QCheckBox( i18n("&Force unsafe operations"), groupWritingApp );
    m_checkManualWritingBufferSize = new QCheckBox( i18n("&Manual writing buffer size") + ':', groupWritingApp );
    m_editWritingBufferSize = new QSpinBox( groupWritingApp );
    m_editWritingBufferSize->setRange( 1, 100 );
    m_editWritingBufferSize->setValue( 4 );
    m_editWritingBufferSize->setSuffix( ' ' + i18n("MB") );
    m_checkShowForceGuiElements = new QCheckBox( i18n("Show &advanced GUI elements"), groupWritingApp );
    bufferLayout->addWidget( m_checkBurnfree, 0, 0, 1, 3 );
    bufferLayout->addWidget( m_checkOverburn, 1, 0, 1, 2 );
    bufferLayout->addWidget( m_checkForceUnsafeOperations, 2, 0, 1, 3 );
    bufferLayout->addWidget( m_checkManualWritingBufferSize, 3, 0 );
    bufferLayout->addWidget( m_editWritingBufferSize, 3, 1 );
    bufferLayout->addWidget( m_checkShowForceGuiElements, 4, 0, 1, 3 );
    bufferLayout->setColumnStretch( 2, 1 );

    QGroupBox* groupMisc = new QGroupBox( i18n("Miscellaneous"), this );
    QVBoxLayout* groupMiscLayout = new QVBoxLayout( groupMisc );
    m_checkEject = new QCheckBox( i18n("Do not &eject medium after write process"), groupMisc );
    groupMiscLayout->addWidget( m_checkEject );
    m_checkAutoErasingRewritable = new QCheckBox( i18n("Automatically erase CD-RWs and DVD-RWs"), groupMisc );
    groupMiscLayout->addWidget( m_checkAutoErasingRewritable );

    groupAdvancedLayout->addWidget( groupWritingApp, 0, 0 );
    groupAdvancedLayout->addWidget( groupMisc, 1, 0 );
    groupAdvancedLayout->setRowStretch( 2, 1 );


    connect( m_checkManualWritingBufferSize, SIGNAL(toggled(bool)),
             m_editWritingBufferSize, SLOT(setEnabled(bool)) );
    connect( m_checkManualWritingBufferSize, SIGNAL(toggled(bool)),
             this, SLOT(slotSetDefaultBufferSizes(bool)) );


    m_editWritingBufferSize->setDisabled( true );
    // -----------------------------------------------------------------------


    m_checkOverburn->setToolTip( i18n("Allow burning more than the official media capacities") );
    m_checkShowForceGuiElements->setToolTip( i18n("Show advanced GUI elements like allowing to choose between cdrecord and cdrdao") );
    m_checkAutoErasingRewritable->setToolTip( i18n("Automatically erase CD-RWs and DVD-RWs without asking") );
    m_checkEject->setToolTip( i18n("Do not eject the burn medium after a completed burn process") );
    m_checkForceUnsafeOperations->setToolTip( i18n("Force K3b to continue some operations otherwise deemed as unsafe") );

    m_checkShowForceGuiElements->setWhatsThis( i18n("<p>If this option is checked additional GUI "
                                                    "elements which allow one to influence the behavior of K3b are shown. "
                                                    "This includes the manual selection of the used burning tool. "
                                                    "(Choose between cdrecord and cdrdao when writing a CD or between "
                                                    "cdrecord and growisofs when writing a DVD/BD.)"
                                                    "<p><b>Be aware that K3b does not support all possible tools "
                                                    "in all project types and actions.</b>") );

    m_checkOverburn->setWhatsThis( i18n("<p>Each medium has an official maximum capacity which is stored in a read-only "
                                        "area of the medium and is guaranteed by the vendor. However, this official "
                                        "maximum is not always the actual maximum. Many media have an "
                                        "actual total capacity that is slightly larger than the official amount."
                                        "<p>If this option is checked K3b will disable a safety check that prevents "
                                        "burning beyond the official capacity."
                                        "<p><b>Caution:</b> Enabling this option can cause failures in the end of the "
                                        "burning process if K3b attempts to write beyond the official capacity. It "
                                        "makes sense to first determine the actual maximum capacity of the media brand "
                                        "with a simulated burn.") );

    m_checkAutoErasingRewritable->setWhatsThis( i18n("<p>If this option is checked K3b will automatically "
                                                     "erase CD-RWs and format DVD-RWs if one is found instead "
                                                     "of an empty media before writing.") );

    m_checkManualWritingBufferSize->setWhatsThis( i18n("<p>K3b uses a software buffer during the burning process to "
                                                       "avoid gaps in the data stream due to high system load. The default "
                                                       "sizes used are %1 MB for CD and %2 MB for DVD burning."
                                                       "<p>If this option is checked the value specified will be used for both "
                                                       "CD and DVD burning.", 4, 32) );

    m_checkEject->setWhatsThis( i18n("<p>If this option is checked K3b will not eject the medium once the burn process "
                                     "finishes. This can be helpful in case one leaves the computer after starting the "
                                     "burning and does not want the tray to be open all the time."
                                     "<p>However, on Linux systems a freshly burned medium has to be reloaded. Otherwise "
                                     "the system will not detect the changes and still treat it as an empty medium.") );

    m_checkForceUnsafeOperations->setWhatsThis( i18n("<p>If this option is checked K3b will continue in some situations "
                                                     "which would otherwise be deemed as unsafe."
                                                     "<p>This setting for example disables the check for medium speed "
                                                     "verification. Thus, one can force K3b to burn a high speed medium on "
                                                     "a low speed writer."
                                                     "<p><b>Caution:</b> Enabling this option may result in damaged media.") );
}


void K3b::AdvancedOptionTab::readSettings()
{
    KConfigGroup c( KSharedConfig::openConfig(), "General Options" );

    m_checkAutoErasingRewritable->setChecked( c.readEntry( "auto rewritable erasing", false ) );
    m_checkShowForceGuiElements->setChecked( c.readEntry( "Show advanced GUI", false ) );

    m_checkBurnfree->setChecked( k3bcore->globalSettings()->burnfree() );
    m_checkEject->setChecked( !k3bcore->globalSettings()->ejectMedia() );
    m_checkOverburn->setChecked( k3bcore->globalSettings()->overburn() );
    m_checkForceUnsafeOperations->setChecked( k3bcore->globalSettings()->force() );
    m_checkManualWritingBufferSize->setChecked( k3bcore->globalSettings()->useManualBufferSize() );
    if( k3bcore->globalSettings()->useManualBufferSize() )
        m_editWritingBufferSize->setValue( k3bcore->globalSettings()->bufferSize() );
}


void K3b::AdvancedOptionTab::saveSettings()
{
    KConfigGroup c( KSharedConfig::openConfig(), "General Options" );

    c.writeEntry( "auto rewritable erasing", m_checkAutoErasingRewritable->isChecked() );
    c.writeEntry( "Show advanced GUI", m_checkShowForceGuiElements->isChecked() );

    k3bcore->globalSettings()->setEjectMedia( !m_checkEject->isChecked() );
    k3bcore->globalSettings()->setOverburn( m_checkOverburn->isChecked() );
    k3bcore->globalSettings()->setBurnfree( m_checkBurnfree->isChecked() );
    k3bcore->globalSettings()->setUseManualBufferSize( m_checkManualWritingBufferSize->isChecked() );
    k3bcore->globalSettings()->setBufferSize( m_editWritingBufferSize->value() );
    k3bcore->globalSettings()->setForce( m_checkForceUnsafeOperations->isChecked() );
}


void K3b::AdvancedOptionTab::slotSetDefaultBufferSizes( bool b )
{
    if( !b ) {
        m_editWritingBufferSize->setValue( 4 );
    }
}



