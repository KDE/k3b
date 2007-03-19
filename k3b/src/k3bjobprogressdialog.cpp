/*
 *
 * $Id$
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bjobprogressdialog.h"
#include "k3bapplication.h"
#include "k3bemptydiscwaiter.h"
#include "k3bjobprogressosd.h"
#include "k3bdebuggingoutputdialog.h"
#include "k3bapplication.h"
#include "k3bjobinterface.h"
#include "k3bthemedlabel.h"
#include <k3bjob.h>
#include <kcutlabel.h>
#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bdeviceglobals.h>
#include <k3bglobals.h>
#include <k3bstdguiitems.h>
#include <k3bversion.h>
#include <k3bthememanager.h>

#include <qgroupbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qdatetime.h>
#include <qstring.h>
#include <qhbox.h>
#include <qheader.h>
#include <qscrollbar.h>
#include <qpoint.h>
#include <qfontmetrics.h>
#include <qtimer.h>
#include <qfont.h>
#include <qeventloop.h>
#include <qfile.h>
#include <qapplication.h>

#include <kprogress.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <klistview.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <knotifyclient.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kmainwindow.h>
#include <kstdguiitem.h>
#include <kpushbutton.h>




class K3bJobProgressDialog::Private
{
public:
  int lastProgress;
};



K3bJobProgressDialog::K3bJobProgressDialog( QWidget* parent, 
					    const char* name, 
					    bool showSubProgress,
					    bool modal, WFlags fl )
  : KDialog( parent, name, modal, fl ),
    in_loop(false),
    m_osd(0)
{
  d = new Private;

  setupGUI();
  setupConnections();

  if( !showSubProgress ) {
    m_progressSubPercent->hide();
  }

  m_job = 0;
  m_timer = new QTimer( this );

  connect( m_timer, SIGNAL(timeout()), this, SLOT(slotUpdateTime()) );
}


/*
 *  Destroys the object and frees any allocated resources
 */
K3bJobProgressDialog::~K3bJobProgressDialog()
{
  delete d;
  delete m_osd;
}


