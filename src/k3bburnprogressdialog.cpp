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
#include "k3b.h"

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
#include <qheader.h>
#include <qscrollbar.h>
#include <qpoint.h>
#include <qfontmetrics.h>
#include <qtimer.h>
#include <qfont.h>

#include <kprogress.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <klistview.h>
#include <kiconloader.h>
#include <kstatusbar.h>
#include <kconfig.h>
#include <ksystemtray.h>
#include <kdebug.h>


class K3bBurnProgressDialog::PrivateDebugWidget : public KDialog 
{
public:
  PrivateDebugWidget( QMap<QString, QStringList>&, QWidget* parent );
};


K3bBurnProgressDialog::PrivateDebugWidget::PrivateDebugWidget( QMap<QString, QStringList>& map, QWidget* parent )
  : KDialog( parent, "debugViewDialog", true )
{
  setCaption( i18n("Debugging output") );

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




K3bBurnProgressDialog::K3bBurnProgressDialog( QWidget *parent, const char *name, bool showSubProgress, 
					      bool showBuffer, bool modal, WFlags wf )
  : KDialog(parent,name, modal, wf), m_showBuffer( showBuffer )
{
  setCaption( i18n("K3b - Progress") );

  m_systemTray = new KSystemTray( this );
  m_systemTray->setPixmap( kapp->iconLoader()->loadIcon( "k3b", KIcon::Panel, 24 ) );

  setupGUI();
  setupConnections();

  // FIXME: this is bad hacking (although it should work!)
  // -----
  if( !showSubProgress ) {
    m_labelFileName->hide();
    m_labelTrackProgress->hide();
    m_progressTrack->hide();
  }
  // -----

  m_job = 0;
  m_timer = new QTimer( this );

  connect( m_timer, SIGNAL(timeout()), this, SLOT(slotUpdateTime()) );
}

K3bBurnProgressDialog::~K3bBurnProgressDialog()
{
}


void K3bBurnProgressDialog::show()
{
  KConfig* c = kapp->config();
  c->setGroup( "General Options");

  m_bShowSystemTrayProgress = c->readBoolEntry( "Show progress in system tray", true );
  if( m_bShowSystemTrayProgress ) {
    m_systemTray->show();
  }

  KDialog::show();
}

void K3bBurnProgressDialog::setExtraInfo( QWidget *extra ){
    mainLayout->addMultiCellWidget( extra, 1, 1, 0, 3 );
    extra->show();
}

void K3bBurnProgressDialog::closeEvent( QCloseEvent* e )
{
  QToolTip::remove( m_systemTray );
  QToolTip::add( m_systemTray, i18n("Ready") );
  m_systemTray->hide();

  if( m_buttonClose->isVisible() ) {
    KDialog::closeEvent( e );
  }
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

  m_viewInfo = new KListView( m_groupInfo, "m_viewInfo" );
  m_viewInfo->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, 
					  (QSizePolicy::SizeType)7, 
					  m_viewInfo->sizePolicy().hasHeightForWidth() ) );
  m_viewInfo->setMinimumSize( QSize( 500, 0 ) );
  m_viewInfo->addColumn( i18n("type") );
  m_viewInfo->addColumn( i18n("info") );
  m_viewInfo->header()->hide();
  m_viewInfo->setSorting( -1 );
  m_groupInfoLayout->addWidget( m_viewInfo );


  m_buttonCancel = new QPushButton( this, "m_buttonCancel" );
  m_buttonCancel->setText( i18n( "Cancel" ) );
  m_buttonClose = new QPushButton( this, "m_buttonClose" );
  m_buttonClose->setText( i18n( "Close" ) );
  m_buttonShowDebug = new QPushButton( i18n("Show Debugging Output"), this, "m_buttonShowDebug" );

  m_groupBuffer = new QGroupBox( this, "m_groupBuffer" );
  m_groupBuffer->setTitle( i18n( "Buffer Status" ) );
  m_groupBuffer->setColumnLayout(0, Qt::Vertical );
  m_groupBuffer->layout()->setSpacing( 0 );
  m_groupBuffer->layout()->setMargin( 0 );
  m_groupBufferLayout = new QHBoxLayout( m_groupBuffer->layout() );
  m_groupBufferLayout->setAlignment( Qt::AlignTop );
  m_groupBufferLayout->setSpacing( spacingHint() );
  m_groupBufferLayout->setMargin( marginHint() );

  m_labelWriter = new QLabel( i18n("Writer"), m_groupBuffer );
  m_progressBuffer = new KProgress( m_groupBuffer, "m_progressBuffer" );
  m_progressBuffer->setMaximumWidth( 150 );

  m_groupBufferLayout->addWidget( m_labelWriter );
  m_groupBufferLayout->addWidget( m_progressBuffer );

  m_groupProgress = new QGroupBox( this, "m_groupProgress" );
  m_groupProgress->setTitle( i18n( "Progress" ) );
  m_groupProgress->setColumnLayout(0, Qt::Vertical );
  m_groupProgress->layout()->setSpacing( 0 );
  m_groupProgress->layout()->setMargin( 0 );
  m_groupProgressLayout = new QGridLayout( m_groupProgress->layout() );
  m_groupProgressLayout->setAlignment( Qt::AlignTop );
  m_groupProgressLayout->setSpacing( spacingHint() );
  m_groupProgressLayout->setMargin( marginHint() );

  m_progressTrack = new KProgress( m_groupProgress, "m_progressTrack" );
  m_groupProgressLayout->addMultiCellWidget( m_progressTrack, 1, 1, 0, 1 );
  m_progressCd = new KProgress( m_groupProgress, "m_progressCd" );
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


  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  mainLayout->addItem( spacer, 4, 0 );
  QSpacerItem* spacer_2 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  mainLayout->addItem( spacer_2, 4, 3 );
  mainLayout->addMultiCellWidget( m_groupInfo, 0, 0, 0, 3 );
  mainLayout->addWidget( m_buttonCancel, 4, 1 );
  mainLayout->addWidget( m_buttonClose, 4, 1 );
  mainLayout->addWidget( m_buttonShowDebug, 4, 2 );
  mainLayout->addMultiCellWidget( m_groupProgress, 2, 2, 0, 3 );
  mainLayout->addMultiCellWidget( m_groupBuffer, 3, 3, 0, 3 );

  QFont f( m_groupProgress->font() );
  f.setBold( true );
  //  m_groupProgress->setFont( f );
}


