/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bcddbpatternwidget.h"

#include <kconfig.h>
#include <klocale.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <kurllabel.h>

#include <qregexp.h>
#include <qvalidator.h>
#include <qwhatsthis.h>
#include <qcheckbox.h>
#include <qlayout.h>


K3bCddbPatternWidget::K3bCddbPatternWidget( QWidget* parent, const char* name )
  : base_K3bCddbPatternWidget( parent, name )
{
  // fix the layout
  ((QGridLayout*)layout())->setRowStretch( 4, 1 );

  // setup validators
  // there can never be one of the following characters in both dir and filename:
  // * ? "
  // additional the filename can never contain a slash /
  // and the dir should never start with a slash since it should always be a relative path

  QRegExpValidator* dirValidator = new QRegExpValidator( QRegExp( "[^/][^?\\*\\\"]*" ), this );
  m_comboAdvancedDirectoryPattern->setValidator( dirValidator );
  m_editDirectoryReplace->setValidator( dirValidator );

  QRegExpValidator* fileValidator = new QRegExpValidator( QRegExp( "[^/?\\*\\\"]*" ), this );
  m_comboAdvancedFilenamePattern->setValidator( fileValidator );
  m_editFilenameReplace->setValidator( fileValidator );

  connect( m_comboAdvancedFilenamePattern, SIGNAL(textChanged(const QString&)),
	   this, SIGNAL(changed()) );
  connect( m_comboAdvancedDirectoryPattern, SIGNAL(textChanged(const QString&)),
	   this, SIGNAL(changed()) );
  connect( m_editFilenameReplace, SIGNAL(textChanged(const QString&)),
	   this, SIGNAL(changed()) );
  connect( m_editDirectoryReplace, SIGNAL(textChanged(const QString&)),
	   this, SIGNAL(changed()) );
  connect( m_checkFilenameReplace, SIGNAL(toggled(bool)),
	   this, SIGNAL(changed()) );
  connect( m_checkDirectoryReplace, SIGNAL(toggled(bool)),
	   this, SIGNAL(changed()) );

  connect( m_specialStringsLabel, SIGNAL(leftClickedURL()), this, SLOT(slotSeeSpecialStrings()) );
}


K3bCddbPatternWidget::~K3bCddbPatternWidget()
{
}


QString K3bCddbPatternWidget::filenamePattern() const
{
  return m_comboAdvancedFilenamePattern->currentText();
}


QString K3bCddbPatternWidget::directoryPattern() const
{
  return m_comboAdvancedDirectoryPattern->currentText();
}


QString K3bCddbPatternWidget::filenameReplaceString() const
{
  return m_editFilenameReplace->text();
}


QString K3bCddbPatternWidget::directoryReplaceString() const
{
  return m_editDirectoryReplace->text();
}


bool K3bCddbPatternWidget::replaceBlanksInFilename() const
{
  return m_checkFilenameReplace->isChecked();
}


bool K3bCddbPatternWidget::replaceBlanksInDirectory() const
{
  return m_checkDirectoryReplace->isChecked();
}


void K3bCddbPatternWidget::loadConfig( KConfig* c )
{
  m_comboAdvancedDirectoryPattern->setEditText( c->readEntry( "directory pattern", "%r/%m" ) );
  m_comboAdvancedFilenamePattern->setEditText( c->readEntry( "filename pattern", "%a - %t" ) );
  m_checkDirectoryReplace->setChecked( c->readBoolEntry( "replace blank in directory", false ) );
  m_checkFilenameReplace->setChecked( c->readBoolEntry( "replace blank in filename", false ) );
  m_editDirectoryReplace->setText( c->readEntry( "directory replace string", "_" ) );
  m_editFilenameReplace->setText( c->readEntry( "filename replace string", "_" ) );
}


void K3bCddbPatternWidget::saveConfig( KConfig* c )
{
  c->writeEntry( "directory pattern", m_comboAdvancedDirectoryPattern->currentText() );
  c->writeEntry( "filename pattern", m_comboAdvancedFilenamePattern->currentText() );
  c->writeEntry( "replace blank in directory", m_checkDirectoryReplace->isChecked() );
  c->writeEntry( "replace blank in filename", m_checkFilenameReplace->isChecked() );
  c->writeEntry( "directory replace string", m_editDirectoryReplace->text() );
  c->writeEntry( "filename replace string", m_editFilenameReplace->text() );
}


void K3bCddbPatternWidget::loadDefaults()
{
  m_comboAdvancedDirectoryPattern->setEditText( "%r/%m" );
  m_comboAdvancedFilenamePattern->setEditText( "%a - %t" );
  m_checkDirectoryReplace->setChecked( false );
  m_checkFilenameReplace->setChecked( false );
  m_editDirectoryReplace->setText( "_" );
  m_editFilenameReplace->setText( "_" );
}


void K3bCddbPatternWidget::slotSeeSpecialStrings()
{
  QWhatsThis::display( i18n( "<p><b>Pattern special strings:</b>"
			     "<ul>\n"
			     "<li>%a - artist of the track\n"
			     "<li>%t - title of the track\n"
			     "<li>%n - track number\n"
			     "<li>%e - extended information about the track\n"
			     "<li>%g - genre of the CD\n"
			     "<li>%r - album artist (differs from %a only on soundtracks or compilations)\n"
			     "<li>%m - album title\n"
			     "<li>%x - extended information about the CD\n"
			     "<li>%d - current date\n"
			     "</ul>" ) );
}

#include "k3bcddbpatternwidget.moc"

