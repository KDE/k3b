/***************************************************************************
                          k3bpatternoptiontab.cpp  -  description
                             -------------------
    begin                : Sat May 4 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3bpatternoptiontab.h"
#include "../rip/k3bpatternparser.h"

#include <kdialog.h>
#include <kdebug.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <kconfig.h>
#include <kapplication.h>
#include <ksqueezedtextlabel.h>

#include <qdatetime.h>
#include <qregexp.h>
#include <qvalidator.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qgroupbox.h>


K3bPatternOptionTab::K3bPatternOptionTab( QWidget* parent, const char* name )
  : base_K3bPatternOptionTab( parent, name )
{
  // fix all the margins and spacings that have been corrupted by QDesigner ;-)
  // -----------------------------------------------------------------------------

  base_K3bPatternOptionTabLayout->setMargin( 0 );
  base_K3bPatternOptionTabLayout->setSpacing( KDialog::spacingHint() );
  //  base_K3bPatternOptionTabLayout->setStretchFactor( m_infoLabel, 1 );

  m_groupDirectoryPattern->layout()->setMargin( 0 );
  m_groupDirectoryPatternLayout->setMargin( KDialog::marginHint() );
  m_groupDirectoryPatternLayout->setSpacing( KDialog::spacingHint() );

  m_groupFilenamePattern->layout()->setMargin( 0 );
  m_groupFilenamePatternLayout->setMargin( KDialog::marginHint() );
  m_groupFilenamePatternLayout->setSpacing( KDialog::spacingHint() );

  m_groupExample->layout()->setMargin( 0 );
  m_groupExampleLayout->setMargin( KDialog::marginHint() );
  m_groupExampleLayout->setSpacing( KDialog::spacingHint() );


  // setup validators
  // there can never be one of the following characters in both dir and filename:
  // * ? "
  // additional the filename can never contain a slash /
  // and the dir should never start with a slash since it should always be a relative path

  QRegExpValidator* dirValidator = new QRegExpValidator( QRegExp( "[^/][^?\\*\\\"]*" ), m_comboDirectoryPattern );
  m_comboDirectoryPattern->setValidator( dirValidator );
  m_editDirectoryReplace->setValidator( dirValidator );

  QRegExpValidator* fileValidator = new QRegExpValidator( QRegExp( "[^/?\\*\\\"]*" ), m_comboDirectoryPattern );
  m_comboFilenamePattern->setValidator( fileValidator );
  m_editFilenameReplace->setValidator( fileValidator );


  // create example entry
  m_exampleEntry.cdArtist = "Morcheeba";
  m_exampleEntry.cdTitle = "Fragments Of Freedom";
  m_exampleEntry.cdExtInfo = "This is some good music";
  m_exampleEntry.category = "misc";
  m_exampleEntry.genre = "Popmusic";
  m_exampleEntry.titles.append( "Let It Go" );
  m_exampleEntry.artists.append( "Morcheeba" );
  m_exampleEntry.extInfos.append( "This track has been recorded" );

  slotUpdateExample();
}


K3bPatternOptionTab::~K3bPatternOptionTab()
{
}


void K3bPatternOptionTab::slotUpdateExample()
{
  QString dir = K3bPatternParser::parsePattern( m_exampleEntry, 1, 
						m_comboDirectoryPattern->currentText(), 
						m_checkDirectoryReplace->isChecked(), 
						m_editDirectoryReplace->text() );

  QString file = K3bPatternParser::parsePattern( m_exampleEntry, 1, 
						 m_comboFilenamePattern->currentText(), 
						 m_checkFilenameReplace->isChecked(), 
						 m_editFilenameReplace->text() );

  if( !dir.isEmpty() && dir[dir.length()-1] != '/' )
    dir.append( "/" );

  m_labelExample->setText( dir + file + ".ogg" );
}


void K3bPatternOptionTab::readSettings()
{
  KConfig* c = kapp->config();
  c->setGroup("Ripping");

  m_comboDirectoryPattern->setEditText( c->readEntry( "directory pattern", "%r/%m" ) );
  m_comboFilenamePattern->setEditText( c->readEntry( "filename pattern", "%a - %t" ) );
  m_checkDirectoryReplace->setChecked( c->readBoolEntry( "replace blank in directory", false ) );
  m_checkFilenameReplace->setChecked( c->readBoolEntry( "replace blank in filename", false ) );
  m_editDirectoryReplace->setText( c->readEntry( "directory replace string", "_" ) );
  m_editFilenameReplace->setText( c->readEntry( "filename replace string", "_" ) );

  slotUpdateExample();
}


void K3bPatternOptionTab::apply()
{
  KConfig* c = kapp->config();
  c->setGroup("Ripping");

  c->writeEntry( "directory pattern", m_comboDirectoryPattern->currentText() );
  c->writeEntry( "filename pattern", m_comboFilenamePattern->currentText() );
  c->writeEntry( "replace blank in directory", m_checkDirectoryReplace->isChecked() );
  c->writeEntry( "replace blank in filename", m_checkFilenameReplace->isChecked() );
  c->writeEntry( "directory replace string", m_editDirectoryReplace->text() );
  c->writeEntry( "filename replace string", m_editFilenameReplace->text() );
}


#include "k3bpatternoptiontab.moc"