void K3bBurnProgressDialog::setupConnections()
{
  connect( m_buttonCancel, SIGNAL(clicked()), this, SLOT(slotCancelPressed()) );
  connect( m_buttonClose, SIGNAL(clicked()), this, SLOT(close()) );
  connect( m_buttonShowDebug, SIGNAL(clicked()), this, SLOT(slotShowDebuggingOutput()) );
}


void K3bBurnProgressDialog::updateCdSizeProgress( int processed, int size )
{
  m_labelCdProgress->setText( i18n("%1 of %2 MB written").arg( processed ).arg( size ) );
}


void K3bBurnProgressDialog::updateTrackSizeProgress( int processedTrackSize, int trackSize )
{
  m_labelTrackProgress->setText( i18n("%1 of %2 MB written").arg(processedTrackSize).arg(trackSize) );
}


void K3bBurnProgressDialog::displayInfo( const QString& infoString, int type )
{
  QListViewItem* currentInfoItem = new QListViewItem( m_viewInfo, m_viewInfo->lastItem(), QString::null, infoString );

  // set the icon
  switch( type ) {
  case K3bJob::ERROR:
    currentInfoItem->setPixmap( 0, SmallIcon( "stop" ) );
    break;
  case K3bJob::PROCESS:
    currentInfoItem->setPixmap( 0, SmallIcon( "cdwriter_unmount" ) );
    break;
  case K3bJob::STATUS:
    currentInfoItem->setPixmap( 0, SmallIcon( "ok" ) );
    break;
  case K3bJob::INFO:
  default:
    currentInfoItem->setPixmap( 0, SmallIcon( "info" ) );
  }

  // This should scroll down (hopefully!)
  m_viewInfo->ensureItemVisible( currentInfoItem );
}


void K3bBurnProgressDialog::finished( bool success )
{
  kdDebug() << "(K3bBurnProgressDialog) received finished signal!" << endl;

  m_job = 0;

  if( success ) {
    m_labelFileName->setText( "" );
    m_groupProgress->setTitle( i18n("Success") );
    m_progressCd->setValue(100);
    m_progressTrack->setValue(100);
    m_labelTrackProgress->setText("");
    m_progressBuffer->setValue(0);
  }
  else {

    if( m_bCanceled )
      m_groupProgress->setTitle( i18n("Canceled") );
    else
      m_groupProgress->setTitle( i18n("Error") );
  }

  m_buttonCancel->hide();
  m_buttonShowDebug->show();
  m_buttonClose->show();
  m_timer->stop();
}


