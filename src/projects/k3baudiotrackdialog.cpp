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


#include <qtextedit.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qframe.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qhbox.h>
#include <qtabwidget.h>
#include <qwhatsthis.h>

#include <kiconloader.h>
#include <klocale.h>
#include <knuminput.h>
#include <kmimetype.h>
#include <kurl.h>
#include <kio/global.h>
#include <klineedit.h>

#include "k3baudiotrackdialog.h"
#include "k3baudioeditorwidget.h"
#include "k3baudiotrackwidget.h"
#include "k3baudiotrack.h"
#include <k3bvalidators.h>
#include <kcutlabel.h>
#include <k3bmsf.h>
#include <k3bcdtextvalidator.h>
#include <k3baudiodecoder.h>
#include <k3bmsfedit.h>


// TODO: three modes:
//    1. Only one track with only one source
//         show decoder tech info, cdtext, options and the track editor without showing anything
//         about sources
//    2. Only one track with multible sources
//         like the above but with the possiblity to edit the sources
//    3. multible tracks
//         do only show cd-text and options (eventuelle index0)


K3bAudioTrackDialog::K3bAudioTrackDialog( QPtrList<K3bAudioTrack>& tracks, QWidget *parent, const char *name )
  : KDialogBase( KDialogBase::Plain, i18n("Audio Track Properties"), 
		 KDialogBase::Ok|KDialogBase::Cancel|KDialogBase::Apply,
		 KDialogBase::Ok, parent, name )
{
  m_tracks = tracks;

  setupGui();
  setupConnections();
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
  m_audioTrackWidget->save();
}


