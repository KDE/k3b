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


#include "k3bjobprogressdialog.h"
#include "k3bjobprogresssystemtray.h"
#include <k3bjob.h>
#include <kcutlabel.h>
#include <k3bdevice.h>
#include <k3bstdguiitems.h>
#include <k3bcore.h>
#include <k3bversion.h>
#include <k3bthememanager.h>

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
#include <qeventloop.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qclipboard.h>
#include <qapplication.h>
#include <qcursor.h>

#include <kprogress.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <klistview.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <ksystemtray.h>
#include <kdebug.h>
#include <kglobal.h>
#include <knotifyclient.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kmainwindow.h>
#include <kstdguiitem.h>
#include <kpushbutton.h>
#include <kfiledialog.h>



class K3bJobProgressDialog::PrivateDebugWidget : public KDialogBase
{
public:
  PrivateDebugWidget( QMap<QString, QStringList>&, QWidget* parent );

private:
  void slotUser1();
  void slotUser2();

  QTextView* debugView;
};


K3bJobProgressDialog::PrivateDebugWidget::PrivateDebugWidget( QMap<QString, QStringList>& map, QWidget* parent )
  : KDialogBase( parent, "debugViewDialog", true, i18n("Debugging Output"), Close|User1|User2, Close, 
		 false, 
		 KStdGuiItem::saveAs(), 
		 KGuiItem( i18n("Copy"), "editcopy" ) )
{
  setButtonTip( User1, i18n("Save to file") );
  setButtonTip( User2, i18n("Copy to clipboard") );

  debugView = new QTextView( this );
  setMainWidget( debugView );

  debugView->append( "System\n" );
  debugView->append( "-----------------------\n" );
  debugView->append( QString("K3b Version:%1 \n").arg( k3bcore->version() ) );
  debugView->append( QString( "KDE Version: %1\n").arg( KDE::versionString() ) );
  debugView->append( QString( "QT Version: %1\n" ).arg( qVersion() ) );
  debugView->append( "\n" );

  // the following may take some time
  QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

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

  QApplication::restoreOverrideCursor();

  resize( 600, 300 );
}


void K3bJobProgressDialog::PrivateDebugWidget::slotUser1()
{
  QString filename = KFileDialog::getSaveFileName();
  if( !filename.isEmpty() ) {
    QFile f( filename );
    if( !f.exists() || KMessageBox::warningYesNo( this,
						  i18n("Do you want to overwrite %1?").arg(filename),
						  i18n("File Exists") )
	== KMessageBox::Yes ) {

      if( f.open( IO_WriteOnly ) ) {
	QTextStream t( &f );
	t << debugView->text();
      }
      else {
	KMessageBox::error( this, i18n("Could not open file %1").arg(filename) );
      }
    }
  }
}


void K3bJobProgressDialog::PrivateDebugWidget::slotUser2()
{
  QApplication::clipboard()->setText( debugView->text(), QClipboard::Clipboard );
}




K3bJobProgressDialog::K3bJobProgressDialog( QWidget* parent,
					    const char* name,
					    bool showSubProgress,
					    bool modal, WFlags fl )
  : KDialog( parent, name, modal, fl ),
    in_loop(false),
    m_systemTray(0)
{
  setupGUI();
  setupConnections();

  if( !showSubProgress ) {
    m_progressSubPercent->hide();
  }

  m_job = 0;
  m_timer = new QTimer( this );

  connect( m_timer, SIGNAL(timeout()), this, SLOT(slotUpdateTime()) );
}