void K3bBurnProgressDialog::canceled()
{
  m_bCanceled = true;
}


void K3bBurnProgressDialog::setJob( K3bJob* job )
{
  m_bCanceled = false;

  // clear everything
  m_buttonClose->hide();
  m_buttonShowDebug->hide();
  m_buttonCancel->show();
  m_viewInfo->clear();
  m_progressBuffer->setValue(0);
  m_progressTrack->setValue(0);
  m_progressCd->setValue(0);
  m_labelFileName->setText("");
  m_labelCdTime->setText("");
  m_labelCdProgress->setText("");
  m_labelTrackProgress->setText("");
  m_groupProgress->setTitle( i18n( "Progress" ) );

  m_debugOutputMap.clear();


  // disconnect from the former job
  if( m_job )
    disconnect( m_job );
  m_job = job;
	
  // connect to all the shit
  connect( job, SIGNAL(infoMessage(const QString&,int)), this, SLOT(displayInfo(const QString&,int)) );
	
  connect( job, SIGNAL(percent(int)), m_progressCd, SLOT(setValue(int)) );
  connect( job, SIGNAL(percent(int)), this, SLOT(animateSystemTray(int)) );
  connect( job, SIGNAL(subPercent(int)), m_progressTrack, SLOT(setValue(int)) );

  connect( job, SIGNAL(processedSubSize(int, int)), this, SLOT(updateTrackSizeProgress(int, int)) );
  connect( job, SIGNAL(processedSize(int, int)), this, SLOT(updateCdSizeProgress(int, int)) );

  connect( job, SIGNAL(newTask(const QString&)), this, SLOT(slotNewTask(const QString&)) );
  connect( job, SIGNAL(newSubTask(const QString&)), this, SLOT(slotNewSubTask(const QString&)) );
  connect( job, SIGNAL(started()), this, SLOT(started()) );
  connect( job, SIGNAL(finished(bool)), this, SLOT(finished(bool)) );
  connect( job, SIGNAL(canceled()), this, SLOT(canceled()) );
	
  connect( job, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	   this, SLOT(mapDebuggingOutput(const QString&, const QString&)) );


  
  K3bBurnJob* burnJob = dynamic_cast<K3bBurnJob*>( job );
  if( m_showBuffer && burnJob ) {
    if( burnJob->writer() )
      m_labelWriter->setText( i18n("Writer: %1 %2").arg(burnJob->writer()->vendor()). 
			      arg(burnJob->writer()->description()) );
  
    // connect to the "special" signals
    connect( burnJob, SIGNAL(bufferStatus(int)), m_progressBuffer, SLOT(setValue(int)) );

    m_groupBuffer->show();
  }
  else {
    m_groupBuffer->hide();
  }
}


void K3bBurnProgressDialog::slotCancelPressed()
{
  if( m_job )
    if( KMessageBox::questionYesNo( this, i18n("Do you really want to cancel?"), i18n("Cancel") ) == KMessageBox::Yes ) {
      if( m_job )
	m_job->cancel();
    }
}


void K3bBurnProgressDialog::slotNewSubTask(const QString& name)
{
  m_labelFileName->setText(name);
  m_labelTrackProgress->setText("");
  m_progressTrack->setValue(0);
}

void K3bBurnProgressDialog::slotNewTask(const QString& name)
{
  m_groupProgress->setTitle( name );

  if( m_bShowSystemTrayProgress ) {
    QToolTip::remove( m_systemTray );
    QToolTip::add( m_systemTray, name );
  }
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
		
  m_labelCdTime->setText( i18n("Overall progress (%1)").arg(timeStr) );	
}


void K3bBurnProgressDialog::mapDebuggingOutput( const QString& type, const QString& output )
{
  m_debugOutputMap[type].append(output);
}


void K3bBurnProgressDialog::slotShowDebuggingOutput()
{
  PrivateDebugWidget debugWidget( m_debugOutputMap, this );
  debugWidget.exec();
}


void K3bBurnProgressDialog::animateSystemTray( int percent )
{
  if( m_bShowSystemTrayProgress ) {
    //    QPixmap p = kapp->iconLoader()->loadIcon( "k3b", KIcon::Panel, 24 );
    // TODO: show <percent> percent of the icon
  }
}


#include "k3bburnprogressdialog.moc"
