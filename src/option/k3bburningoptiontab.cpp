/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bburningoptiontab.h"
#include <k3bmsfedit.h>
#include <k3bcore.h>
#include <k3bstdguiitems.h>
#include <k3bglobalsettings.h>

#include <qlabel.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qtabwidget.h>
#include <qradiobutton.h>
#include <qvalidator.h>
#include <qbuttongroup.h>
#include <qspinbox.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include <knuminput.h>
#include <kconfig.h>
#include <kdialog.h>
#include <klocale.h>
#include <klineedit.h>


K3bBurningOptionTab::K3bBurningOptionTab( QWidget* parent, const char* name )
  : QWidget( parent, name )
{
  setupGui();
}


K3bBurningOptionTab::~K3bBurningOptionTab()
{
}


void K3bBurningOptionTab::setupGui()
{
  QVBoxLayout* box = new QVBoxLayout( this );
  box->setAutoAdd( true );

  QTabWidget* mainTabbed = new QTabWidget( this );


  // PROJECT TAB
  // //////////////////////////////////////////////////////////////////////
  QWidget* projectTab = new QWidget( mainTabbed );

  // general settings group
  // -----------------------------------------------------------------------
  QGroupBox* groupGeneral = new QGroupBox( 2, Qt::Vertical, i18n("General"), projectTab );
  groupGeneral->setInsideSpacing( KDialog::spacingHint() );
  groupGeneral->setInsideMargin( KDialog::marginHint() );

  m_checkBurnfree = K3bStdGuiItems::burnproofCheckbox( groupGeneral );
  // -----------------------------------------------------------------------

  // -----------------------------------------------------------------------
  // vcd settings group
  // -----------------------------------------------------------------------
  QGroupBox* groupVideo = new QGroupBox( projectTab, "groupVideo" );
  groupVideo->setTitle( i18n( "Video Project" ) );
  groupVideo->setColumnLayout(0, Qt::Vertical );
  groupVideo->layout()->setSpacing( 0 );
  groupVideo->layout()->setMargin( 0 );
  QGridLayout* groupVideoLayout = new QGridLayout( groupVideo->layout() );
  groupVideoLayout->setAlignment( Qt::AlignTop );
  groupVideoLayout->setSpacing( KDialog::spacingHint() );
  groupVideoLayout->setMargin( KDialog::marginHint() );

  m_checkUsePbc = new QCheckBox( i18n("Use playback control (PBC) by default"), groupVideo );
  m_labelPlayTime = new QLabel( i18n("Play each sequence/segment by default:"), groupVideo );
  m_spinPlayTime = new QSpinBox( groupVideo, "m_spinPlayTime" );
  m_spinPlayTime->setValue( 1 );
  m_spinPlayTime->setSuffix( i18n( " time(s)" ) );
  m_spinPlayTime->setSpecialValueText( i18n( "forever" ) );

  m_labelWaitTime = new QLabel( i18n("Time to wait after each sequence/segment by default:"), groupVideo );
  m_spinWaitTime = new QSpinBox( groupVideo, "m_spinWaitTime" );
  m_spinWaitTime->setMinValue( -1 );
  m_spinWaitTime->setValue( 2 );
  m_spinWaitTime->setSuffix( i18n( " second(s)" ) );
  m_spinWaitTime->setSpecialValueText( i18n( "infinite" ) );

  /* not implemented yet ********************************/
  m_checkUseNumKey = new QCheckBox( i18n("Use numeric keys by default"), groupVideo );
  /*************************************************/

  m_labelPlayTime->setDisabled( true );
  m_spinPlayTime->setDisabled( true );
  m_labelWaitTime->setDisabled( true );
  m_spinWaitTime->setDisabled( true );
  m_checkUseNumKey->setDisabled( true );

  groupVideoLayout->addMultiCellWidget( m_checkUsePbc, 0, 0, 0, 1 );
  groupVideoLayout->addMultiCellWidget( m_checkUseNumKey, 1, 1, 0, 1 );
  groupVideoLayout->addWidget( m_labelPlayTime, 2, 0 );
  groupVideoLayout->addWidget( m_spinPlayTime, 2, 1 );
  groupVideoLayout->addWidget( m_labelWaitTime, 3, 0 );
  groupVideoLayout->addWidget( m_spinWaitTime, 3, 1 );


  connect( m_checkUsePbc, SIGNAL(toggled(bool)), m_labelPlayTime, SLOT(setEnabled(bool)) );
  connect( m_checkUsePbc, SIGNAL(toggled(bool)), m_spinPlayTime, SLOT(setEnabled(bool)) );
  connect( m_checkUsePbc, SIGNAL(toggled(bool)), m_labelWaitTime, SLOT(setEnabled(bool)) );
  connect( m_checkUsePbc, SIGNAL(toggled(bool)), m_spinWaitTime, SLOT(setEnabled(bool)) );
  connect( m_checkUsePbc, SIGNAL(toggled(bool)), m_checkUseNumKey, SLOT(setEnabled(bool)) );


  QGridLayout* projectGrid = new QGridLayout( projectTab );
  projectGrid->setSpacing( KDialog::spacingHint() );
  projectGrid->setMargin( KDialog::marginHint() );

  projectGrid->addWidget( groupGeneral, 0, 0 );
  projectGrid->addWidget( groupVideo, 1, 0 );
  projectGrid->setRowStretch( 2, 1 );

  // ///////////////////////////////////////////////////////////////////////



  // advanced settings tab
  // -----------------------------------------------------------------------
  QWidget* advancedTab = new QWidget( mainTabbed );
  QGridLayout* groupAdvancedLayout = new QGridLayout( advancedTab );
  groupAdvancedLayout->setAlignment( Qt::AlignTop );
  groupAdvancedLayout->setSpacing( KDialog::spacingHint() );
  groupAdvancedLayout->setMargin( KDialog::marginHint() );



  QGroupBox* groupWritingApp = new QGroupBox( 0, Qt::Vertical, i18n("Writing Applications"), advancedTab );
  groupWritingApp->layout()->setMargin( 0 );
  QGridLayout* bufferLayout = new QGridLayout( groupWritingApp->layout() );
  bufferLayout->setMargin( KDialog::marginHint() );
  bufferLayout->setSpacing( KDialog::spacingHint() );

  m_checkOverburn = new QCheckBox( i18n("Allow &overburning (not supported by cdrecord <= 1.10)"), groupWritingApp );
  m_checkManualWritingBufferSize = new QCheckBox( i18n("&Manual writing buffer size") + ":", groupWritingApp );
  m_editWritingBufferSize = new KIntNumInput( 4, groupWritingApp );
  m_editWritingBufferSize->setSuffix( " " + i18n("MB") );
  m_checkAllowWritingAppSelection = new QCheckBox( i18n("Manual writing application &selection"), groupWritingApp );
  bufferLayout->addMultiCellWidget( m_checkOverburn, 0, 0, 0, 2 );
  bufferLayout->addWidget( m_checkManualWritingBufferSize, 1, 0 );
  bufferLayout->addWidget( m_editWritingBufferSize, 1, 1 );
  bufferLayout->addMultiCellWidget( m_checkAllowWritingAppSelection, 2, 2, 0, 2 );
  bufferLayout->setColStretch( 2, 1 );

  QGroupBox* groupMisc = new QGroupBox( 2, Qt::Vertical, i18n("Miscellaneous"), advancedTab );
  m_checkEject = new QCheckBox( i18n("Do not &eject medium after write process"), groupMisc );
  m_checkAutoErasingRewritable = new QCheckBox( i18n("Automatically erase CD-RWs and DVD-RWs"), groupMisc );

  groupAdvancedLayout->addWidget( groupWritingApp, 0, 0 );
  groupAdvancedLayout->addWidget( groupMisc, 1, 0 );
  groupAdvancedLayout->setRowStretch( 2, 1 );


  connect( m_checkManualWritingBufferSize, SIGNAL(toggled(bool)),
           m_editWritingBufferSize, SLOT(setEnabled(bool)) );
  connect( m_checkManualWritingBufferSize, SIGNAL(toggled(bool)),
           this, SLOT(slotSetDefaultBufferSizes(bool)) );


  m_editWritingBufferSize->setDisabled( true );
  // -----------------------------------------------------------------------



  // put all in the main tabbed
  // -----------------------------------------------------------------------
  mainTabbed->addTab( projectTab, i18n("&Writing") );
  mainTabbed->addTab( advancedTab, i18n("&Advanced") );

  QToolTip::add( m_checkAllowWritingAppSelection, i18n("Allow to choose between cdrecord and cdrdao") );

  QToolTip::add( m_checkAutoErasingRewritable, i18n("Automatically erase CD-RWs and DVD-RWs without asking") );

  QToolTip::add( m_checkUsePbc, i18n("Playback control, PBC, is available for Video CD 2.0 and Super Video CD 1.0 disc formats.") );
  QToolTip::add( m_checkUseNumKey, i18n("Use numeric keys to navigate chapters by default (In addition to 'Previous' and 'Next')") );
  QToolTip::add( m_labelWaitTime, i18n("Time to wait after each sequence/segment by default.") );
  QToolTip::add( m_labelPlayTime, i18n("Play each sequence/segment by default.") );

  QWhatsThis::add( m_checkAllowWritingAppSelection, i18n("<p>If this option is checked K3b gives "
                                                         "the possibility to choose between cdrecord "
                                                         "and cdrdao when writing a cd."
                                                         "<p>This may be useful if one of the programs "
                                                         "does not support the used writer."
                                                         "<p><b>Be aware that K3b does not support both "
                                                         "programs in all project types.</b>") );

  QWhatsThis::add( m_checkAutoErasingRewritable, i18n("<p>If this option is checked K3b will automatically "
                                                      "erase CD-RWs and format DVD-RWs if one is found instead "
                                                      "of an empty media before writing.") );

  QWhatsThis::add( m_checkUsePbc, i18n( "<p>Playback control, PBC, is available for Video CD 2.0 and Super Video CD 1.0 disc formats."
                             "<p>PBC allows control of the playback of play items and the possibility of interaction with the user through the remote control or some other input device available." ) );
}


