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

#include "k3bdvdformattingdialog.h"
#include "k3bdvdformattingjob.h"

#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bglobals.h>
#include <k3bcore.h>
#include <k3bwriterselectionwidget.h>
#include <k3bwritingmodewidget.h>
#include <k3bjobprogressdialog.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <kapplication.h>

#include <qgroupbox.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qframe.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <qwhatsthis.h>


K3bDvdFormattingDialog::K3bDvdFormattingDialog( QWidget* parent, const char* name, bool modal )
  : K3bInteractionDialog( parent, name,
			  i18n("DVD Formatting"),
			  i18n("DVD-RW and DVD+RW"),
			  START_BUTTON|CANCEL_BUTTON,
			  START_BUTTON,
			  "DVD Formatting", // config group
			  modal )
{
  setCancelButtonText( i18n("Close") );


  QWidget* frame = mainWidget();

  m_writerSelectionWidget = new K3bWriterSelectionWidget( true, frame );
  m_writerSelectionWidget->setSupportedWritingApps( K3b::DVD_RW_FORMAT );
  m_writerSelectionWidget->setForceAutoSpeed(true);

  QGroupBox* groupWritingMode = new QGroupBox( 1, Qt::Vertical, i18n("Writing Mode"), frame );
  groupWritingMode->layout()->setMargin( marginHint() );
  groupWritingMode->layout()->setSpacing( spacingHint() );
  m_writingModeWidget = new K3bWritingModeWidget( K3b::WRITING_MODE_INCR_SEQ|K3b::WRITING_MODE_RES_OVWR,
						  groupWritingMode );


  QGroupBox* groupOptions = new QGroupBox( 2, Qt::Vertical, i18n("Options"), frame );
  groupOptions->layout()->setMargin( marginHint() );
  groupOptions->layout()->setSpacing( spacingHint() );
  m_checkForce = new QCheckBox( i18n("Force"), groupOptions );
  m_checkQuickFormat = new QCheckBox( i18n("Quick format"), groupOptions );

  QGridLayout* grid = new QGridLayout( frame );
  grid->setMargin( 0 );
  grid->setSpacing( spacingHint() );

  grid->addMultiCellWidget( m_writerSelectionWidget, 0, 0, 0, 1 );
  grid->addWidget( groupWritingMode, 1, 0 );
  grid->addWidget( groupOptions, 1, 1 );
  grid->setRowStretch( 1, 1 );


  QToolTip::add( m_checkForce, i18n("Force formatting of empty DVDs") );
  QWhatsThis::add( m_checkForce, i18n("<p>If this option is checked K3b will format a "
				      "DVD-RW even if it is empty. It may also be used to "
				      "force K3b to format a DVD+RW or a DVD-RW in restricted "
				      "overwrite mode."
				      "<p><b>Caution:</b> It is not recommended to often format a DVD "
				      "since it may already be unusable after 10-20 reformat procedures."
				      "<p>DVD+RW media only needs to be formatted once. After that it "
				      "just needs to be overwritten. The same applies to DVD-RW in "
				      "restricted overwrite mode.") );

  QToolTip::add( m_checkQuickFormat, i18n("Try to perform quick formatting") );
  QWhatsThis::add( m_checkQuickFormat, i18n("<p>If this option is checked K3b will tell the writer "
					    "to perform a quick format."
					    "<p>Formatting a DVD-RW completely can take a very long "
					    "time and some DVD writers perform a full format even if "
					    "quick format is enabled." ) );
  connect( m_writerSelectionWidget, SIGNAL(writerChanged()), this, SLOT(slotWriterChanged()) );

  slotWriterChanged();
} 


K3bDvdFormattingDialog::~K3bDvdFormattingDialog()
{
} 


void K3bDvdFormattingDialog::slotStartClicked()
{
  //
  // create a jobprogressdialog and start the job
  //



  K3bJobProgressDialog d( kapp->mainWidget(), "formattingProgress", false );

  K3bDvdFormattingJob*  job = new K3bDvdFormattingJob( &d, this );
  job->setDevice( m_writerSelectionWidget->writerDevice() );
  job->setMode( m_writingModeWidget->writingMode() );
  job->setForce( m_checkForce->isChecked() );
  job->setQuickFormat( m_checkQuickFormat->isChecked() );

  hide();

  d.startJob( job );

  delete job;

  show();
} 


void K3bDvdFormattingDialog::slotWriterChanged()
{
  m_buttonStart->setEnabled( m_writerSelectionWidget->writerDevice() != 0 );
} 


void K3bDvdFormattingDialog::loadUserDefaults( KConfig* c )
{
  m_checkForce->setChecked( c->readBoolEntry( "force", false ) );
  m_checkQuickFormat->setChecked( c->readBoolEntry( "quick format", true ) );
  m_writerSelectionWidget->loadConfig( c );
  m_writingModeWidget->loadConfig( c );
} 


void K3bDvdFormattingDialog::saveUserDefaults( KConfig* c )
{
  c->writeEntry( "force", m_checkForce->isChecked() );
  c->writeEntry( "quick format", m_checkQuickFormat->isChecked() );
  m_writerSelectionWidget->saveConfig( c );
  m_writingModeWidget->saveConfig( c );
} 


void K3bDvdFormattingDialog::loadK3bDefaults()
{
  m_writerSelectionWidget->loadDefaults();
  m_checkForce->setChecked( false );
  m_checkQuickFormat->setChecked( true );
  m_writingModeWidget->setWritingMode( K3b::WRITING_MODE_AUTO );
} 


#include "k3bdvdformattingdialog.moc"
