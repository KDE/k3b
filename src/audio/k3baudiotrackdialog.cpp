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

#include <kiconloader.h>
#include <klocale.h>
#include <knuminput.h>
#include <kmimetype.h>
#include <kurl.h>

#include "k3baudiotrackdialog.h"
#include "k3baudiotrack.h"
#include "../kcutlabel.h"
#include "../k3bglobals.h"


K3bAudioTrackDialog::K3bAudioTrackDialog( QList<K3bAudioTrack>& tracks, QWidget *parent, const char *name )
  : KDialogBase( KDialogBase::Plain, i18n("Audio track"), KDialogBase::Ok|KDialogBase::Cancel|KDialogBase::Apply,
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
    m_displaySize->setText( QString("%1 kb").arg(allTrackSize / 1024) );

    m_labelMimeType->setPixmap( KMimeType::pixmapForURL( KURL(m_tracks.first()->absPath()) ) );
  }
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


void K3bAudioTrackDialog::slotCancel()
{
  done(0);
}


void K3bAudioTrackDialog::setupGui()
{
  QFrame* frame = plainPage();

  QGridLayout* mainLayout = new QGridLayout( frame );
  mainLayout->setSpacing( spacingHint() );
  mainLayout->setMargin( marginHint() );

  QGroupBox* _groupCdText = new QGroupBox( frame, "_groupCdText" );
  _groupCdText->setTitle( i18n( "CD-Text" ) );
  _groupCdText->setColumnLayout(0, Qt::Vertical );
  _groupCdText->layout()->setSpacing( 0 );
  _groupCdText->layout()->setMargin( 0 );
  QGridLayout* _groupCdTextLayout = new QGridLayout( _groupCdText->layout() );
  _groupCdTextLayout->setAlignment( Qt::AlignTop );
  _groupCdTextLayout->setSpacing( spacingHint() );
  _groupCdTextLayout->setMargin( marginHint() );

  QLabel* _labelPerformer = new QLabel( _groupCdText, "_labelPerformer" );
  _labelPerformer->setText( i18n( "&Performer" ) );

  _groupCdTextLayout->addWidget( _labelPerformer, 1, 0 );

  m_editPerformer = new QLineEdit( _groupCdText, "m_editPerformer" );
  m_editPerformer->setMinimumWidth( 100 );
	
  _groupCdTextLayout->addWidget( m_editPerformer, 1, 1 );

  m_editTitle = new QLineEdit( _groupCdText, "m_editTitle" );

  _groupCdTextLayout->addWidget( m_editTitle, 0, 1 );

  QLabel* _labelTitle = new QLabel( _groupCdText, "_labelTitle" );
  _labelTitle->setText( i18n( "&Title" ) );

  _groupCdTextLayout->addWidget( _labelTitle, 0, 0 );

  m_editMessage = new QMultiLineEdit( _groupCdText, "m_editMessage" );
  m_editMessage->setWordWrap( QMultiLineEdit::WidgetWidth );

  _groupCdTextLayout->addMultiCellWidget( m_editMessage, 6, 7, 1, 1 );

  QLabel* _labelMessage = new QLabel( _groupCdText, "_labelMessage" );
  _labelMessage->setText( i18n( "&Message" ) );
  _labelMessage->setAlignment( Qt::AlignLeft | Qt::AlignTop );

  _groupCdTextLayout->addWidget( _labelMessage, 6, 0 );

  m_editArranger = new QLineEdit( _groupCdText, "m_editArranger" );

  _groupCdTextLayout->addWidget( m_editArranger, 3, 1 );

  QLabel* _labelIsrc = new QLabel( _groupCdText, "_labelIsrc" );
  _labelIsrc->setText( i18n( "&ISRC" ) );

  _groupCdTextLayout->addWidget( _labelIsrc, 5, 0 );
  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
  _groupCdTextLayout->addItem( spacer, 7, 0 );

  QLabel* _labelSongwriter = new QLabel( _groupCdText, "_labelSongwriter" );
  _labelSongwriter->setText( i18n( "&Songwriter" ) );

  _groupCdTextLayout->addWidget( _labelSongwriter, 4, 0 );

  m_editSongwriter = new QLineEdit( _groupCdText, "m_editSongwriter" );

  _groupCdTextLayout->addWidget( m_editSongwriter, 4, 1 );

  m_editIsrc = new QLineEdit( _groupCdText, "m_editIsrc" );

  _groupCdTextLayout->addWidget( m_editIsrc, 5, 1 );

  QLabel* _labelArranger = new QLabel( _groupCdText, "_labelArranger" );
  _labelArranger->setText( i18n( "&Arranger" ) );

  _groupCdTextLayout->addWidget( _labelArranger, 3, 0 );

  QFrame* _line1 = new QFrame( _groupCdText, "_line1" );
  _line1->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  _groupCdTextLayout->addMultiCellWidget( _line1, 2, 2, 0, 1 );

  QGroupBox* _groupFileInfo = new QGroupBox( frame, "_groupFileInfo" );
  _groupFileInfo->setTitle( i18n( "File Info" ) );
  _groupFileInfo->setColumnLayout(0, Qt::Vertical );
  _groupFileInfo->layout()->setSpacing( 0 );
  _groupFileInfo->layout()->setMargin( 0 );
  QGridLayout* _groupFileInfoLayout = new QGridLayout( _groupFileInfo->layout() );
  _groupFileInfoLayout->setAlignment( Qt::AlignTop );
  _groupFileInfoLayout->setSpacing( spacingHint() );
  _groupFileInfoLayout->setMargin( marginHint() );

  m_labelMimeType = new QLabel( _groupFileInfo, "m_labelMimeType" );
  m_labelMimeType->setScaledContents( FALSE );

  _groupFileInfoLayout->addWidget( m_labelMimeType, 0, 0 );

  m_displayFileName = new KCutLabel( _groupFileInfo );
  m_displayFileName->setText( i18n( "filename" ) );
  m_displayFileName->setAlignment( int( QLabel::AlignTop | QLabel::AlignLeft ) );

  _groupFileInfoLayout->addWidget( m_displayFileName, 0, 1 );
  QSpacerItem* spacer_3 = new QSpacerItem( 2, 2 );
  _groupFileInfoLayout->addMultiCell( spacer_3, 1, 2, 0, 0 );

  QGridLayout* Layout1 = new QGridLayout;
  Layout1->setSpacing( spacingHint() );
  Layout1->setMargin( 0 );


  QLabel* _labelSize = new QLabel( _groupFileInfo, "_labelSize" );
  _labelSize->setText( i18n( "Size" ) );

  Layout1->addWidget( _labelSize, 1, 1 );

  QLabel* _labelLength = new QLabel( _groupFileInfo, "_labelLength" );
  _labelLength->setText( i18n( "Length     " ) ); // little HACK to maintain a minimum distance to the next label!!

  Layout1->addWidget( _labelLength, 0, 1 );

  m_displaySize = new QLabel( _groupFileInfo, "m_displaySize" );
  m_displaySize->setText( i18n( "0.0 MB" ) );
  m_displaySize->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

  Layout1->addWidget( m_displaySize, 1, 2 );

  m_displayLength = new QLabel( _groupFileInfo, "m_displayLength" );
  m_displayLength->setText( i18n( "0:0:0" ) );
  m_displayLength->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

  Layout1->addWidget( m_displayLength, 0, 2 );

  _groupFileInfoLayout->addMultiCellLayout( Layout1, 1, 1, 1, 2 );
  QSpacerItem* spacer_4 = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
  _groupFileInfoLayout->addItem( spacer_4, 2, 2 );

  QGroupBox* _groupOptions = new QGroupBox( frame, "_groupOptions" );
  _groupOptions->setTitle( i18n( "Options" ) );
  _groupOptions->setColumnLayout(0, Qt::Vertical );
  _groupOptions->layout()->setSpacing( 0 );
  _groupOptions->layout()->setMargin( 0 );
  QGridLayout* _groupOptionsLayout = new QGridLayout( _groupOptions->layout() );
  _groupOptionsLayout->setAlignment( Qt::AlignTop );
  _groupOptionsLayout->setSpacing( spacingHint() );
  _groupOptionsLayout->setMargin( marginHint() );

  QLabel* _labelPregap = new QLabel( _groupOptions, "_labelPregap" );
  _labelPregap->setText( i18n( "Pre&gap" ) );

  _groupOptionsLayout->addWidget( _labelPregap, 0, 0 );
  QSpacerItem* spacer_6 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  _groupOptionsLayout->addItem( spacer_6, 1, 1 );
  QSpacerItem* spacer_7 = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
  _groupOptionsLayout->addItem( spacer_7, 2, 1 );

  m_inputPregap = new KIntNumInput( _groupOptions, "m_inputPregap" );

  _groupOptionsLayout->addWidget( m_inputPregap, 1, 0 );

  m_comboPregapFormat = new QComboBox( _groupOptions );
  m_comboPregapFormat->insertItem( i18n("Seconds" ) );
  m_comboPregapFormat->insertItem( i18n("Frames" ) );

  _groupOptionsLayout->addWidget( m_comboPregapFormat, 1, 1 );

  m_checkPreEmp = new QCheckBox( _groupOptions, "m_checkPreEmp" );
  m_checkPreEmp->setText( i18n( "Preemphasis" ) );

  _groupOptionsLayout->addWidget( m_checkPreEmp, 0, 3 );

  m_checkCopy = new QCheckBox( _groupOptions, "m_checkCopy" );
  m_checkCopy->setText( i18n( "Copy protected" ) );

  _groupOptionsLayout->addWidget( m_checkCopy, 1, 3 );

  // tab order
  setTabOrder( m_editTitle, m_editPerformer );
  setTabOrder( m_editPerformer, m_editArranger );
  setTabOrder( m_editArranger, m_editSongwriter );
  setTabOrder( m_editSongwriter, m_editIsrc );
  setTabOrder( m_editIsrc, m_editMessage );

  // buddies
  _labelPerformer->setBuddy( m_editPerformer );
  _labelTitle->setBuddy( m_editTitle );
  _labelMessage->setBuddy( m_editMessage );
  _labelIsrc->setBuddy( m_editIsrc );
  _labelSongwriter->setBuddy( m_editSongwriter );
  _labelArranger->setBuddy( m_editArranger );
  _labelPregap->setBuddy( m_inputPregap );

  Layout1->setColStretch( 0, 1 );

  mainLayout->addMultiCellWidget( _groupCdText, 0, 1, 1, 1 );
  mainLayout->addWidget( _groupFileInfo, 0, 0 );
  mainLayout->addWidget( _groupOptions, 1, 0 );

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
