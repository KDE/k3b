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


#include "k3bpatternoptiontab.h"
#include "../rip/k3bpatternparser.h"

#include <kdialog.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kapplication.h>
#include <klocale.h>
#include <ksqueezedtextlabel.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <kurllabel.h>

#include <qdatetime.h>
#include <qregexp.h>
#include <qvalidator.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qtabwidget.h>
#include <qwhatsthis.h>

static const int PATTERN_COMBO_INDEX_TITLE = 0;
static const int PATTERN_COMBO_INDEX_ARTIST = 1;
static const int PATTERN_COMBO_INDEX_ALBUM = 0;
static const int PATTERN_COMBO_INDEX_TRACK_NUMBER = 2;
static const int PATTERN_COMBO_INDEX_DIR_EMPTY = 2;
static const int PATTERN_COMBO_INDEX_FILE_EMPTY = 3;


K3bPatternOptionTab::K3bPatternOptionTab( QWidget* parent, const char* name )
  : base_K3bPatternOptionTab( parent, name )
{
  // fix all the margins and spacings that have been corrupted by QDesigner ;-)
  // -----------------------------------------------------------------------------

  base_K3bPatternOptionTabLayout->setMargin( 0 );
  base_K3bPatternOptionTabLayout->setSpacing( KDialog::spacingHint() );

  m_mainTabbed->page(0)->layout()->setMargin( KDialog::marginHint() );
  m_mainTabbed->page(0)->layout()->setSpacing( KDialog::spacingHint() );
  m_mainTabbed->page(1)->layout()->setMargin( KDialog::marginHint() );
  m_mainTabbed->page(1)->layout()->setSpacing( KDialog::spacingHint() );


  m_groupBasicDirectoryPattern->layout()->setMargin( 0 );
  m_groupBasicDirectoryPatternLayout->setMargin( KDialog::marginHint() );
  m_groupBasicDirectoryPatternLayout->setSpacing( KDialog::spacingHint() );

  m_groupBasicFilenamePattern->layout()->setMargin( 0 );
  m_groupBasicFilenamePatternLayout->setMargin( KDialog::marginHint() );
  m_groupBasicFilenamePatternLayout->setSpacing( KDialog::spacingHint() );

  m_boxReplaceLayout->setMargin( KDialog::marginHint() );
  m_boxReplaceLayout->setSpacing( KDialog::spacingHint() );


  m_groupAdvancedDirectoryPattern->layout()->setMargin( 0 );
  m_groupAdvancedDirectoryPatternLayout->setMargin( KDialog::marginHint() );
  m_groupAdvancedDirectoryPatternLayout->setSpacing( KDialog::spacingHint() );

  m_groupAdvancedFilenamePattern->layout()->setMargin( 0 );
  m_groupAdvancedFilenamePatternLayout->setMargin( KDialog::marginHint() );
  m_groupAdvancedFilenamePatternLayout->setSpacing( KDialog::spacingHint() );

  m_groupExample->layout()->setMargin( 0 );
  m_groupExampleLayout->setMargin( KDialog::marginHint() );
  m_groupExampleLayout->setSpacing( KDialog::spacingHint() );


  // fill the basic combos
  m_comboBasicDirectory1->insertItem( i18n("Album"), PATTERN_COMBO_INDEX_ALBUM );
  m_comboBasicDirectory1->insertItem( i18n("Artist"), PATTERN_COMBO_INDEX_ARTIST );
  m_comboBasicDirectory1->insertItem( "", PATTERN_COMBO_INDEX_DIR_EMPTY );
  m_comboBasicDirectory2->insertItem( i18n("Album"), PATTERN_COMBO_INDEX_ALBUM );
  m_comboBasicDirectory2->insertItem( i18n("Artist"), PATTERN_COMBO_INDEX_ARTIST );
  m_comboBasicDirectory2->insertItem( "", PATTERN_COMBO_INDEX_DIR_EMPTY );

  m_comboBasicFilename1->insertItem( i18n("Title"), PATTERN_COMBO_INDEX_TITLE );
  m_comboBasicFilename1->insertItem( i18n("Artist"), PATTERN_COMBO_INDEX_ARTIST );
  m_comboBasicFilename1->insertItem( i18n("Track Number"), PATTERN_COMBO_INDEX_TRACK_NUMBER );
  m_comboBasicFilename1->insertItem( "", PATTERN_COMBO_INDEX_FILE_EMPTY );

  m_comboBasicFilename2->insertItem( i18n("Title"), PATTERN_COMBO_INDEX_TITLE );
  m_comboBasicFilename2->insertItem( i18n("Artist"), PATTERN_COMBO_INDEX_ARTIST );
  m_comboBasicFilename2->insertItem( i18n("Track Number"), PATTERN_COMBO_INDEX_TRACK_NUMBER );
  m_comboBasicFilename2->insertItem( "", PATTERN_COMBO_INDEX_FILE_EMPTY );

  m_comboBasicFilename3->insertItem( i18n("Title"), PATTERN_COMBO_INDEX_TITLE );
  m_comboBasicFilename3->insertItem( i18n("Artist"), PATTERN_COMBO_INDEX_ARTIST );
  m_comboBasicFilename3->insertItem( i18n("Track Number"), PATTERN_COMBO_INDEX_TRACK_NUMBER );
  m_comboBasicFilename3->insertItem( "", PATTERN_COMBO_INDEX_FILE_EMPTY );



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
  m_editBasicFilename1->setValidator( fileValidator );
  m_editBasicFilename2->setValidator( fileValidator );


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

  connect( m_specialStringsLabel, SIGNAL(leftClickedURL()), this, SLOT(slotSeeSpecialStrings()) );
}


