/***************************************************************************
                          k3baudiotrackdialog.cpp  -  description
                             -------------------
    begin                : Sat Mar 31 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#include <qlineedit.h>
#include <qmultilineedit.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qframe.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qhbox.h>
#include <qtabwidget.h>

#include <kiconloader.h>
#include <klocale.h>
#include <knuminput.h>
#include <kmimetype.h>
#include <kurl.h>

#include "k3baudiotrackdialog.h"
#include "k3baudiotrack.h"
#include "../kcutlabel.h"
#include "../tools/k3bglobals.h"


K3bAudioTrackDialog::K3bAudioTrackDialog( QPtrList<K3bAudioTrack>& tracks, QWidget *parent, const char *name )
  : KDialogBase( KDialogBase::Plain, i18n("Audio Track Properties"), KDialogBase::Ok|KDialogBase::Cancel|KDialogBase::Apply,
		 KDialogBase::Ok, parent, name )
{
  setupGui();
  setupConnections();
	
  m_tracks = tracks;

  m_bPregapSeconds = true;


  if( !m_tracks.isEmpty() ) {

    K3bAudioTrack* track = m_tracks.first();

    QString allTrackNames = track->fileName();
    long allTrackLength = track->length();
    long allTrackSize = track->size();

    m_editTitle->setText( track->title() );
    m_editPerformer->setText( track->artist() );
    m_editArranger->setText( track->arranger() );
    m_editSongwriter->setText( track->songwriter() );
    m_editComposer->setText( track->composer() );
    m_editIsrc->setText( track->isrc() );
    m_editMessage->setText( track->cdTextMessage() );
    
    m_checkCopy->setChecked( track->copyProtection() );
    m_checkPreEmp->setChecked( track->preEmp() );
    
    if( m_bPregapSeconds )
      m_inputPregap->setValue( track->pregap() / 75 );
    else
      m_inputPregap->setValue( track->pregap() );
    
    for( track = m_tracks.next(); track != 0; track = m_tracks.next() ) {

      allTrackNames += ("\n" + track->fileName());
      allTrackLength += track->length();
      allTrackSize += track->size();


      if( track->title() != m_editTitle->text() )
	m_editTitle->setText( QString::null );

      if( track->artist() != m_editPerformer->text() )
	m_editPerformer->setText( QString::null );

      if( track->arranger() != m_editArranger->text() )
	m_editArranger->setText( QString::null );

      if( track->songwriter() != m_editSongwriter->text() )
	m_editSongwriter->setText( QString::null );

      if( track->composer() != m_editComposer->text() )
	m_editComposer->setText( QString::null );

      if( track->isrc() != m_editIsrc->text() )
	m_editIsrc->setText( QString::null );

      if( track->cdTextMessage() != m_editMessage->text() )
	m_editMessage->setText( QString::null );

      if( track->copyProtection() != m_checkCopy->isChecked() )
	m_checkCopy->setNoChange();

      if( track->preEmp() != m_checkPreEmp->isChecked() )
	m_checkPreEmp->setNoChange();

      // ignore the pregap for the time being...
    }

    m_displayFileName->setText( allTrackNames );
    m_displayLength->setText( K3b::framesToString(allTrackLength) );
    m_displaySize->setText( i18n("%1 kb").arg(allTrackSize / 1024) );

    m_labelMimeType->setPixmap( KMimeType::pixmapForURL( KURL(m_tracks.first()->absPath()), 0, KIcon::Desktop, 48 ) );
  }

  m_editTitle->setFocus();
}

K3bAudioTrackDialog::~K3bAudioTrackDialog()
{
}
	

void K3bAudioTrackDialog::slotOk()
{
  slotApply();
  done(0);
}


void K3bAudioTrackDialog::slotApply()
{
  // apply all changes to all tracks
  for( K3bAudioTrack* track = m_tracks.first(); track != 0; track = m_tracks.next() ) {
    
    if( m_editTitle->edited() )
      track->setTitle( m_editTitle->text() );

    if( m_editPerformer->edited() )
      track->setArtist( m_editPerformer->text() );

    if( m_editArranger->edited() )
      track->setArranger( m_editArranger->text() );

    if( m_editSongwriter->edited() )
      track->setSongwriter( m_editSongwriter->text() );

    if( m_editComposer->edited() )
      track->setComposer( m_editComposer->text() );

    if( m_editIsrc->edited() )
      track->setIsrc( m_editIsrc->text() );

    if( m_editMessage->edited() )
      track->setCdTextMessage( m_editMessage->text() );

    if( m_checkCopy->state() != QButton::NoChange )
      track->setCopyProtection( m_checkCopy->isChecked() );

    if( m_checkPreEmp->state() != QButton::NoChange )
      track->setPreEmp( m_checkPreEmp->isChecked() );

    if( m_bPregapSeconds )
      track->setPregap( m_inputPregap->value()*75 );
    else
      track->setPregap( m_inputPregap->value() );
  }
}


void K3bAudioTrackDialog::setupGui()
{
  QFrame* frame = plainPage();

  QGridLayout* mainLayout = new QGridLayout( frame );
  mainLayout->setSpacing( spacingHint() );
  mainLayout->setMargin( 0 );

  QTabWidget* mainTabbed = new QTabWidget( frame );


  // /////////////////////////////////////////////////
  // OPTIONS TAB
  // /////////////////////////////////////////////////
  // /////////////////////////////////////////////////
  QWidget* optionsTab = new QWidget( mainTabbed );
  QGridLayout* optionsGrid = new QGridLayout( optionsTab );
  optionsGrid->setSpacing( spacingHint() );
  optionsGrid->setMargin( marginHint() );

  QLabel* labelPregap = new QLabel( i18n("&Pregap:"), optionsTab );
  m_inputPregap       = new KIntNumInput( optionsTab, "m_inputPregap" );
  m_comboPregapFormat = new QComboBox( optionsTab );
  labelPregap->setBuddy( m_inputPregap );

  m_checkPreEmp       = new QCheckBox( i18n( "Pr&eemphasis" ), optionsTab, "m_checkPreEmp" );
  m_checkCopy         = new QCheckBox( i18n( "&Copy protected" ), optionsTab, "m_checkCopy" );

  m_comboPregapFormat->insertItem( i18n("Seconds" ) );
  m_comboPregapFormat->insertItem( i18n("Frames" ) );

  optionsGrid->addWidget( labelPregap, 0, 0 );
  optionsGrid->addWidget( m_inputPregap, 0, 1 );
  optionsGrid->addWidget( m_comboPregapFormat, 0, 2 );
  optionsGrid->addMultiCellWidget( m_checkPreEmp, 1, 1, 0, 2 );
  optionsGrid->addMultiCellWidget( m_checkCopy, 2, 2, 0, 2 );

  optionsGrid->setRowStretch( 3, 1 );
  // /////////////////////////////////////////////////
  // /////////////////////////////////////////////////



  // /////////////////////////////////////////////////
  // CD TEXT TAB
  // /////////////////////////////////////////////////
  // /////////////////////////////////////////////////
  QWidget* cdTextTab = new QWidget( mainTabbed );
  QGridLayout* cdTextTabLayout = new QGridLayout( cdTextTab );
  cdTextTabLayout->setAlignment( Qt::AlignTop );
  cdTextTabLayout->setSpacing( spacingHint() );
  cdTextTabLayout->setMargin( marginHint() );

  QLabel* labelMessage    = new QLabel( i18n( "&Message:" ), cdTextTab, "labelMessage" );
  QLabel* labelPerformer  = new QLabel( i18n( "&Performer:" ), cdTextTab, "labelPerformer" );
  QLabel* labelTitle      = new QLabel( i18n( "&Title:" ), cdTextTab, "labelTitle" );
  QLabel* labelIsrc       = new QLabel( i18n( "&ISRC:" ), cdTextTab, "labelIsrc" );
  QLabel* labelSongwriter = new QLabel( i18n( "&Songwriter:" ), cdTextTab, "labelSongwriter" );
  QLabel* labelComposer   = new QLabel( i18n( "&Composer:" ), cdTextTab, "labelComposer" );
  QLabel* labelArranger   = new QLabel( i18n( "&Arranger:" ), cdTextTab, "labelArranger" );

  labelMessage->setAlignment( Qt::AlignLeft | Qt::AlignTop );

  m_editPerformer  = new QLineEdit( cdTextTab, "m_editPerformer" );
  m_editTitle      = new QLineEdit( cdTextTab, "m_editTitle" );
  m_editMessage    = new QMultiLineEdit( cdTextTab, "m_editMessage" );
  m_editArranger   = new QLineEdit( cdTextTab, "m_editArranger" );
  m_editSongwriter = new QLineEdit( cdTextTab, "m_editSongwriter" );
  m_editComposer = new QLineEdit( cdTextTab, "m_editComposer" );
  m_editIsrc       = new QLineEdit( cdTextTab, "m_editIsrc" );
  QFrame* line1    = new QFrame( cdTextTab, "_line1" );

  //  m_editPerformer->setMinimumWidth( 100 );
  m_editMessage->setWordWrap( QMultiLineEdit::WidgetWidth );
  line1->setFrameStyle( QFrame::HLine | QFrame::Sunken );


  cdTextTabLayout->addWidget( labelPerformer, 1, 0 );
  cdTextTabLayout->addWidget( m_editPerformer, 1, 1 );
  cdTextTabLayout->addWidget( labelTitle, 0, 0 );
  cdTextTabLayout->addWidget( m_editTitle, 0, 1 );
  cdTextTabLayout->addMultiCellWidget( line1, 2, 2, 0, 1 );
  cdTextTabLayout->addWidget( labelArranger, 3, 0 );
  cdTextTabLayout->addWidget( m_editArranger, 3, 1 );
  cdTextTabLayout->addWidget( labelSongwriter, 4, 0 );
  cdTextTabLayout->addWidget( m_editSongwriter, 4, 1 );
  cdTextTabLayout->addWidget( labelComposer, 5, 0 );
  cdTextTabLayout->addWidget( m_editComposer, 5, 1 );
  cdTextTabLayout->addWidget( labelIsrc, 6, 0 );
  cdTextTabLayout->addWidget( m_editIsrc, 6, 1 );
  cdTextTabLayout->addWidget( labelMessage, 7, 0 );
  cdTextTabLayout->addWidget( m_editMessage, 7, 1 );

  cdTextTabLayout->setRowStretch( 7, 1 );


  // buddies
  labelPerformer->setBuddy( m_editPerformer );
  labelTitle->setBuddy( m_editTitle );
  labelMessage->setBuddy( m_editMessage );
  labelIsrc->setBuddy( m_editIsrc );
  labelSongwriter->setBuddy( m_editSongwriter );
  labelComposer->setBuddy( m_editComposer );
  labelArranger->setBuddy( m_editArranger );

  // tab order
  setTabOrder( m_editTitle, m_editPerformer );
  setTabOrder( m_editPerformer, m_editArranger );
  setTabOrder( m_editArranger, m_editSongwriter );
  setTabOrder( m_editSongwriter, m_editComposer );
  setTabOrder( m_editComposer, m_editIsrc );
  setTabOrder( m_editIsrc, m_editMessage );

  // /////////////////////////////////////////////////
  // /////////////////////////////////////////////////


  // /////////////////////////////////////////////////
  // FILE-INFO BOX
  // /////////////////////////////////////////////////
  // /////////////////////////////////////////////////

  QGroupBox* groupFileInfo = new QGroupBox( 0, Qt::Vertical, i18n( "File Info" ), frame, "groupFileInfo" );
  groupFileInfo->layout()->setSpacing( 0 );
  groupFileInfo->layout()->setMargin( 0 );
  QGridLayout* groupFileInfoLayout = new QGridLayout( groupFileInfo->layout() );
  groupFileInfoLayout->setAlignment( Qt::AlignTop );
  groupFileInfoLayout->setSpacing( spacingHint() );
  groupFileInfoLayout->setMargin( marginHint() );

  m_labelMimeType = new QLabel( groupFileInfo, "m_labelMimeType" );
  m_displayFileName = new KCutLabel( groupFileInfo );
  m_displayFileName->setText( i18n( "Filename" ) );
  m_displayFileName->setAlignment( int( QLabel::AlignTop | QLabel::AlignLeft ) );
  QLabel* labelSize = new QLabel( i18n( "Size" ), groupFileInfo, "labelSize" );
  QLabel* labelLength = new QLabel( i18n( "Length"), groupFileInfo, "labelLength" );
  m_displaySize = new QLabel( groupFileInfo, "m_displaySize" );
  m_displaySize->setText( i18n( "0.0 MB" ) );
  m_displaySize->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );
  m_displayLength = new QLabel( groupFileInfo, "m_displayLength" );
  m_displayLength->setText( i18n( "0:0:0" ) );
  m_displayLength->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

  QFrame* fileInfoLine = new QFrame( groupFileInfo );
  fileInfoLine->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  groupFileInfoLayout->addWidget( m_labelMimeType, 0, 0 );
  groupFileInfoLayout->addMultiCellWidget( m_displayFileName, 0, 1, 1, 2 );
  groupFileInfoLayout->addMultiCellWidget( fileInfoLine, 2, 2, 0, 2 );
  groupFileInfoLayout->addWidget( labelLength, 3, 0 );
  groupFileInfoLayout->addWidget( labelSize, 4, 0 );
  groupFileInfoLayout->addWidget( m_displayLength, 3, 2 );
  groupFileInfoLayout->addWidget( m_displaySize, 4, 2 );


  groupFileInfoLayout->setRowStretch( 5, 1 );


  QFont f( m_displayLength->font() );
  f.setBold( true );
  m_displayLength->setFont( f );
  m_displaySize->setFont( f );
  // /////////////////////////////////////////////////
  // /////////////////////////////////////////////////


  mainTabbed->addTab( cdTextTab, i18n("CD-Text") );
  mainTabbed->addTab( optionsTab, i18n("Options") );


  mainLayout->addWidget( groupFileInfo, 0, 0 );
  mainLayout->addWidget( mainTabbed, 0, 1 );

  mainLayout->setColStretch( 0, 1 );

  m_checkCopy->setTristate();
  m_checkPreEmp->setTristate();
}

void K3bAudioTrackDialog::setupConnections()
{
  connect( m_comboPregapFormat, SIGNAL(activated(const QString&)), 
	   this, SLOT(slotChangePregapFormat(const QString&)) );
}

void K3bAudioTrackDialog::slotChangePregapFormat( const QString& str )
{
  if( str == i18n( "Seconds" ) ) {
    if( !m_bPregapSeconds ) {
      m_bPregapSeconds = true;
      m_inputPregap->setValue( m_inputPregap->value() / 75 );
    }
  }
  else {
    if( m_bPregapSeconds ) {
      m_bPregapSeconds = false;
      m_inputPregap->setValue( m_inputPregap->value() * 75 );
    }
  }
}


#include "k3baudiotrackdialog.moc"