void K3bJobProgressDialog::setupGUI()
{
  QVBoxLayout* mainLayout = new QVBoxLayout( this, 11, 6, "mainLayout"); 


  // header 
  // ------------------------------------------------------------------------------------------
  QFrame* headerFrame = new QFrame( this, "headerFrame" );
  headerFrame->setFrameShape( QFrame::StyledPanel );
  headerFrame->setFrameShadow( QFrame::Sunken );
  headerFrame->setLineWidth( 1 );
  headerFrame->setMargin( 1 );
  QHBoxLayout* headerLayout = new QHBoxLayout( headerFrame ); 
  headerLayout->setMargin( 2 ); // to make sure the frame gets displayed
  headerLayout->setSpacing( 0 );
  m_pixLabel = new K3bThemedLabel( headerFrame );
  headerLayout->addWidget( m_pixLabel );

  QFrame* frame4 = new QFrame( headerFrame, "frame4" );
  frame4->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)5, 1, 0, frame4->sizePolicy().hasHeightForWidth() ) );
  frame4->setFrameShape( QFrame::NoFrame );
  frame4->setFrameShadow( QFrame::Raised );
  QVBoxLayout* frame4Layout = new QVBoxLayout( frame4, 6, 3, "frame4Layout"); 

  m_labelJob = new K3bThemedLabel( frame4 );
  m_labelJob->setMinimumVisibleText( 40 );
  QFont m_labelJob_font(  m_labelJob->font() );
  m_labelJob_font.setPointSize( m_labelJob_font.pointSize() + 2 );
  m_labelJob_font.setBold( true );
  m_labelJob->setFont( m_labelJob_font ); 
  m_labelJob->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );
  frame4Layout->addWidget( m_labelJob );

  m_labelJobDetails = new K3bThemedLabel( frame4 );
  m_labelJobDetails->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)5, 0, 1, m_labelJobDetails->sizePolicy().hasHeightForWidth() ) );
  m_labelJobDetails->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );
  frame4Layout->addWidget( m_labelJobDetails );
  headerLayout->addWidget( frame4 );

  mainLayout->addWidget( headerFrame );
  // ------------------------------------------------------------------------------------------


  m_viewInfo = new KListView( this, "m_viewInfo" );
  m_viewInfo->addColumn( "" );
  m_viewInfo->addColumn( i18n( "Message" ) );
  m_viewInfo->setFullWidth( true );
  m_viewInfo->header()->hide();
  m_viewInfo->setSorting(-1);
  mainLayout->addWidget( m_viewInfo );


  // progress header
  // ------------------------------------------------------------------------------------------
  QFrame* progressHeaderFrame = new QFrame( this, "progressHeaderFrame" );
  progressHeaderFrame->setFrameShape( QFrame::StyledPanel );
  progressHeaderFrame->setFrameShadow( QFrame::Sunken );
  progressHeaderFrame->setLineWidth( 1 );
  progressHeaderFrame->setMargin( 1 );

  QHBoxLayout* progressHeaderLayout = new QHBoxLayout( progressHeaderFrame ); 
  progressHeaderLayout->setMargin( 2 );
  progressHeaderLayout->setSpacing( 0 );

  QFrame* frame5 = new QFrame( progressHeaderFrame, "frame5" );
  frame5->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)5, 1, 0, frame5->sizePolicy().hasHeightForWidth() ) );
  frame5->setFrameShape( QFrame::NoFrame );
  frame5->setFrameShadow( QFrame::Raised );
  QVBoxLayout* frame5Layout = new QVBoxLayout( frame5, 6, 3, "frame5Layout"); 

  m_labelTask = new K3bThemedLabel( frame5 );
  QFont m_labelTask_font(  m_labelTask->font() );
  m_labelTask_font.setPointSize( m_labelTask_font.pointSize() + 2 );
  m_labelTask_font.setBold( true );
  m_labelTask->setFont( m_labelTask_font ); 
  frame5Layout->addWidget( m_labelTask );

  m_labelElapsedTime = new K3bThemedLabel( frame5 );
  m_labelElapsedTime->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)5, 0, 1, m_labelElapsedTime->sizePolicy().hasHeightForWidth() ) );
  frame5Layout->addWidget( m_labelElapsedTime );
  progressHeaderLayout->addWidget( frame5 );

  progressHeaderLayout->addWidget( new K3bThemedLabel( K3bTheme::PROGRESS_RIGHT, progressHeaderFrame ) );
  mainLayout->addWidget( progressHeaderFrame );
  // ------------------------------------------------------------------------------------------

  QHBoxLayout* layout3 = new QHBoxLayout( 0, 0, 6, "layout3"); 

  m_labelSubTask = new KCutLabel( this, "m_labelSubTask" );
  layout3->addWidget( m_labelSubTask );

  m_labelSubProcessedSize = new QLabel( this, "m_labelSubProcessedSize" );
  m_labelSubProcessedSize->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );
  layout3->addWidget( m_labelSubProcessedSize );
  mainLayout->addLayout( layout3 );

  m_progressSubPercent = new KProgress( this, "m_progressSubPercent" );
  mainLayout->addWidget( m_progressSubPercent );

  QHBoxLayout* layout4 = new QHBoxLayout( 0, 0, 6, "layout4"); 

  QLabel* textLabel5 = new QLabel( i18n("Overall progress:"), this, "textLabel5" );
  layout4->addWidget( textLabel5 );

  m_labelProcessedSize = new QLabel( this, "m_labelProcessedSize" );
  m_labelProcessedSize->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );
  layout4->addWidget( m_labelProcessedSize );
  mainLayout->addLayout( layout4 );

  m_progressPercent = new KProgress( this, "m_progressPercent" );
  mainLayout->addWidget( m_progressPercent );

  m_frameExtraInfo = new QFrame( this, "m_frameExtraInfo" );
  m_frameExtraInfo->setFrameShape( QFrame::NoFrame );
  m_frameExtraInfo->setFrameShadow( QFrame::Raised );
  m_frameExtraInfoLayout = new QGridLayout( m_frameExtraInfo ); 
  m_frameExtraInfoLayout->setMargin(0);
  m_frameExtraInfoLayout->setSpacing( spacingHint() );
  mainLayout->addWidget( m_frameExtraInfo );

  QFrame* line2 = new QFrame( this, "line2" );
  line2->setFrameShape( QFrame::HLine );
 line2->setFrameShadow( QFrame::Sunken );
  mainLayout->addWidget( line2 );

  QHBoxLayout* layout5 = new QHBoxLayout( 0, 0, 6, "layout5"); 
  QSpacerItem* spacer = new QSpacerItem( 10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum );
  layout5->addItem( spacer );

  m_buttonCancel = new KPushButton( KStdGuiItem::cancel(), this, "m_buttonCancel" );
  layout5->addWidget( m_buttonCancel );
  m_buttonClose = new KPushButton( KStdGuiItem::close(), this );
  layout5->addWidget( m_buttonClose );
  m_buttonShowDebug = new QPushButton( i18n("Show Debugging Output"), this );
  layout5->addWidget( m_buttonShowDebug );

  mainLayout->addLayout( layout5 );

  m_pixLabel->setThemePixmap( K3bTheme::PROGRESS_WORKING );

  slotThemeChanged();

  connect( k3bappcore->themeManager(), SIGNAL(themeChanged()),
	   this, SLOT(slotThemeChanged()) );
  connect( kapp, SIGNAL(appearanceChanged()),
	   this, SLOT(slotThemeChanged()) );
}


