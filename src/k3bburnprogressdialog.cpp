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
#include "kcutlabel.h"
#include "device/k3bdevice.h"
#include "k3bjob.h"
#include "k3bdoc.h"

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
#include <qtextview.h>
#include <qhbox.h>

#include <kprogress.h>
#include <klocale.h>
#include <kmessagebox.h>




K3bBurnProgressDialog::PrivateDebugWidget::PrivateDebugWidget( QMap<QString, QStringList>& map, QWidget* parent )
  : KDialog( parent, "debugViewDialog", true )
{
  setCaption( "Debugging output" );

  QPushButton* okButton = new QPushButton( "OK", this );
  QTextView* debugView = new QTextView( this );
  QGridLayout* grid = new QGridLayout( this );
  grid->addMultiCellWidget( debugView, 0, 0, 0, 1 );
  grid->addWidget( okButton, 1, 1 );
  grid->setSpacing( spacingHint() );
  grid->setMargin( marginHint() );
  grid->setColStretch( 0, 1 );

  connect( okButton, SIGNAL(pressed()), this, SLOT(accept()) );

  // add the debugging output
  for( QMap<QString, QStringList>::Iterator itMap = map.begin(); itMap != map.end(); ++itMap ) {
    QStringList& list = itMap.data();
    debugView->append( itMap.key() + "\n" );
    debugView->append( "-----------------------\n" );
    for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it ) {
       QStringList lines = QStringList::split( "\n", *it );
       // do every line
       for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ )
	 debugView->append( *str + "\n" );
    }
    debugView->append( "\n" );
  }

  resize( 200, 300 );
}


K3bBurnProgressDialog::K3bBurnProgressDialog( QWidget *parent, const char *name )
  : KDialog(parent,name, true)
{
  setCaption( "Writing process" );

  setupGUI();
  setupConnections();
  	
  m_groupBuffer->setEnabled( false );

  m_job = 0;
  m_timer = new QTimer( this );

  connect( m_timer, SIGNAL(timeout()), this, SLOT(slotUpdateTime()) );
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

  mainLayout->addMultiCellWidget( m_groupInfo, 0, 0, 0, 3 );

  m_buttonCancel = new QPushButton( this, "m_buttonCancel" );
  m_buttonCancel->setText( i18n( "Cancel" ) );
  m_buttonClose = new QPushButton( this, "m_buttonClose" );
  m_buttonClose->setText( i18n( "Close" ) );
  m_buttonShowDebug = new QPushButton( i18n("Show Debugging Output"), this, "m_buttonShowDebug" );

  mainLayout->addWidget( m_buttonCancel, 3, 1 );
  mainLayout->addWidget( m_buttonClose, 3, 1 );
  mainLayout->addWidget( m_buttonShowDebug, 3, 2 );
 	
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

  mainLayout->addMultiCellWidget( m_groupBuffer, 2, 2, 0, 3 );
  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  mainLayout->addItem( spacer, 3, 0 );
  QSpacerItem* spacer_2 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  mainLayout->addItem( spacer_2, 3, 3 );

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

  m_labelFileName = new KCutLabel( m_groupProgress );

  m_groupProgressLayout->addWidget( m_labelFileName, 0, 0 );

  m_labelTrackProgress = new QLabel( m_groupProgress, "m_labelTrackProgress" );
  m_labelTrackProgress->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

  m_groupProgressLayout->addWidget( m_labelTrackProgress, 0, 1 );

  m_labelCdTime = new QLabel( m_groupProgress, "m_labelCdTime" );

  m_groupProgressLayout->addWidget( m_labelCdTime, 3, 0 );

  m_labelCdProgress = new QLabel( m_groupProgress, "m_labelCdProgress" );
  m_labelCdProgress->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

  m_groupProgressLayout->addWidget( m_labelCdProgress, 3, 1 );

  mainLayout->addMultiCellWidget( m_groupProgress, 1, 1, 0, 3 );
}