void K3bBurningOptionTab::readSettings()
{
  KConfig* c = k3bcore->config();

  c->setGroup( "Video project settings" );
  m_checkUsePbc->setChecked( c->readBoolEntry("Use Playback Control", false) );
  m_spinWaitTime->setValue( c->readNumEntry( "Time to wait after each Sequence/Segment", 2 ) );
  m_spinPlayTime->setValue( c->readNumEntry( "Play each Sequence/Segment", 1 ) );
  m_checkUseNumKey->setChecked( c->readBoolEntry("Use numeric keys to navigate chapters", false) );

  c->setGroup( "General Options" );
  m_checkAutoErasingRewritable->setChecked( c->readBoolEntry( "auto rewritable erasing", false ) );
  m_checkAllowWritingAppSelection->setChecked( c->readBoolEntry( "Manual writing app selection", false ) );

  m_checkBurnfree->setChecked( k3bcore->globalSettings()->burnfree() );
  m_checkEject->setChecked( !k3bcore->globalSettings()->ejectMedia() );
  m_checkOverburn->setChecked( k3bcore->globalSettings()->overburn() );
  m_checkManualWritingBufferSize->setChecked( k3bcore->globalSettings()->useManualBufferSize() );
  if( k3bcore->globalSettings()->useManualBufferSize() )
    m_editWritingBufferSize->setValue( k3bcore->globalSettings()->bufferSize() );
}