void K3bJobProgressDialog::show()
{
  if( KConfigGroup( k3bcore->config(), "General Options" ).readBoolEntry( "hide main window while writing", false ) )
    if( QWidget* w = kapp->mainWidget() )
      w->hide();

  if( m_osd ) {
    m_osd->readSettings( k3bcore->config() );
    m_osd->show();
  }

  KDialog::show();
}


void K3bJobProgressDialog::setExtraInfo( QWidget *extra )
{
  extra->reparent( m_frameExtraInfo, QPoint(0,0) );
  m_frameExtraInfoLayout->addWidget( extra, 0, 0 );
}


void K3bJobProgressDialog::closeEvent( QCloseEvent* e )
{
  if( m_buttonClose->isVisible() ) {
    KDialog::closeEvent( e );
    if( QWidget* w = kapp->mainWidget() )
      w->show();

    if( !m_plainCaption.isEmpty() )
      if( KMainWindow* w = dynamic_cast<KMainWindow*>(kapp->mainWidget()) )
	w->setPlainCaption( m_plainCaption );

    if( m_osd ) {
      m_osd->hide();
      m_osd->saveSettings( kapp->config() );
    }
  }
  else
    e->ignore();
}


void K3bJobProgressDialog::setupConnections()
{
  connect( m_buttonCancel, SIGNAL(clicked()), this, SLOT(slotCancelButtonPressed()) );
  connect( m_buttonClose, SIGNAL(clicked()), this, SLOT(close()) );
  connect( m_buttonShowDebug, SIGNAL(clicked()), this, SLOT(slotShowDebuggingOutput()) );
}


void K3bJobProgressDialog::slotProcessedSize( int processed, int size )
{
  m_labelProcessedSize->setText( i18n("%1 of %2 MB").arg( processed ).arg( size ) );
}


void K3bJobProgressDialog::slotProcessedSubSize( int processedTrackSize, int trackSize )
{
  m_labelSubProcessedSize->setText( i18n("%1 of %2 MB").arg(processedTrackSize).arg(trackSize) );
}