void K3bJobProgressDialog::setupGUI()
{
  QVBoxLayout* mainLayout = new QVBoxLayout( this, 11, 6, "mainLayout");


  // header
  // ------------------------------------------------------------------------------------------
  QFrame* headerFrame = K3bStdGuiItems::purpleFrame( this );
  QHBoxLayout* headerLayout = new QHBoxLayout( headerFrame );
  headerLayout->setMargin( 2 ); // to make sure the frame gets displayed
  headerLayout->setSpacing( 0 );
  m_pixLabel = new QLabel( headerFrame, "m_pixLabel" );
  m_pixLabel->setScaledContents( FALSE );
  headerLayout->addWidget( m_pixLabel );

  QFrame* frame4 = new QFrame( headerFrame, "frame4" );
  frame4->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)5, 1, 0, frame4->sizePolicy().hasHeightForWidth() ) );
  frame4->setFrameShape( QFrame::NoFrame );
  frame4->setFrameShadow( QFrame::Raised );
  QVBoxLayout* frame4Layout = new QVBoxLayout( frame4, 6, 3, "frame4Layout");

  m_labelJob = new QLabel( frame4, "m_labelJob" );
  QFont m_labelJob_font(  m_labelJob->font() );
  m_labelJob_font.setPointSize( m_labelJob_font.pointSize() + 2 );
  m_labelJob_font.setBold( TRUE );
  m_labelJob->setFont( m_labelJob_font );
  m_labelJob->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );
  frame4Layout->addWidget( m_labelJob );

  m_labelJobDetails = new QLabel( frame4, "m_labelJobDetails" );
  m_labelJobDetails->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)5, 0, 1, m_labelJobDetails->sizePolicy().hasHeightForWidth() ) );
  m_labelJobDetails->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );
  frame4Layout->addWidget( m_labelJobDetails );
  headerLayout->addWidget( frame4 );

  mainLayout->addWidget( headerFrame );
  // ------------------------------------------------------------------------------------------


  m_viewInfo = new KListView( this, "m_viewInfo" );
  m_viewInfo->addColumn( "" );
  m_viewInfo->addColumn( i18n( "Message" ) );
  m_viewInfo->setFullWidth( TRUE );
  m_viewInfo->header()->hide();
  m_viewInfo->setSorting(-1);
  mainLayout->addWidget( m_viewInfo );


  // progress header
  // ------------------------------------------------------------------------------------------
  QFrame* progressHeaderFrame = K3bStdGuiItems::purpleFrame( this );
  QHBoxLayout* progressHeaderLayout = new QHBoxLayout( progressHeaderFrame );
  progressHeaderLayout->setMargin( 2 );
  progressHeaderLayout->setSpacing( 0 );

  QFrame* frame5 = new QFrame( progressHeaderFrame, "frame5" );
  frame5->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)5, 1, 0, frame5->sizePolicy().hasHeightForWidth() ) );
  frame5->setFrameShape( QFrame::NoFrame );
  frame5->setFrameShadow( QFrame::Raised );
  QVBoxLayout* frame5Layout = new QVBoxLayout( frame5, 6, 3, "frame5Layout");

  m_labelTask = new KCutLabel( frame5, "m_labelTask" );
  QFont m_labelTask_font(  m_labelTask->font() );
  m_labelTask_font.setPointSize( m_labelTask_font.pointSize() + 2 );
  m_labelTask_font.setBold( TRUE );
  m_labelTask->setFont( m_labelTask_font );
  frame5Layout->addWidget( m_labelTask );

  m_labelElapsedTime = new QLabel( frame5, "m_labelElapsedTime" );
  m_labelElapsedTime->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)5, 0, 1, m_labelElapsedTime->sizePolicy().hasHeightForWidth() ) );
  frame5Layout->addWidget( m_labelElapsedTime );
  progressHeaderLayout->addWidget( frame5 );

  QLabel* pixmapLabel2 = new QLabel( progressHeaderFrame, "pixmapLabel2" );
  pixmapLabel2->setScaledContents( FALSE );
  progressHeaderLayout->addWidget( pixmapLabel2 );
  mainLayout->addWidget( progressHeaderFrame );
  // ------------------------------------------------------------------------------------------

  if( K3bTheme* theme = k3bthememanager->currentTheme() ) {
    m_pixLabel->setPaletteBackgroundColor( theme->backgroundColor() );
    m_labelJob->setPaletteBackgroundColor( theme->backgroundColor() );
    m_labelJobDetails->setPaletteBackgroundColor( theme->backgroundColor() );
    m_labelElapsedTime->setPaletteBackgroundColor( theme->backgroundColor() );
    m_labelTask->setPaletteBackgroundColor( theme->backgroundColor() );

    m_pixLabel->setPaletteForegroundColor( theme->foregroundColor() );
    m_labelJob->setPaletteForegroundColor( theme->foregroundColor() );
    m_labelJobDetails->setPaletteForegroundColor( theme->foregroundColor() );
    m_labelElapsedTime->setPaletteForegroundColor( theme->foregroundColor() );
    m_labelTask->setPaletteForegroundColor( theme->foregroundColor() );

    m_pixLabel->setPixmap( theme->pixmap( "k3bprojectview_left" ) );
    frame4->setPaletteBackgroundColor( theme->backgroundColor() );
    frame5->setPaletteBackgroundColor( theme->backgroundColor() );
    pixmapLabel2->setPaletteBackgroundColor( theme->backgroundColor() );
    pixmapLabel2->setPixmap( theme->pixmap( "k3bprojectview_right" ) );
  }



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
}

/*
 *  Destroys the object and frees any allocated resources
 */
K3bJobProgressDialog::~K3bJobProgressDialog()
{
  // no need to delete child widgets, Qt does it all for us
}


void K3bJobProgressDialog::show()
{
  KConfig* c = k3bcore->config();
  c->setGroup( "General Options");

  m_bShowSystemTrayProgress = c->readBoolEntry( "Show progress in system tray", true );
  if( m_bShowSystemTrayProgress ) {
    if( !m_systemTray )
      m_systemTray = new K3bJobProgressSystemTray(this);
    m_systemTray->setJob( m_job );
    m_systemTray->show();
  }

  if( c->readBoolEntry( "hide main window while writing", false ) )
    if( QWidget* w = kapp->mainWidget() )
      w->hide();

  KDialog::show();
}

