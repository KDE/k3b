/***************************************************************************
                          k3baudioburninfodialog.cpp  -  description
                             -------------------
    begin                : Thu Apr 5 2001
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

#include "k3bburnprogressdialog.h"
#include "audio/k3baudiodoc.h"
#include "audio/k3baudiotrack.h"

#include <qgroupbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qtextview.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qdatetime.h>
#include <qstring.h>

#include <kprogress.h>
#include <klocale.h>


K3bBurnProgressDialog::K3bBurnProgressDialog( K3bDoc* _doc, QWidget *parent, const char *name )
  : KDialog(parent,name, true)
{
  doc = _doc;
  setupGUI();
  setupConnections();
	
  if( doc->burner() )
    m_labelWriter->setText( "Writer: " + doc->burner()->vendor + " " + doc->burner()->description );
		
  m_groupBuffer->setEnabled( false );

  alreadyWrittenMb = 0;
  alreadyWrittenTrackMb = 0;
}

K3bBurnProgressDialog::~K3bBurnProgressDialog()
{
}


void K3bBurnProgressDialog::setupGUI()
{
  mainLayout = new QGridLayout( this );
  mainLayout->setSpacing( spacingHint() );
  mainLayout->setMargin( marginHint() );

  m_groupInfo = new QGroupBox( this, "m_groupInfo" );
  m_groupInfo->setTitle( tr( "Information" ) );
  m_groupInfo->setColumnLayout(0, Qt::Vertical );
  m_groupInfo->layout()->setSpacing( 0 );
  m_groupInfo->layout()->setMargin( 0 );
  m_groupInfoLayout = new QHBoxLayout( m_groupInfo->layout() );
  m_groupInfoLayout->setAlignment( Qt::AlignTop );
  m_groupInfoLayout->setSpacing( spacingHint() );
  m_groupInfoLayout->setMargin( marginHint() );

  m_viewInfo = new QTextView( m_groupInfo, "m_viewInfo" );
  m_viewInfo->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)7, m_viewInfo->sizePolicy().hasHeightForWidth() ) );
  m_viewInfo->setMinimumSize( QSize( 500, 0 ) );
  m_groupInfoLayout->addWidget( m_viewInfo );

  mainLayout->addMultiCellWidget( m_groupInfo, 0, 0, 0, 2 );

  m_buttonCancel = new QPushButton( this, "m_buttonCancel" );
  m_buttonCancel->setText( tr( "Cancel" ) );
  m_buttonOk = new QPushButton( this, "m_buttonOk" );
  m_buttonOk->setText( tr( "OK" ) );

  mainLayout->addWidget( m_buttonCancel, 3, 1 );
  mainLayout->addWidget( m_buttonOk, 3, 1 );
 	
  m_buttonOk->hide();

  m_groupBuffer = new QGroupBox( this, "m_groupBuffer" );
  m_groupBuffer->setTitle( tr( "Buffer Status" ) );
  m_groupBuffer->setColumnLayout(0, Qt::Vertical );
  m_groupBuffer->layout()->setSpacing( 0 );
  m_groupBuffer->layout()->setMargin( 0 );
  m_groupBufferLayout = new QHBoxLayout( m_groupBuffer->layout() );
  m_groupBufferLayout->setAlignment( Qt::AlignTop );
  m_groupBufferLayout->setSpacing( spacingHint() );
  m_groupBufferLayout->setMargin( marginHint() );

  m_labelWriter = new QLabel( "WRITER", m_groupBuffer );
  m_progressBuffer = new KProgress( 0, 100, 0, Qt::Horizontal, m_groupBuffer, "m_progressBuffer" );
  m_progressBuffer->setMaximumWidth( 150 );

  m_groupBufferLayout->addWidget( m_labelWriter );
  m_groupBufferLayout->addWidget( m_progressBuffer );

  mainLayout->addMultiCellWidget( m_groupBuffer, 2, 2, 0, 2 );
  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  mainLayout->addItem( spacer, 3, 0 );
  QSpacerItem* spacer_2 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  mainLayout->addItem( spacer_2, 3, 2 );

  m_groupProgress = new QGroupBox( this, "m_groupProgress" );
  m_groupProgress->setTitle( tr( "Progress" ) );
  m_groupProgress->setColumnLayout(0, Qt::Vertical );
  m_groupProgress->layout()->setSpacing( 0 );
  m_groupProgress->layout()->setMargin( 0 );
  m_groupProgressLayout = new QGridLayout( m_groupProgress->layout() );
  m_groupProgressLayout->setAlignment( Qt::AlignTop );
  m_groupProgressLayout->setSpacing( spacingHint() );
  m_groupProgressLayout->setMargin( marginHint() );

  m_progressTrack = new KProgress( 0, 100, 0, Qt::Horizontal, m_groupProgress, "m_progressTrack" );

  m_groupProgressLayout->addMultiCellWidget( m_progressTrack, 1, 1, 0, 1 );

  m_progressCd = new KProgress( 0, 100, 0, Qt::Horizontal, m_groupProgress, "m_progressCd" );

  m_groupProgressLayout->addMultiCellWidget( m_progressCd, 4, 4, 0, 1 );

  m_labelFileName = new QLabel( m_groupProgress, "m_labelFileName" );
  m_labelFileName->setText( tr( "Writing Track 0 - " ) );

  m_groupProgressLayout->addWidget( m_labelFileName, 0, 0 );

  m_labelTrackProgress = new QLabel( m_groupProgress, "m_labelTrackProgress" );
  m_labelTrackProgress->setText( tr( "0 of 00 MB written" ) );
  m_labelTrackProgress->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

  m_groupProgressLayout->addWidget( m_labelTrackProgress, 0, 1 );

  m_labelCdTime = new QLabel( m_groupProgress, "m_labelCdTime" );
  m_labelCdTime->setText( tr( "00:00:00 / 00:00:00" ) );

  m_groupProgressLayout->addWidget( m_labelCdTime, 3, 0 );

  m_labelCdProgress = new QLabel( m_groupProgress, "m_labelCdProgress" );
  m_labelCdProgress->setText( tr( "0 of 0 MB written" ) );
  m_labelCdProgress->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

  m_groupProgressLayout->addWidget( m_labelCdProgress, 3, 1 );

  mainLayout->addMultiCellWidget( m_groupProgress, 1, 1, 0, 2 );
}


void K3bBurnProgressDialog::setupConnections()
{
  connect( m_buttonCancel, SIGNAL(pressed()), this, SIGNAL(cancelPressed()) );

  connect( doc, SIGNAL(startWriting()), this, SLOT(startWriting()) );
  connect( doc, SIGNAL(startDecoding()), this, SLOT(startDecoding()) );
	
  connect( doc, SIGNAL(infoMessage(const QString&)), this, SLOT(displayInfo(const QString&)) );
	
  connect( doc, SIGNAL(percent(int)), m_progressCd, SLOT(setValue(int)) );
  connect( doc, SIGNAL(trackPercent(int)), m_progressTrack, SLOT(setValue(int)) );
  connect( doc, SIGNAL(bufferStatus(int)), m_progressBuffer, SLOT(setValue(int)) );
	
//   connect( doc, SIGNAL(processedSize(unsigned long, unsigned long)), this, SLOT(updateCdSizeProgress(unsigned long, unsigned long)) );
  connect( doc, SIGNAL(processedMinutes(const QTime&)), this, SLOT(updateCdTimeProgress(const QTime&)) );
  connect( doc, SIGNAL(trackProcessedSize(int, int)), this, SLOT(updateTrackSizeProgress(int, int)) );
  //	connect( doc, SIGNAL(trackProcessedMinutes(const QTime&)), this, SLOT(updateTrackTimeProgress(const QTime&)) );

  connect( doc, SIGNAL(nextTrackProcessed()), this, SLOT(nextTrack()) );
  connect( doc, SIGNAL(result()), this, SLOT(finished()) );
}


// void K3bBurnProgressDialog::updateCdSizeProgress( unsigned long processed, unsigned long size )
// {
//   m_labelCdProgress->setText( i18n("%1 of %2 written").arg( processed ).arg( size ) );
// }

void K3bBurnProgressDialog::updateCdTimeProgress( const QTime& processedMin )
{
  QString str = processedMin.toString();
  str += " / ";
  str += ((K3bAudioDoc*)doc)->audioSize().toString();
  m_labelCdTime->setText( str );
}

void K3bBurnProgressDialog::updateTrackSizeProgress( int processedTrackSize, int trackSize )
{
  switch( currentAction ) {
  case DECODING:
    m_labelTrackProgress->setText( i18n("%1 of %2 frames processed").arg(processedTrackSize).arg(trackSize) );
    break;
  case WRITING_AUDIO:
  case WRITING_DATA:
    alreadyWrittenTrackMb = processedTrackSize;
    m_labelTrackProgress->setText( i18n("%1 of %2 MB written").arg(processedTrackSize).arg(trackSize) );
    m_labelCdProgress->setText( i18n("%1 of %2 written").arg( alreadyWrittenMb ).arg( doc->size() ) );
    break;
  }
}

//void K3bBurnProgressDialog::updateTrackTimeProgress( const QTime& processedTrackTime )
//{
//
//}


void K3bBurnProgressDialog::displayInfo( const QString& infoString )
{
  m_viewInfo->append( infoString + "\n" );
}


void K3bBurnProgressDialog::nextTrack()
{
  alreadyWrittenMb += alreadyWrittenTrackMb;

  K3bAudioTrack* _track = ((K3bAudioDoc*)doc)->currentProcessedTrack();
  if( _track ) {
    switch( currentAction ) {
    case DECODING:
      m_labelFileName->setText( QString("Decoding Track %1 - '%2'").arg( _track->index() +1 ).arg( _track->fileName() ) );
      break;
    case WRITING_AUDIO:
      m_labelFileName->setText( QString("Writing Track %1 - '%2'").arg( _track->index() +1 ).arg( _track->fileName() ) );
      break;
    }
  }
}


void K3bBurnProgressDialog::finished()
{
  m_labelFileName->setText("Writing finished");
  m_labelTrackProgress->setText("");
  m_groupBuffer->setEnabled( false );

  m_buttonCancel->hide();
  m_buttonOk->show();
	
  connect( m_buttonOk, SIGNAL(clicked()), this, SLOT(close()) );
}

void K3bBurnProgressDialog::startDecoding()
{
	// hide some unuseful things here
	m_labelCdProgress->hide();

	m_groupBuffer->setEnabled( false );
	currentAction = DECODING;
}


void K3bBurnProgressDialog::startWriting()
{
	m_labelCdProgress->show();
	m_progressTrack->setValue(0);
	m_labelFileName->setText( "Writing Pregap" );
	
	m_groupBuffer->setEnabled( true );
	
	if( K3bAudioDoc* a = dynamic_cast<K3bAudioDoc*>(doc) )
		currentAction = WRITING_AUDIO;
	else
		currentAction = WRITING_DATA;
}