void K3bJobProgressDialog::slotInfoMessage( const QString& infoString, int type )
{
  QListViewItem* currentInfoItem = new QListViewItem( m_viewInfo, m_viewInfo->lastItem(), QString::null, infoString );
  currentInfoItem->setSelectable( false );

  // set the icon
  switch( type ) {
  case K3bJob::ERROR:
    currentInfoItem->setPixmap( 0, SmallIcon( "stop" ) );
    break;
  case K3bJob::WARNING:
    currentInfoItem->setPixmap( 0, SmallIcon( "yellowinfo" ) );
    break;
  case K3bJob::SUCCESS:
    currentInfoItem->setPixmap( 0, SmallIcon( "ok" ) );
    break;
  case K3bJob::INFO:
  default:
    currentInfoItem->setPixmap( 0, SmallIcon( "info" ) );
  }

  // This should scroll down (hopefully!)
  m_viewInfo->ensureItemVisible( currentInfoItem );
}


void K3bJobProgressDialog::slotFinished( bool success )
{
  kdDebug() << "(K3bJobProgressDialog) received finished signal!" << endl;

  m_logFile.close();

  if( success ) {
    m_pixLabel->setThemePixmap( K3bTheme::PROGRESS_SUCCESS );

    m_labelTask->setText( i18n("Success.") );
    m_labelTask->setPaletteForegroundColor( Qt::darkGreen );
    m_labelSubTask->setText( QString::null );

    m_progressPercent->setValue(100);
    m_progressSubPercent->setValue(100);
    slotProgress(100);

    // one last time update to be sure no remaining time is displayed anymore
    slotUpdateTime();

    if( m_osd )
      m_osd->setText( i18n("Success.") );

    KNotifyClient::event( 0, "SuccessfullyFinished", i18n("Successfully finished.") );
  }
  else {
    m_pixLabel->setThemePixmap( K3bTheme::PROGRESS_FAIL );

    m_labelTask->setPaletteForegroundColor( Qt::red );

    if( m_bCanceled ) {
      m_labelTask->setText( i18n("Canceled.") );
      if( m_osd )
	m_osd->setText( i18n("Canceled.") );
    }
    else {
      m_labelTask->setText( i18n("Error.") );
      if( m_osd )
	m_osd->setText( i18n("Error.") );
    }
   
    KNotifyClient::event( 0, "FinishedWithError", i18n("Finished with errors") );
  }

  m_buttonCancel->hide();
  m_buttonShowDebug->show();
  m_buttonClose->show();
  m_timer->stop();
}


void K3bJobProgressDialog::slotCanceled()
{
  m_bCanceled = true;
}


void K3bJobProgressDialog::setJob( K3bJob* job )
{
  m_bCanceled = false;

  // clear everything
  m_buttonClose->hide();
  m_buttonShowDebug->hide();
  m_buttonCancel->show();
  m_buttonCancel->setEnabled(true);
  m_viewInfo->clear();
  m_progressPercent->setValue(0);
  m_progressSubPercent->setValue(0);
  m_labelTask->setText("");
  m_labelSubTask->setText("");
  m_labelProcessedSize->setText("");
  m_labelSubProcessedSize->setText("");
  m_labelTask->setPaletteForegroundColor( k3bappcore->themeManager()->currentTheme()->foregroundColor() );
  m_debugOutputMap.clear();

  // disconnect from the former job
  if( m_job )
    disconnect( m_job );
  m_job = job;

  if( job ) {

    // connect to all the shit
    connect( job, SIGNAL(infoMessage(const QString&,int)), this, SLOT(slotInfoMessage(const QString&,int)) );
    
    connect( job, SIGNAL(percent(int)), m_progressPercent, SLOT(setValue(int)) );
    connect( job, SIGNAL(percent(int)), this, SLOT(slotProgress(int)) );
    connect( job, SIGNAL(subPercent(int)), m_progressSubPercent, SLOT(setValue(int)) );
    
    connect( job, SIGNAL(processedSubSize(int, int)), this, SLOT(slotProcessedSubSize(int, int)) );
    connect( job, SIGNAL(processedSize(int, int)), this, SLOT(slotProcessedSize(int, int)) );
    
    connect( job, SIGNAL(newTask(const QString&)), this, SLOT(slotNewTask(const QString&)) );
    connect( job, SIGNAL(newSubTask(const QString&)), this, SLOT(slotNewSubTask(const QString&)) );
    connect( job, SIGNAL(started()), this, SLOT(slotStarted()) );
    connect( job, SIGNAL(finished(bool)), this, SLOT(slotFinished(bool)) );
    connect( job, SIGNAL(canceled()), this, SLOT(slotCanceled()) );
    
    connect( job, SIGNAL(debuggingOutput(const QString&, const QString&)),
	     this, SLOT(slotDebuggingOutput(const QString&, const QString&)) );

    m_labelJob->setText( m_job->jobDescription() );
    m_labelJobDetails->setText( m_job->jobDetails() );

    setCaption( m_job->jobDescription() );

    if( KConfigGroup( k3bcore->config(), "General Options" ).readBoolEntry( "Show progress OSD", true ) ) {
      if( !m_osd )
	m_osd = new K3bJobProgressOSD( this, "progressosd" );
    }
    else
      delete m_osd;

    if( m_osd ) {
      m_osd->setText( job->jobDescription() );
      // FIXME: use a setJob method and let the osd also change the text color to red/green
      //      connect( job, SIGNAL(newTask(const QString&)), m_osd, SLOT(setText(const QString&)) );
      connect( job, SIGNAL(percent(int)), m_osd, SLOT(setProgress(int)) );
    }
  }
}