void K3bBurnProgressDialog::setupConnections()
{
  connect( m_buttonCancel, SIGNAL(clicked()), this, SLOT(slotCancelPressed()) );
  connect( m_buttonClose, SIGNAL(clicked()), this, SLOT(accept()) );
  connect( m_buttonShowDebug, SIGNAL(clicked()), this, SLOT(slotShowDebuggingOutput()) );
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

  m_buttonCancel->hide();
  m_buttonShowDebug->show();
  m_buttonClose->show();
  m_timer->stop();

  m_progressBuffer->setValue(0);
}


void K3bBurnProgressDialog::setJob( K3bBurnJob* job )
{
  // clear everything
  m_buttonClose->hide();
  m_buttonShowDebug->hide();
  m_buttonCancel->show();
  m_viewInfo->setText("");
  m_progressBuffer->setValue(0);
  m_progressTrack->setValue(0);
  m_progressCd->setValue(0);
  m_labelFileName->setText("");
  m_labelCdTime->setText("");
  m_labelCdProgress->setText("");
  m_labelTrackProgress->setText("");
  m_groupProgress->setTitle( i18n( "Progress" ) );

  m_debugOutputMap.clear();

  //	m_progressTrack->hide();
  //	m_labelFileName->hide();
  //	m_labelTrackProgress->hide();

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

  connect( job, SIGNAL(newTask(const QString&)), this, SLOT(slotNewTask(const QString&)) );
  connect( job, SIGNAL(newSubTask(const QString&)), this, SLOT(slotNewSubTask(const QString&)) );
  connect( job, SIGNAL(started()), this, SLOT(started()) );
  connect( job, SIGNAL(finished(K3bJob*)), this, SLOT(finished()) );
	
  connect( job, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	   this, SLOT(mapDebuggingOutput(const QString&, const QString&)) );

  if( job->doc() )
    {
      if( job->doc()->burner() )
	m_labelWriter->setText( "Writer: " + job->doc()->burner()->vendor() + " " + 
				job->doc()->burner()->description() );

      // connect to the "special" signals
      connect( job, SIGNAL(bufferStatus(int)), m_progressBuffer, SLOT(setValue(int)) );
		
      m_groupBuffer->setEnabled( true ); 	
    }
}


void K3bBurnProgressDialog::slotCancelPressed()
{
  if( m_job )
    if( KMessageBox::questionYesNo( this, "Do you really want to cancel?", "Cancel" ) == KMessageBox::Yes ) {
      if( m_job )
	m_job->cancel();
    }
}

void K3bBurnProgressDialog::show()
{
  QWidget::show();
}


void K3bBurnProgressDialog::slotNewSubTask(const QString& name)
{
  //	m_progressTrack->show();
  //	m_labelFileName->show();
  //	m_labelTrackProgress->show();
  m_labelFileName->setText(name);
  m_labelTrackProgress->setText("");
  m_progressTrack->setValue(0);
}

void K3bBurnProgressDialog::slotNewTask(const QString& name)
{
  m_groupProgress->setTitle( name );
}


void K3bBurnProgressDialog::started()
{
  m_timer->start( 1000 );
  m_time = 0;
}


void K3bBurnProgressDialog::slotUpdateTime()
{
  m_time++;
  int min = m_time / 60;
  int sec = m_time % 60;
	
  QString timeStr = QString::number(sec);
  if( sec < 10 )
    timeStr = "0" + timeStr;
  timeStr = QString::number(min) + ":" + timeStr;
  if( min < 10 )
    timeStr = "0" + timeStr;	
		
  m_labelCdTime->setText( i18n("Overall progress - %1").arg(timeStr) );	
}


void K3bBurnProgressDialog::mapDebuggingOutput( const QString& type, const QString& output )
{
  m_debugOutputMap[type].append(output);
}


void K3bBurnProgressDialog::slotShowDebuggingOutput()
{
  PrivateDebugWidget* debugWidget = new PrivateDebugWidget( m_debugOutputMap, this );
  debugWidget->exec();
  delete debugWidget;
}
