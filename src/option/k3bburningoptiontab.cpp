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
  QGridLayout* groupAdvancedLayout = new QGridLayout( this );
  groupAdvancedLayout->setAlignment( Qt::AlignTop );
  groupAdvancedLayout->setSpacing( KDialog::spacingHint() );
  groupAdvancedLayout->setMargin( 0 );


  QGroupBox* groupWritingApp = new QGroupBox( 0, Qt::Vertical, i18n("Burning"), this );
  groupWritingApp->layout()->setMargin( 0 );
  QGridLayout* bufferLayout = new QGridLayout( groupWritingApp->layout() );
  bufferLayout->setMargin( KDialog::marginHint() );
  bufferLayout->setSpacing( KDialog::spacingHint() );

  m_checkBurnfree = K3bStdGuiItems::burnproofCheckbox( groupWritingApp );
  m_checkOverburn = new QCheckBox( i18n("Allow &overburning (not supported by cdrecord <= 1.10)"), groupWritingApp );
  m_checkManualWritingBufferSize = new QCheckBox( i18n("&Manual writing buffer size") + ":", groupWritingApp );
  m_editWritingBufferSize = new KIntNumInput( 4, groupWritingApp );
  m_editWritingBufferSize->setSuffix( " " + i18n("MB") );
  m_checkAllowWritingAppSelection = new QCheckBox( i18n("Manual writing application &selection"), groupWritingApp );
  bufferLayout->addMultiCellWidget( m_checkBurnfree, 0, 0, 0, 2 );
  bufferLayout->addMultiCellWidget( m_checkOverburn, 1, 1, 0, 2 );
  bufferLayout->addWidget( m_checkManualWritingBufferSize, 2, 0 );
  bufferLayout->addWidget( m_editWritingBufferSize, 2, 1 );
  bufferLayout->addMultiCellWidget( m_checkAllowWritingAppSelection, 3, 3, 0, 2 );
  bufferLayout->setColStretch( 2, 1 );

  QGroupBox* groupMisc = new QGroupBox( 2, Qt::Vertical, i18n("Miscellaneous"), this );
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



  QToolTip::add( m_checkAllowWritingAppSelection, i18n("Allow to choose between cdrecord and cdrdao") );

  QToolTip::add( m_checkAutoErasingRewritable, i18n("Automatically erase CD-RWs and DVD-RWs without asking") );

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

  QWhatsThis::add( m_checkManualWritingBufferSize, i18n("<p>K3b uses a software buffer during the burning process to "
							"avoid gaps in the data stream due to high system load. The default "
							"sizes used are %1 MB for CD and %2 MB for DVD burning."
							"<p>If this option is checked the value specified will be used for both "
							"CD and DVD burning.").arg(4).arg(32) );
}


void K3bBurningOptionTab::readSettings()
{
  KConfig* c = k3bcore->config();

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