K3bPatternOptionTab::~K3bPatternOptionTab()
{
}


void K3bPatternOptionTab::readSettings()
{
  KConfig* c = kapp->config();
  c->setGroup("Audio Ripping");

  // basic system
  QString bd1 = c->readEntry( "basic directory 1", "Artist" );
  QString bd2 = c->readEntry( "basic directory 2", "Album" );
  QString bf1 = c->readEntry( "basic filename 1", "Title" );
  QString bf2 = c->readEntry( "basic filename 2", "" );
  QString bf3 = c->readEntry( "basic filename 3", "" );

  if( bd1 == "Artist" )
    m_comboBasicDirectory1->setCurrentItem( PATTERN_COMBO_INDEX_ARTIST );
  else if( bd1 == "Album" )
    m_comboBasicDirectory1->setCurrentItem( PATTERN_COMBO_INDEX_ALBUM );
  else
    m_comboBasicDirectory1->setCurrentItem( PATTERN_COMBO_INDEX_DIR_EMPTY );

  if( bd2 == "Artist" )
    m_comboBasicDirectory2->setCurrentItem( PATTERN_COMBO_INDEX_ARTIST );
  else if( bd2 == "Album" )
    m_comboBasicDirectory2->setCurrentItem( PATTERN_COMBO_INDEX_ALBUM );
  else
    m_comboBasicDirectory2->setCurrentItem( PATTERN_COMBO_INDEX_DIR_EMPTY );


  if( bf1 == "Title" )
    m_comboBasicFilename1->setCurrentItem( PATTERN_COMBO_INDEX_TITLE );
  else if( bf1 == "Artist" )
    m_comboBasicFilename1->setCurrentItem( PATTERN_COMBO_INDEX_ARTIST );
  else if( bf1 == "Track Number" )
    m_comboBasicFilename1->setCurrentItem( PATTERN_COMBO_INDEX_TRACK_NUMBER );
  else
    m_comboBasicFilename1->setCurrentItem( PATTERN_COMBO_INDEX_FILE_EMPTY );

  if( bf2 == "Title" )
    m_comboBasicFilename2->setCurrentItem( PATTERN_COMBO_INDEX_TITLE );
  else if( bf2 == "Artist" )
    m_comboBasicFilename2->setCurrentItem( PATTERN_COMBO_INDEX_ARTIST );
  else if( bf2 == "Track Number" )
    m_comboBasicFilename2->setCurrentItem( PATTERN_COMBO_INDEX_TRACK_NUMBER );
  else
    m_comboBasicFilename2->setCurrentItem( PATTERN_COMBO_INDEX_FILE_EMPTY );

  if( bf3 == "Title" )
    m_comboBasicFilename3->setCurrentItem( PATTERN_COMBO_INDEX_TITLE );
  else if( bf3 == "Artist" )
    m_comboBasicFilename3->setCurrentItem( PATTERN_COMBO_INDEX_ARTIST );
  else if( bf3 == "Track Number" )
    m_comboBasicFilename3->setCurrentItem( PATTERN_COMBO_INDEX_TRACK_NUMBER );
  else
    m_comboBasicFilename3->setCurrentItem( PATTERN_COMBO_INDEX_FILE_EMPTY );


  m_editBasicFilename1->setText( c->readEntry( "basic filename fill 1", "" ) );
  m_editBasicFilename2->setText( c->readEntry( "basic filename fill 2", "" ) );


  // advanced system
  m_comboAdvancedDirectoryPattern->setEditText( c->readEntry( "advanced directory pattern", "%r/%m" ) );
  m_comboAdvancedFilenamePattern->setEditText( c->readEntry( "advanced filename pattern", "%a - %t" ) );

  if( c->readBoolEntry( "use advanced pattern system", false ) )
    m_mainTabbed->setCurrentPage( 1 );

  m_checkDirectoryReplace->setChecked( c->readBoolEntry( "replace blank in directory", false ) );
  m_checkFilenameReplace->setChecked( c->readBoolEntry( "replace blank in filename", false ) );
  m_editDirectoryReplace->setText( c->readEntry( "directory replace string", "_" ) );
  m_editFilenameReplace->setText( c->readEntry( "filename replace string", "_" ) );

  slotUpdateExample();
}


