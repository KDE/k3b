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
#include "audio/k3baudiojob.h"

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
#include <kmessagebox.h>


K3bBurnProgressDialog::K3bBurnProgressDialog( QWidget *parent, const char *name )
  : KDialog(parent,name, true)
{
  setupGUI();
  setupConnections();
  	
  m_groupBuffer->setEnabled( false );

  m_job = 0;
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
  m_groupInfo->setTitle( i18n( "Information" ) );
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
  m_buttonCancel->setText( i18n( "Cancel" ) );
  m_buttonClose = new QPushButton( this, "m_buttonClose" );
  m_buttonClose->setText( i18n( "Close" ) );

  mainLayout->addWidget( m_buttonCancel, 3, 1 );
  mainLayout->addWidget( m_buttonClose, 3, 1 );
 	
  m_buttonClose->hide();

  m_groupBuffer = new QGroupBox( this, "m_groupBuffer" );
  m_groupBuffer->setTitle( i18n( "Buffer Status" ) );
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
  m_groupProgress->setTitle( i18n( "Progress" ) );
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

  m_groupProgressLayout->addWidget( m_labelFileName, 0, 0 );

  m_labelTrackProgress = new QLabel( m_groupProgress, "m_labelTrackProgress" );
  m_labelTrackProgress->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

  m_groupProgressLayout->addWidget( m_labelTrackProgress, 0, 1 );

  m_labelCdTime = new QLabel( m_groupProgress, "m_labelCdTime" );

  m_groupProgressLayout->addWidget( m_labelCdTime, 3, 0 );

  m_labelCdProgress = new QLabel( m_groupProgress, "m_labelCdProgress" );
  m_labelCdProgress->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

  m_groupProgressLayout->addWidget( m_labelCdProgress, 3, 1 );

  mainLayout->addMultiCellWidget( m_groupProgress, 1, 1, 0, 2 );
}


void K3bBurnProgressDialog::setupConnections()
{
	connect( m_buttonCancel, SIGNAL(pressed()), this, SLOT(slotCancelPressed()) );
	connect( m_buttonClose, SIGNAL(clicked()), this, SLOT(close()) );
}


void K3bBurnProgressDialog::updateCdSizeProgress( int processed, int size )
{
	m_labelCdProgress->setText( i18n("%1 of %2 written").arg( processed ).arg( size ) );
}

//void K3bBurnProgressDialog::updateCdTimeProgress( const QTime& processedMin )
//{
//  QString str = processedMin.toString();
//  str += " / ";
//  str += ((K3bAudioDoc*)doc)->audioSize().toString();
//  m_labelCdTime->setText( str );
//}

void K3bBurnProgressDialog::updateTrackSizeProgress( int processedTrackSize, int trackSize )
{
   	m_labelTrackProgress->setText( i18n("%1 of %2 processed").arg(processedTrackSize).arg(trackSize) );
}

//void K3bBurnProgressDialog::updateTrackTimeProgress( const QTime& processedTrackTime )
//{
//
//}


void K3bBurnProgressDialog::displayInfo( const QString& infoString )
{
  m_viewInfo->append( infoString + "\n" );
}


void K3bBurnProgressDialog::finished()
{
  qDebug( "(K3bBurnProgressDialog) received finished signal!");

  m_labelFileName->setText("Writing finished");
  m_labelTrackProgress->setText("");
//  m_groupBuffer->setEnabled( false );

  m_buttonCancel->hide();
  m_buttonClose->show();
}


void K3bBurnProgressDialog::setJob( K3bJob* job )
{
	// clear everything
	m_buttonClose->hide();
	m_buttonCancel->show();
	m_viewInfo->setText("");
	m_progressBuffer->setValue(0);
	m_progressTrack->setValue(0);
	m_progressCd->setValue(0);
	m_labelFileName->setText("");
	m_groupProgress->setTitle( i18n( "Progress" ) );

	// disconnect from the former job
	if( m_job )
		disconnect( m_job );
	m_job = job;
	
	// connect to all the shit
	connect( job, SIGNAL(infoMessage(const QString&)), this, SLOT(displayInfo(const QString&)) );
	
	connect( job, SIGNAL(percent(int)), m_progressCd, SLOT(setValue(int)) );
	connect( job, SIGNAL(subPercent(int)), m_progressTrack, SLOT(setValue(int)) );

	connect( job, SIGNAL(processedSubSize(int, int)), this, SLOT(updateTrackSizeProgress(int, int)) );
	connect( job, SIGNAL(processedSize(int, int)), this, SLOT(updateCdSizeProgress(int, int)) );

	connect( job, SIGNAL(newTask(const QString&)), m_groupProgress, SLOT(setTitle(const QString&)) );
	connect( job, SIGNAL(newSubTask(const QString&)), this, SLOT(slotNewSubJob(const QString&)) );
	connect( job, SIGNAL(finished(K3bJob*)), this, SLOT(finished()) );
	

	if( K3bAudioJob* ajob = dynamic_cast<K3bAudioJob*>( job ) )
	{
		if( ajob->doc()->burner() )
    		m_labelWriter->setText( "Writer: " + ajob->doc()->burner()->vendor + " " + ajob->doc()->burner()->description );

    	// connect to the "special" signals
		connect( ajob, SIGNAL(bufferStatus(int)), m_progressBuffer, SLOT(setValue(int)) );
		
		m_groupBuffer->setEnabled( true ); 	
	}
}


void K3bBurnProgressDialog::slotCancelPressed()
{
	if( m_job )
		if( KMessageBox::questionYesNo( this, "Do you really want to cancel?", "Cancel" ) == KMessageBox::Yes ) {
			m_job->cancel();
			m_buttonCancel->hide();
			m_buttonClose->show();
		}
}

void K3bBurnProgressDialog::show()
{
	QWidget::show();
}


void K3bBurnProgressDialog::slotNewSubJob(const QString& name)
{
	m_labelFileName->setText(name);
	m_labelTrackProgress->setText("");
	m_progressTrack->setValue(0);
}