void K3bJobProgressDialog::slotCancelButtonPressed()
{
  if( m_job )
    if( KMessageBox::questionYesNo( this, i18n("Do you really want to cancel?"), i18n("Cancel Confirmation") ) == KMessageBox::Yes ) {
      if( m_job ) {
	m_job->cancel();
	m_buttonCancel->setDisabled(true);  // do not cancel twice
      }
    }
}


void K3bJobProgressDialog::slotNewSubTask(const QString& name)
{
  m_labelSubTask->setText(name);
  m_labelSubProcessedSize->setText("");
  m_progressSubPercent->setValue(0);
}

void K3bJobProgressDialog::slotNewTask(const QString& name)
{
  m_labelTask->setText( name );
}


void K3bJobProgressDialog::slotStarted()
{
  d->lastProgress = 0;
  m_timer->start( 1000 );
  m_startTime = QTime::currentTime();
  if( KMainWindow* w = dynamic_cast<KMainWindow*>(kapp->mainWidget()) )
    m_plainCaption = w->caption();

  m_logFile.open();
}


void K3bJobProgressDialog::slotUpdateTime()
{
  int elapsed = m_startTime.secsTo( QTime::currentTime() );

  QString s = i18n("Elapsed time: %1 h").arg( QTime().addSecs(elapsed).toString() );
  if( d->lastProgress > 0 && d->lastProgress < 100 ) {
    int rem = m_startTime.secsTo( m_lastProgressUpdateTime ) * (100-d->lastProgress) / d->lastProgress;
    s += " / " + i18n("Remaining: %1 h").arg( QTime().addSecs(rem).toString() );
  }

  m_labelElapsedTime->setText( s );
}


void K3bJobProgressDialog::slotDebuggingOutput( const QString& type, const QString& output )
{
  m_debugOutputMap[type].append(output);
  m_logFile.addOutput( type, output );
}


void K3bJobProgressDialog::slotShowDebuggingOutput()
{
  K3bDebuggingOutputDialog debugWidget( this );
  debugWidget.setOutput( m_debugOutputMap );
  debugWidget.exec();
}


void K3bJobProgressDialog::slotProgress( int percent )
{
  if( percent > d->lastProgress ) {
    d->lastProgress = percent;
    m_lastProgressUpdateTime = QTime::currentTime();
    if( KMainWindow* w = dynamic_cast<KMainWindow*>(kapp->mainWidget()) ) {
      w->setPlainCaption( QString( "(%1%) %2" ).arg(percent).arg(m_plainCaption) );
    }

    setCaption( QString( "(%1%) %2" ).arg(percent).arg(m_job->jobDescription()) );
  }
}