void K3bPatternOptionTab::apply()
{
  KConfig* c = kapp->config();
  c->setGroup("Audio Ripping");

  // basic system
  c->writeEntry( "basic directory 1", basicPatternItemFromIndex(m_comboBasicDirectory1->currentItem(), true) );
  c->writeEntry( "basic directory 2", basicPatternItemFromIndex(m_comboBasicDirectory2->currentItem(), true) );

  c->writeEntry( "basic filename 1", basicPatternItemFromIndex(m_comboBasicFilename1->currentItem(), false) );
  c->writeEntry( "basic filename 2", basicPatternItemFromIndex(m_comboBasicFilename2->currentItem(), false) );
  c->writeEntry( "basic filename 3", basicPatternItemFromIndex(m_comboBasicFilename3->currentItem(), false) );

  c->writeEntry( "basic filename fill 1", m_editBasicFilename1->text() );
  c->writeEntry( "basic filename fill 2", m_editBasicFilename2->text() );


  // advanced system
  c->writeEntry( "advanced directory pattern", m_comboAdvancedDirectoryPattern->currentText() );
  c->writeEntry( "advanced filename pattern", m_comboAdvancedFilenamePattern->currentText() );

  c->writeEntry( "replace blank in directory", m_checkDirectoryReplace->isChecked() );
  c->writeEntry( "replace blank in filename", m_checkFilenameReplace->isChecked() );
  c->writeEntry( "directory replace string", m_editDirectoryReplace->text() );
  c->writeEntry( "filename replace string", m_editFilenameReplace->text() );

  bool advanced = m_mainTabbed->indexOf( m_mainTabbed->currentPage() ) == 1;

  c->writeEntry( "use advanced pattern system", advanced );
  if( advanced ) {
    c->writeEntry( "directory pattern", m_comboAdvancedDirectoryPattern->currentText() );
    c->writeEntry( "filename pattern", m_comboAdvancedFilenamePattern->currentText() );
  }
  else {
    c->writeEntry( "directory pattern", basicDirectoryPattern() );
    c->writeEntry( "filename pattern", basicFilenamePattern() );
  }
}


void K3bPatternOptionTab::slotUpdateExample()
{
  QString dir, file;

  if( m_mainTabbed->indexOf( m_mainTabbed->currentPage() ) == 0 ) {
    dir = K3bPatternParser::parsePattern( m_exampleEntry, 1, 
					  basicDirectoryPattern(),
					  m_checkDirectoryReplace->isChecked(), 
					  m_editDirectoryReplace->text() );
    file = K3bPatternParser::parsePattern( m_exampleEntry, 1, 
					   basicFilenamePattern(),
					   m_checkFilenameReplace->isChecked(), 
					   m_editFilenameReplace->text() );
  }
  else {
    dir = K3bPatternParser::parsePattern( m_exampleEntry, 1, 
					  m_comboAdvancedDirectoryPattern->currentText(), 
					  m_checkDirectoryReplace->isChecked(), 
					  m_editDirectoryReplace->text() );
    file = K3bPatternParser::parsePattern( m_exampleEntry, 1, 
					   m_comboAdvancedFilenamePattern->currentText(), 
					   m_checkFilenameReplace->isChecked(), 
					   m_editFilenameReplace->text() );
  }


  if( !dir.isEmpty() && dir[dir.length()-1] != '/' )
    dir.append( "/" );

  m_labelExample->setText( dir + file + ".ogg" );
}


QString K3bPatternOptionTab::basicFilenamePattern()
{
  QString filePattern = patternForName( m_comboBasicFilename1->currentText() );
  filePattern.append( m_editBasicFilename1->text() );
  filePattern.append( patternForName( m_comboBasicFilename2->currentText() ) );
  filePattern.append( m_editBasicFilename2->text() );
  filePattern.append( patternForName( m_comboBasicFilename3->currentText() ) );

  return filePattern;
}

QString K3bPatternOptionTab::basicDirectoryPattern()
{
  QString dirPattern = patternForName( m_comboBasicDirectory1->currentText() );
  if( !dirPattern.isEmpty() )
    dirPattern.append( "/" );
  dirPattern.append( patternForName( m_comboBasicDirectory2->currentText() ) );
  
  return dirPattern;
}


QString K3bPatternOptionTab::patternForName( const QString& name )
{
  if( name == i18n("Title") )
    return "%t";
  else if( name == i18n("Artist") )
    return "%a";
  else if( name == i18n("Album") )
    return "%m";
  else if( name == i18n("Track Number") )
    return "%n";
  else
    return "";
}


QString K3bPatternOptionTab::basicPatternItemFromIndex( int i, bool dir )
{
  if( dir ) {
    switch( i ) {
    case PATTERN_COMBO_INDEX_ARTIST:
      return "Artist";
    case PATTERN_COMBO_INDEX_ALBUM:
      return "Album";
    default:
      return "";
    }
  }
  else {
    switch( i ) {
    case PATTERN_COMBO_INDEX_ARTIST:
      return "Artist";
    case PATTERN_COMBO_INDEX_TITLE:
      return "Title";
    case PATTERN_COMBO_INDEX_TRACK_NUMBER:
      return "Track Number";
    default:
      return "";
    }
  }
}


void K3bPatternOptionTab::slotSeeSpecialStrings()
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

#include "k3bpatternoptiontab.moc"
