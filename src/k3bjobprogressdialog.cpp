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


#include "k3bjobprogressdialog.h"
#include <k3bjob.h>
#include <kcutlabel.h>
#include <device/k3bdevice.h>
#include <k3b.h>
#include <k3bstdguiitems.h>

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
#include <qpainter.h>
#include <qregion.h>
#include <qpointarray.h>

#include <kprogress.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <klistview.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <ksystemtray.h>
#include <kdebug.h>
#include <kpixmap.h>
#include <kpixmapeffect.h>
#include <kglobal.h>
#include <knotifyclient.h>
#include <kstandarddirs.h>


class K3bJobProgressDialog::PrivateDebugWidget : public KDialog
{
public:
  PrivateDebugWidget( QMap<QString, QStringList>&, QWidget* parent );
};


K3bJobProgressDialog::PrivateDebugWidget::PrivateDebugWidget( QMap<QString, QStringList>& map, QWidget* parent )
  : KDialog( parent, "debugViewDialog", true )
{
  setCaption( i18n("Debugging output") );

  QPushButton* okButton = new QPushButton( i18n("OK"), this );
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




K3bJobProgressDialog::K3bJobProgressDialog( QWidget* parent, 
					    const char* name, 
					    bool showSubProgress,
					    bool modal, WFlags fl )
  : KDialog( parent, name, modal, fl )
{
  setCaption( i18n("Progress") );

  m_systemTray = new KSystemTray( parent );

  setupGUI();
  setupConnections();

  // FIXME: this is bad hacking (although it should work!)
  // -----
  if( !showSubProgress ) {
    m_progressSubPercent->hide();
  }
  // -----

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
  m_pixLabel->setPaletteBackgroundColor( QColor( 205, 210, 255 ) );
  m_pixLabel->setPixmap( QPixmap(locate( "appdata", "pics/k3bprojectview_left.png" )) );
  m_pixLabel->setScaledContents( FALSE );
  headerLayout->addWidget( m_pixLabel );

  QFrame* frame4 = new QFrame( headerFrame, "frame4" );
  frame4->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)5, 1, 0, frame4->sizePolicy().hasHeightForWidth() ) );
  frame4->setPaletteBackgroundColor( QColor( 205, 210, 255 ) );
  frame4->setFrameShape( QFrame::NoFrame );
  frame4->setFrameShadow( QFrame::Raised );
  QVBoxLayout* frame4Layout = new QVBoxLayout( frame4, 6, 3, "frame4Layout"); 

  m_labelJob = new KCutLabel( frame4, "m_labelJob" );
  QFont m_labelJob_font(  m_labelJob->font() );
  m_labelJob_font.setPointSize( 12 );
  m_labelJob_font.setBold( TRUE );
  m_labelJob->setFont( m_labelJob_font ); 
  m_labelJob->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );
  frame4Layout->addWidget( m_labelJob );

  m_labelJobDetails = new KCutLabel( frame4, "m_labelJobDetails" );
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
  frame5->setPaletteBackgroundColor( QColor( 205, 210, 255 ) );
  frame5->setFrameShape( QFrame::NoFrame );
  frame5->setFrameShadow( QFrame::Raised );
  QVBoxLayout* frame5Layout = new QVBoxLayout( frame5, 6, 3, "frame5Layout"); 

  m_labelTask = new KCutLabel( frame5, "m_labelTask" );
  QFont m_labelTask_font(  m_labelTask->font() );
  m_labelTask_font.setPointSize( 12 );
  m_labelTask_font.setBold( TRUE );
  m_labelTask->setFont( m_labelTask_font ); 
  frame5Layout->addWidget( m_labelTask );

  m_labelElapsedTime = new QLabel( frame5, "m_labelElapsedTime" );
  m_labelElapsedTime->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)5, 0, 1, m_labelElapsedTime->sizePolicy().hasHeightForWidth() ) );
  frame5Layout->addWidget( m_labelElapsedTime );
  progressHeaderLayout->addWidget( frame5 );

  QLabel* pixmapLabel2 = new QLabel( progressHeaderFrame, "pixmapLabel2" );
  pixmapLabel2->setPaletteBackgroundColor( QColor( 205, 210, 255 ) );
  pixmapLabel2->setPixmap( QPixmap(locate( "data", "k3b/pics/k3bprojectview_right.png" )) );
  pixmapLabel2->setScaledContents( FALSE );
  progressHeaderLayout->addWidget( pixmapLabel2 );
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

  m_buttonCancel = new QPushButton( i18n("Cancel"), this, "m_buttonCancel" );
  layout5->addWidget( m_buttonCancel );
  m_buttonClose = new QPushButton( i18n("Close"), this );
  layout5->addWidget( m_buttonClose );
  m_buttonShowDebug = new QPushButton( i18n("Show debugging output"), this );
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
  KConfig* c = kapp->config();
  c->setGroup( "General Options");

  m_bShowSystemTrayProgress = c->readBoolEntry( "Show progress in system tray", true );
  if( m_bShowSystemTrayProgress ) {
    m_systemTray->show();
  }

  if( c->readBoolEntry( "hide main window while writing", false ) )
    k3bMain()->hide();

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
    k3bMain()->show();

    QToolTip::remove( m_systemTray );
    QToolTip::add( m_systemTray, i18n("Ready") );
    m_systemTray->hide();

    if( !m_plainCaption.isEmpty() )
      k3bMain()->setPlainCaption( m_plainCaption );
  }
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