void K3bJobProgressDialog::keyPressEvent( QKeyEvent *e )
{
  e->accept();

  switch ( e->key() ) {
  case Key_Enter:
  case Key_Return:
    // if the process finished this closes the dialog
    if( m_buttonClose->isVisible() )
      close();
    break;
  case Key_Escape:
    // simulate button clicks
    if( m_buttonCancel->isVisible() && m_buttonCancel->isEnabled() )
      slotCancelButtonPressed();
    else if( !m_buttonCancel->isVisible() )
      close();
    break;
  default:
    // nothing
    break;
  }
}


QSize K3bJobProgressDialog::sizeHint() const
{
  QSize s = layout()->totalSizeHint();
  if( s.width() < s.height() )
    s.setWidth( s.height() );
  return s;
}


int K3bJobProgressDialog::startJob( K3bJob* job )
{
  if( job ) {
    setJob( job );
    k3bappcore->jobInterface()->setJob( job );
  }
  else if( !m_job ) {
    kdError() << "(K3bJobProgressDialog) null job!" << endl;
    return -1;
  }

  // the following code is mainly taken from QDialog::exec

  if ( in_loop ) {
    kdError() << "(K3bJobProgressDialog::startJob) Recursive call detected." << endl;
    return -1;
  }
  
  bool destructiveClose = testWFlags( WDestructiveClose );
  clearWFlags( WDestructiveClose );
  
  bool wasShowModal = testWFlags( WShowModal );
  setWFlags( WShowModal );
  setResult( 0 );

  show();
  
  // start the job after showing the dialog
  m_job->start();

  in_loop = true;
  QApplication::eventLoop()->enterLoop();

  if ( !wasShowModal )
    clearWFlags( WShowModal );
  
  int res = result();
  
  if ( destructiveClose )
    delete this;
  
  return res;
}


void K3bJobProgressDialog::hide()
{
  // we need to reimplement this since
  // QDialog does not know if we are in a loop from startJob

  if ( isHidden() )
    return;
  
  KDialog::hide();
  
  if ( in_loop ) {
    in_loop = FALSE;
    QApplication::eventLoop()->exitLoop();
  }
}


int K3bJobProgressDialog::waitForMedia( K3bDevice::Device* device,
					int mediaState,
					int mediaType,
					const QString& message )
{
  return K3bEmptyDiscWaiter::wait( device, mediaState, mediaType, message, this );
}

  
bool K3bJobProgressDialog::questionYesNo( const QString& text,
					  const QString& caption,
					  const QString& yesText,
					  const QString& noText )
{
  return ( KMessageBox::questionYesNo( this, 
				       text, 
				       caption, 
				       yesText.isEmpty() ? KStdGuiItem::yes() : KGuiItem(yesText),
				       noText.isEmpty() ? KStdGuiItem::no() : KGuiItem(noText) ) == KMessageBox::Yes );
}


void K3bJobProgressDialog::blockingInformation( const QString& text,
						const QString& caption )
{
  KMessageBox::information( this, text, caption );
}


void K3bJobProgressDialog::slotThemeChanged()
{
  if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
    static_cast<QWidget*>(child( "frame4" ))->setPaletteBackgroundColor( theme->backgroundColor() );
    static_cast<QWidget*>(child( "frame4" ))->setPaletteForegroundColor( theme->backgroundColor() );
    static_cast<QWidget*>(child( "frame5" ))->setPaletteBackgroundColor( theme->backgroundColor() );
    static_cast<QWidget*>(child( "frame5" ))->setPaletteForegroundColor( theme->backgroundColor() );
    static_cast<QWidget*>(child( "progressHeaderFrame" ))->setPaletteBackgroundColor( theme->backgroundColor() );
    static_cast<QWidget*>(child( "progressHeaderFrame" ))->setPaletteForegroundColor( theme->backgroundColor() );
    static_cast<QWidget*>(child( "headerFrame" ))->setPaletteBackgroundColor( theme->backgroundColor() );
    static_cast<QWidget*>(child( "headerFrame" ))->setPaletteForegroundColor( theme->backgroundColor() );
  }
}

#include "k3bjobprogressdialog.moc"