void K3bBurningOptionTab::saveSettings()
{
  KConfig* c = k3bcore->config();

  c->setGroup( "Video project settings" );
  c->writeEntry( "Use Playback Control", m_checkUsePbc->isChecked() );
  c->writeEntry( "Time to wait after each Sequence/Segment", m_spinWaitTime->value() );
  c->writeEntry( "Play each Sequence/Segment", m_spinPlayTime->value() );
  c->writeEntry( "Use numeric keys to navigate chapters", m_checkUseNumKey->isChecked() );

  c->setGroup( "General Options" );
  c->writeEntry( "auto rewritable erasing", m_checkAutoErasingRewritable->isChecked() );
  c->writeEntry( "Manual writing app selection", m_checkAllowWritingAppSelection->isChecked() );

  k3bcore->globalSettings()->setEjectMedia( !m_checkEject->isChecked() );
  k3bcore->globalSettings()->setOverburn( m_checkOverburn->isChecked() );
  k3bcore->globalSettings()->setBurnfree( m_checkBurnfree->isChecked() );
  k3bcore->globalSettings()->setUseManualBufferSize( m_checkManualWritingBufferSize->isChecked() );
  k3bcore->globalSettings()->setBufferSize( m_editWritingBufferSize->value() );

  // FIXME: remove this once libk3b does not use KConfig anymore for these values
  k3bcore->globalSettings()->saveSettings( c );
}


void K3bBurningOptionTab::slotSetDefaultBufferSizes( bool b )
{
  if( !b ) {
    m_editWritingBufferSize->setValue( 4 );
  }
}


#include "k3bburningoptiontab.moc"