void K3bJobProgressDialog::setExtraInfo( QWidget *extra ){
  extra->reparent( m_frameExtraInfo, QPoint(0,0) );
  m_frameExtraInfoLayout->addWidget( extra, 0, 0 );
}

void K3bJobProgressDialog::closeEvent( QCloseEvent* e )
{
  if( m_buttonClose->isVisible() ) {
    KDialog::closeEvent( e );
    if( QWidget* w = kapp->mainWidget() )
      w->show();

    if( m_systemTray ) {
      m_systemTray->hide();
    }

    if( !m_plainCaption.isEmpty() )
      if( KMainWindow* w = dynamic_cast<KMainWindow*>(kapp->mainWidget()) )
	w->setPlainCaption( m_plainCaption );
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
  m_labelProcessedSize->setText( i18n("%1 of %2 MB written").arg( processed ).arg( size ) );
}


void K3bJobProgressDialog::slotProcessedSubSize( int processedTrackSize, int trackSize )
{
  m_labelSubProcessedSize->setText( i18n("%1 of %2 MB written").arg(processedTrackSize).arg(trackSize) );
}


void K3bJobProgressDialog::slotInfoMessage( const QString& infoString, int type )
{
  QListViewItem* currentInfoItem = new QListViewItem( m_viewInfo, m_viewInfo->lastItem(), QString::null, infoString );

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

  m_job = 0;

  if( success ) {
    m_pixLabel->setPixmap( k3bthememanager->currentTheme()->pixmap( "k3b_progress_dialog_success" ) );

    m_labelTask->setText( i18n("Success!") );
    m_labelTask->setPaletteForegroundColor( Qt::darkGreen );
    m_labelSubTask->setText( QString::null );

    m_progressPercent->setValue(100);
    m_progressSubPercent->setValue(100);
    slotUpdateCaption(100);

    KNotifyClient::event( "SuccessfullyFinished" );
  }
  else {
    m_pixLabel->setPixmap( k3bthememanager->currentTheme()->pixmap( "k3b_progress_dialog_failed" ) );

    m_labelTask->setPaletteForegroundColor( Qt::red );

    if( m_bCanceled )
      m_labelTask->setText( i18n("Canceled!") );
    else
      m_labelTask->setText( i18n("Error!") );

    KNotifyClient::event( "FinishedWithError" );
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
  m_labelTask->setPaletteForegroundColor( k3bthememanager->currentTheme()->foregroundColor() );
  m_debugOutputMap.clear();


  // disconnect from the former job
  if( m_job )
    disconnect( m_job );
  m_job = job;

  if( job ) {

    // connect to all the shit
    connect( job, SIGNAL(infoMessage(const QString&,int)), this, SLOT(slotInfoMessage(const QString&,int)) );

    connect( job, SIGNAL(percent(int)), m_progressPercent, SLOT(setValue(int)) );
    connect( job, SIGNAL(percent(int)), this, SLOT(slotUpdateCaption(int)) );
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
  }
}


void K3bJobProgressDialog::slotCancelButtonPressed()
{
  if( m_job )
    if( KMessageBox::questionYesNo( this, i18n("Do you really want to cancel?"), i18n("Cancel") ) == KMessageBox::Yes ) {
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
  m_timer->start( 1000 );
  m_startTime = QTime::currentTime();
  if( KMainWindow* w = dynamic_cast<KMainWindow*>(kapp->mainWidget()) )
    m_plainCaption = w->caption();
}


void K3bJobProgressDialog::slotUpdateTime()
{
  int elapsed = m_startTime.secsTo( QTime::currentTime() );

  m_labelElapsedTime->setText( i18n("Elapsed time: %1 h").arg( QTime().addSecs(elapsed).toString() ) );
}


void K3bJobProgressDialog::slotDebuggingOutput( const QString& type, const QString& output )
{
  m_debugOutputMap[type].append(output);
}


void K3bJobProgressDialog::slotShowDebuggingOutput()
{
  PrivateDebugWidget debugWidget( m_debugOutputMap, this );
  debugWidget.exec();
}


void K3bJobProgressDialog::slotUpdateCaption( int percent )
{
  if( KMainWindow* w = dynamic_cast<KMainWindow*>(kapp->mainWidget()) )
    w->setPlainCaption( QString( "(%1%) %2" ).arg(percent).arg(m_plainCaption) );
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
    if( m_buttonCancel->isVisible() )
      slotCancelButtonPressed();
    else
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

  in_loop = TRUE;
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


#include "k3bjobprogressdialog.moc"