void K3bJobProgressDialog::slotFinished( bool success )
{
  kdDebug() << "(K3bJobProgressDialog) received finished signal!" << endl;

  m_job = 0;

  if( success ) {
    m_pixLabel->setPixmap( QPixmap(locate( "appdata", "pics/k3b_progress_dialog_success.png" )) );

    m_labelTask->setText( i18n("Success!") );
    m_labelTask->setPaletteForegroundColor( Qt::darkGreen );
    m_labelSubTask->setText( QString::null );

    m_progressPercent->setValue(100);
    m_progressSubPercent->setValue(100);

    KNotifyClient::event( "SuccessfullyFinished" );
  }
  else {
    m_pixLabel->setPixmap( QPixmap(locate( "appdata", "pics/k3b_progress_dialog_failed.png" )) );

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
  m_viewInfo->clear();
  m_progressPercent->setValue(0);
  m_progressSubPercent->setValue(0);
  m_labelTask->setText("");
  m_labelSubTask->setText("");
  m_labelProcessedSize->setText("");
  m_labelSubProcessedSize->setText("");
  m_labelTask->setPaletteForegroundColor( paletteForegroundColor() );
  m_debugOutputMap.clear();


  // disconnect from the former job
  if( m_job )
    disconnect( m_job );
  m_job = job;

  if( job ) {

    // connect to all the shit
    connect( job, SIGNAL(infoMessage(const QString&,int)), this, SLOT(slotInfoMessage(const QString&,int)) );
    
    connect( job, SIGNAL(percent(int)), m_progressPercent, SLOT(setValue(int)) );
    connect( job, SIGNAL(percent(int)), this, SLOT(animateSystemTray(int)) );
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
  }
}


void K3bJobProgressDialog::slotCancelButtonPressed()
{
  if( m_job )
    if( KMessageBox::questionYesNo( this, i18n("Do you really want to cancel?"), i18n("Cancel") ) == KMessageBox::Yes ) {
      if( m_job )
	m_job->cancel();
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

  if( m_bShowSystemTrayProgress ) {
    QToolTip::remove( m_systemTray );
    QToolTip::add( m_systemTray, name );
  }
}


void K3bJobProgressDialog::slotStarted()
{
  m_timer->start( 1000 );
  m_startTime = QTime::currentTime();
  m_lastAnimatedProgress = -1;
  m_plainCaption = k3bMain()->caption();
  animateSystemTray( 0 );
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


void K3bJobProgressDialog::animateSystemTray( int percent )
{
  if( m_bShowSystemTrayProgress ) {
    if( m_lastAnimatedProgress < percent ) {
      m_lastAnimatedProgress = percent;

      static KPixmap logo = kapp->iconLoader()->loadIcon( "k3b", KIcon::Panel, 24 );
      if( logo.height() != 25 )
	logo.resize( 25, 25 ); // much better to handle since 4*25=100 ;)

      if( percent == 100 )
	m_systemTray->setPixmap( logo );

      KPixmap darkLogo( logo );
      //      KPixmapEffect::intensity( darkLogo, -0.8 );
      KPixmapEffect::toGray( darkLogo );

      QPointArray pa(7);
      int size = 7;
      pa.setPoint( 0, 13, 0 );  // always the first point
      // calculate the second point
      // if percent > 13 it is the upper right edge
      if( percent > 13 ) {

	// upper right edge
	pa.setPoint( 1, 25, 0 );
	if( percent > 38 ) {

	  // lower right edge
	  pa.setPoint( 2, 25, 25 );
	  if( percent > 38+25 ) {

	    // lower left edge
	    pa.setPoint( 3, 0, 25 );
	    if( percent > 38+25+25 ) {

	      // upper left edge
	      pa.setPoint( 4, 0, 0 );
	      pa.setPoint( 5, percent - (38+25+25), 0 );
	      size = 7;
	    }
	    else {
	      pa.setPoint( 4, 0, 25 - (percent - (38+25)) );
	      size = 6;
	    }
	  }
	  else {
	    pa.setPoint( 3, 25 - (percent-38), 25 );
	    size = 5;
	  }
	}
	else {
	  pa.setPoint( 2, 25, percent-13 );
	  size = 4;
	}
      }
      else {
	pa.setPoint( 1, percent == 0 ? 13 : 12+percent, 0 );
	size = 3;
      }

      pa.setPoint( size-1, 13, 13 );
      pa.resize( size );


      //       for( int i = 0; i < pa.size(); ++i ) {
      // 	printf("(%i, %i) ", pa.point(i).x(), pa.point(i).y() );
      //       }
      //       printf("\n");

      QPainter p( &darkLogo );

      p.setClipRegion( QRegion( pa ) );

      p.drawPixmap( 0, 0, logo );
      p.end();

      m_systemTray->setPixmap( darkLogo );
    }
  }
}


void K3bJobProgressDialog::slotUpdateCaption( int percent )
{
  k3bMain()->setPlainCaption( QString( "(%1%) %2" ).arg(percent).arg(m_plainCaption) );
}


void K3bJobProgressDialog::keyPressEvent( QKeyEvent *e )
{
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

  e->accept();
}