void K3bAudioTrackDialog::setupGui()
{
  QFrame* frame = plainPage();

  QGridLayout* mainLayout = new QGridLayout( frame );
  mainLayout->setSpacing( spacingHint() );
  mainLayout->setMargin( 0 );

  QTabWidget* mainTabbed = new QTabWidget( frame );
  m_audioTrackWidget = new K3bAudioTrackWidget( m_tracks, mainTabbed );
  mainTabbed->addTab( m_audioTrackWidget, i18n("Track Properties") );
  mainLayout->addWidget( mainTabbed, 0, 0 );

//   // /////////////////////////////////////////////////
//   // OPTIONS TAB
//   // /////////////////////////////////////////////////
//   // /////////////////////////////////////////////////
//   QWidget* optionsTab = new QWidget( mainTabbed );
//   QGridLayout* optionsGrid = new QGridLayout( optionsTab );
//   optionsGrid->setSpacing( spacingHint() );
//   optionsGrid->setMargin( marginHint() );

//   QLabel* labelPregap = new QLabel( i18n("&Pregap:"), optionsTab );
//   m_inputPregap       = new K3bMsfEdit( optionsTab, "m_inputPregap" );
//   labelPregap->setBuddy( m_inputPregap );

//   m_checkPreEmp       = new QCheckBox( i18n( "Pr&eemphasis" ), optionsTab, "m_checkPreEmp" );
//   m_checkCopy         = new QCheckBox( i18n( "&Copy protected" ), optionsTab, "m_checkCopy" );


//   optionsGrid->addWidget( labelPregap, 0, 0 );
//   optionsGrid->addWidget( m_inputPregap, 0, 1 );
//   optionsGrid->addMultiCellWidget( m_checkPreEmp, 1, 1, 0, 1 );
//   optionsGrid->addMultiCellWidget( m_checkCopy, 2, 2, 0, 1 );

//   optionsGrid->setRowStretch( 3, 1 );
//   // /////////////////////////////////////////////////
//   // /////////////////////////////////////////////////



//   // /////////////////////////////////////////////////
//   // CD TEXT TAB
//   // /////////////////////////////////////////////////
//   // /////////////////////////////////////////////////
//   QWidget* cdTextTab = new QWidget( mainTabbed );
//   QGridLayout* cdTextTabLayout = new QGridLayout( cdTextTab );
//   cdTextTabLayout->setAlignment( Qt::AlignTop );
//   cdTextTabLayout->setSpacing( spacingHint() );
//   cdTextTabLayout->setMargin( marginHint() );

//   QLabel* labelMessage    = new QLabel( i18n( "&Message:" ), cdTextTab, "labelMessage" );
//   QLabel* labelPerformer  = new QLabel( i18n( "&Performer:" ), cdTextTab, "labelPerformer" );
//   QLabel* labelTitle      = new QLabel( i18n( "&Title:" ), cdTextTab, "labelTitle" );
//   QLabel* labelIsrc       = new QLabel( i18n( "&ISRC:" ), cdTextTab, "labelIsrc" );
//   QLabel* labelSongwriter = new QLabel( i18n( "&Songwriter:" ), cdTextTab, "labelSongwriter" );
//   QLabel* labelComposer   = new QLabel( i18n( "&Composer:" ), cdTextTab, "labelComposer" );
//   QLabel* labelArranger   = new QLabel( i18n( "&Arranger:" ), cdTextTab, "labelArranger" );

//   labelMessage->setAlignment( Qt::AlignLeft | Qt::AlignTop );

//   m_editPerformer  = new QLineEdit( cdTextTab, "m_editPerformer" );
//   m_editTitle      = new QLineEdit( cdTextTab, "m_editTitle" );
//   m_editMessage    = new QTextEdit( cdTextTab, "m_editMessage" );
//   m_editArranger   = new QLineEdit( cdTextTab, "m_editArranger" );
//   m_editSongwriter = new QLineEdit( cdTextTab, "m_editSongwriter" );
//   m_editComposer = new QLineEdit( cdTextTab, "m_editComposer" );
//   m_editIsrc       = new QLineEdit( cdTextTab, "m_editIsrc" );
//   QFrame* line1    = new QFrame( cdTextTab, "_line1" );

//   //  m_editPerformer->setMinimumWidth( 100 );
//   m_editMessage->setWordWrap( QTextEdit::WidgetWidth );
//   line1->setFrameStyle( QFrame::HLine | QFrame::Sunken );
//   m_editIsrc->setValidator( K3bValidators::isrcValidator( m_editIsrc ) );
//   QValidator* cdTextVal = new K3bCdTextValidator( this );
//   m_editPerformer->setValidator( cdTextVal );
//   m_editTitle->setValidator( cdTextVal );
//   //  m_editMessage->setValidator( cdTextVal );
//   m_editArranger->setValidator( cdTextVal );
//   m_editSongwriter->setValidator( cdTextVal );
//   m_editComposer->setValidator( cdTextVal );


//   cdTextTabLayout->addWidget( labelPerformer, 1, 0 );
//   cdTextTabLayout->addWidget( m_editPerformer, 1, 1 );
//   cdTextTabLayout->addWidget( labelTitle, 0, 0 );
//   cdTextTabLayout->addWidget( m_editTitle, 0, 1 );
//   cdTextTabLayout->addMultiCellWidget( line1, 2, 2, 0, 1 );
//   cdTextTabLayout->addWidget( labelArranger, 3, 0 );
//   cdTextTabLayout->addWidget( m_editArranger, 3, 1 );
//   cdTextTabLayout->addWidget( labelSongwriter, 4, 0 );
//   cdTextTabLayout->addWidget( m_editSongwriter, 4, 1 );
//   cdTextTabLayout->addWidget( labelComposer, 5, 0 );
//   cdTextTabLayout->addWidget( m_editComposer, 5, 1 );
//   cdTextTabLayout->addWidget( labelIsrc, 6, 0 );
//   cdTextTabLayout->addWidget( m_editIsrc, 6, 1 );
//   cdTextTabLayout->addWidget( labelMessage, 7, 0 );
//   cdTextTabLayout->addWidget( m_editMessage, 7, 1 );

//   cdTextTabLayout->setRowStretch( 7, 1 );


//   // buddies
//   labelPerformer->setBuddy( m_editPerformer );
//   labelTitle->setBuddy( m_editTitle );
//   labelMessage->setBuddy( m_editMessage );
//   labelIsrc->setBuddy( m_editIsrc );
//   labelSongwriter->setBuddy( m_editSongwriter );
//   labelComposer->setBuddy( m_editComposer );
//   labelArranger->setBuddy( m_editArranger );

//   // tab order
//   setTabOrder( m_editTitle, m_editPerformer );
//   setTabOrder( m_editPerformer, m_editArranger );
//   setTabOrder( m_editArranger, m_editSongwriter );
//   setTabOrder( m_editSongwriter, m_editComposer );
//   setTabOrder( m_editComposer, m_editIsrc );
//   setTabOrder( m_editIsrc, m_editMessage );

//   // /////////////////////////////////////////////////
//   // /////////////////////////////////////////////////


//   // /////////////////////////////////////////////////
//   // FILE-INFO BOX
//   // /////////////////////////////////////////////////
//   // /////////////////////////////////////////////////

//   QGroupBox* groupFileInfo = new QGroupBox( 0, Qt::Vertical, i18n( "File Info" ), frame, "groupFileInfo" );
//   groupFileInfo->layout()->setSpacing( 0 );
//   groupFileInfo->layout()->setMargin( 0 );
//   QGridLayout* groupFileInfoLayout = new QGridLayout( groupFileInfo->layout() );
//   groupFileInfoLayout->setAlignment( Qt::AlignTop );
//   groupFileInfoLayout->setSpacing( spacingHint() );
//   groupFileInfoLayout->setMargin( marginHint() );

//   m_labelMimeType = new QLabel( groupFileInfo, "m_labelMimeType" );
//   m_displayFileName = new KCutLabel( groupFileInfo );
//   m_displayFileName->setText( i18n( "Filename" ) );
//   m_displayFileName->setAlignment( int( QLabel::AlignTop | QLabel::AlignLeft ) );
//   QLabel* labelSize = new QLabel( i18n( "Size:" ), groupFileInfo, "labelSize" );
//   QLabel* labelLength = new QLabel( i18n( "Length:"), groupFileInfo, "labelLength" );
//   m_displaySize = new QLabel( groupFileInfo, "m_displaySize" );
//   m_displaySize->setText( "0.0 MB" );
//   m_displaySize->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );
//   m_displayLength = new QLabel( groupFileInfo, "m_displayLength" );
//   m_displayLength->setText( "0:0:0" );
//   m_displayLength->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

//   QFrame* fileInfoLine = new QFrame( groupFileInfo );
//   fileInfoLine->setFrameStyle( QFrame::HLine | QFrame::Sunken );

//   QGridLayout* filenameLayout = new QGridLayout;
//   filenameLayout->addWidget( m_labelMimeType, 0, 0 );
//   filenameLayout->addMultiCellWidget( m_displayFileName, 0, 1, 1, 1 );
//   filenameLayout->setRowStretch( 1, 1 );
//   filenameLayout->setColStretch( 1, 1 );
//   groupFileInfoLayout->addMultiCellLayout( filenameLayout, 0, 0, 0, 1 );
//   groupFileInfoLayout->addMultiCellWidget( fileInfoLine, 2, 2, 0, 1 );
//   groupFileInfoLayout->addWidget( labelLength, 3, 0 );
//   groupFileInfoLayout->addWidget( labelSize, 4, 0 );
//   groupFileInfoLayout->addWidget( m_displayLength, 3, 1 );
//   groupFileInfoLayout->addWidget( m_displaySize, 4, 1 );

//   QFont f( m_displayLength->font() );
//   f.setBold( true );
//   m_displayLength->setFont( f );
//   m_displaySize->setFont( f );

//   // technical info
//   int row = 5;
//   if( m_tracks.count() == 1 ) {
//     K3bAudioDecoder* dec = m_tracks.first()->module();
    
//     QStringList infos = dec->supportedTechnicalInfos();
//     if( !infos.isEmpty() ) {
//       for( QStringList::iterator it = infos.begin(); it != infos.end(); ++it ) {
// 	QLabel* l1 = new QLabel( *it + ":", groupFileInfo );
// 	QLabel* l2 = new QLabel( dec->technicalInfo( *it ), groupFileInfo );
// 	l2->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );
// 	l2->setFont( f );
// 	groupFileInfoLayout->addWidget( l1, row, 0 );
// 	groupFileInfoLayout->addWidget( l2, row, 1 );
// 	++row;
//       }
//     }
//   }

//   groupFileInfoLayout->setRowStretch( row, 1 );
//   groupFileInfoLayout->setColStretch( 1, 1 );

//   // /////////////////////////////////////////////////
//   // /////////////////////////////////////////////////


//   mainTabbed->addTab( cdTextTab, i18n("CD-Text") );
//   mainTabbed->addTab( optionsTab, i18n("Options") );

//   // track length offset stuff
//   if( m_tracks.count() == 1 ) {
//     QWidget* tab = new QWidget( mainTabbed );
//     QGridLayout* tabLayout = new QGridLayout( tab );
//     tabLayout->setAlignment( Qt::AlignTop );
//     tabLayout->setSpacing( spacingHint() );
//     tabLayout->setMargin( marginHint() );
    
//     m_editTrackStart = new K3bMsfEdit( tab );
//     m_editTrackEnd = new K3bMsfEdit( tab );

//     QLabel* labelStart = new QLabel( i18n("Track start"), tab );
//     QLabel* labelEnd = new QLabel( i18n("Track end"), tab );
//     labelEnd->setAlignment( QLabel::AlignVCenter | QLabel::AlignRight );

//     tabLayout->addWidget( labelStart, 0, 0 );
//     tabLayout->addWidget( labelEnd, 0, 1 );
//     tabLayout->addWidget( m_editTrackStart, 1, 0 );
//     tabLayout->addWidget( m_editTrackEnd, 1, 1 );

//     mainTabbed->addTab( tab, i18n("Edit Track") );

//     m_editTrackStart->setMsfValue( m_tracks.first()->trackStart() );
//     m_editTrackEnd->setMsfValue( m_tracks.first()->trackEnd() );

//     connect( m_editTrackStart, SIGNAL(valueChanged(int)),
// 	     this, SLOT(slotTrackStartChanged(int)) );
//     connect( m_editTrackEnd, SIGNAL(valueChanged(int)),
// 	     this, SLOT(slotTrackEndChanged(int)) );

//     // set the track editor ranges
//     slotTrackStartChanged( m_tracks.first()->trackStart().totalFrames() );
//     slotTrackEndChanged( m_tracks.first()->trackEnd().totalFrames() );
//   }


//   mainLayout->addWidget( groupFileInfo, 0, 0 );
//   mainLayout->addWidget( mainTabbed, 0, 1 );

//   mainLayout->setColStretch( 0, 1 );

//   m_checkCopy->setTristate();
//   m_checkPreEmp->setTristate();
}

void K3bAudioTrackDialog::setupConnections()
{
}


void K3bAudioTrackDialog::updateTrackLengthDisplay()
{
//   K3b::Msf len = m_editTrackEnd->msfValue() - m_editTrackStart->msfValue();
//   m_displayLength->setText( len.toString() );
//   m_displaySize->setText( KIO::convertSize(len.audioBytes()) );
}



#include "k3baudiotrackdialog.moc"
