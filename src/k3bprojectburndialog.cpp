/***************************************************************************
                          k3bprojectburndialog.cpp  -  description
                             -------------------
    begin                : Thu May 17 2001
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

#include "k3bprojectburndialog.h"
#include "k3b.h"
#include "k3bdoc.h"
#include "device/k3bdevice.h"
#include "device/k3bdevicemanager.h"
#include "k3bwriterselectionwidget.h"
#include "k3bburnprogressdialog.h"
#include "k3bjob.h"

#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qtoolbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qstring.h>
#include <qlineedit.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qtimer.h>

#include <klocale.h>
#include <kconfig.h>
#include <kstddirs.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include "kdiskfreesp.h"


K3bProjectBurnDialog::K3bProjectBurnDialog(K3bDoc* doc, QWidget *parent, const char *name, bool modal )
  : KDialogBase( KDialogBase::Tabbed, i18n("Write CD"), User1|User2|Cancel, 
		 User1, parent, name, modal, true, i18n("Write"), i18n("Save") )
{
  m_doc = doc;
	
  setButtonBoxOrientation( Vertical );

  m_writerSelectionWidget = 0;
  m_groupTempDir = 0;
  m_labelCdSize = 0;
  m_labelFreeSpace = 0;
  m_freeTempSpace = 0;
  m_job = 0;
}


QGroupBox* K3bProjectBurnDialog::writerBox( QWidget* parent )
{
  if( m_writerSelectionWidget == 0 && parent != 0)
    {
      m_writerSelectionWidget = new K3bWriterSelectionWidget( parent );

      connect( m_writerSelectionWidget, SIGNAL(writerChanged()), this, SIGNAL(writerChanged()) );
    }
  
  return m_writerSelectionWidget;
}


QGroupBox* K3bProjectBurnDialog::tempDirBox( QWidget* parent )
{
  if( !m_groupTempDir && parent != 0 )
    {
      // setup temp dir selection
      // -----------------------------------------------
      m_groupTempDir = new QGroupBox( parent, "m_groupTempDir" );
      m_groupTempDir->setFrameShape( QGroupBox::Box );
      m_groupTempDir->setFrameShadow( QGroupBox::Sunken );
      m_groupTempDir->setTitle( i18n( "Temp Directory" ) );
      m_groupTempDir->setColumnLayout(0, Qt::Vertical );
      m_groupTempDir->layout()->setSpacing( 0 );
      m_groupTempDir->layout()->setMargin( 0 );
      QGridLayout* m_groupTempDirLayout = new QGridLayout( m_groupTempDir->layout() );
      m_groupTempDirLayout->setAlignment( Qt::AlignTop );
      m_groupTempDirLayout->setSpacing( spacingHint() );
      m_groupTempDirLayout->setMargin( marginHint() );

      QLabel* TextLabel1_3 = new QLabel( m_groupTempDir, "TextLabel1_3" );
      TextLabel1_3->setText( i18n( "Write Image file to" ) );

      m_groupTempDirLayout->addWidget( TextLabel1_3, 0, 0 );

      QLabel* TextLabel2 = new QLabel( m_groupTempDir, "TextLabel2" );
      TextLabel2->setText( i18n( "Free space on device" ) );

      m_groupTempDirLayout->addWidget( TextLabel2, 2, 0 );

      QLabel* TextLabel4 = new QLabel( m_groupTempDir, "TextLabel4" );
      TextLabel4->setText( i18n( "Size of CD" ) );

      m_groupTempDirLayout->addWidget( TextLabel4, 3, 0 );

      m_labelCdSize = new QLabel( m_groupTempDir, "m_labelCdSize" );
      m_labelCdSize->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

      m_groupTempDirLayout->addMultiCellWidget( m_labelCdSize, 3, 3, 1, 2 );

      m_labelFreeSpace = new QLabel( m_groupTempDir, "m_labelFreeSpace" );
      m_labelFreeSpace->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

      m_groupTempDirLayout->addMultiCellWidget( m_labelFreeSpace, 2, 2, 1, 2 );

      m_editDirectory = new QLineEdit( m_groupTempDir, "m_editDirectory" );

      m_groupTempDirLayout->addMultiCellWidget( m_editDirectory, 1, 1, 0, 1 );

      m_buttonFindIsoImage = new QToolButton( m_groupTempDir, "m_buttonFindDir" );
      m_buttonFindIsoImage->setText( i18n( "..." ) );

      m_groupTempDirLayout->addWidget( m_buttonFindIsoImage, 1, 2 );

      m_groupTempDirLayout->setColStretch( 1, 1 );

      m_freeTempSpaceTimer = new QTimer( this );

      connect( m_freeTempSpaceTimer, SIGNAL(timeout()), this, SLOT(slotUpdateFreeTempSpace()) );
      connect( m_buttonFindIsoImage, SIGNAL(clicked()), this, SLOT(slotTempDirButtonPressed()) );
      connect( m_editDirectory, SIGNAL(textChanged(const QString&)), this, SLOT(slotUpdateFreeTempSpace()) );

      m_freeTempSpaceTimer->start( 1000 );
    }
  
  return m_groupTempDir;
}


K3bProjectBurnDialog::~K3bProjectBurnDialog(){
}


int K3bProjectBurnDialog::exec( bool burn )
{
  if( burn && m_job == 0 ) {
    showButton(User1, true );
    actionButton(User1)->setDefault(true);
    actionButton(User2)->setDefault(false);
    actionButton(User2)->clearFocus();
  }
  else {
    showButton(User1, false );
    actionButton(User2)->setDefault(false);
    actionButton(User1)->setDefault(true);
  }


  readSettings();
		
  return KDialogBase::exec();
}


void K3bProjectBurnDialog::slotUser2()
{
  saveSettings();
  m_doc->updateAllViews(0);
  done( Saved );
}


void K3bProjectBurnDialog::slotCancel()
{
  done( Canceled );
}

void K3bProjectBurnDialog::slotUser1()
{
  if( m_job ) {
    KMessageBox::sorry( k3bMain(), i18n("Sorry, K3b is already working on this project!"), i18n("Sorry") );
    return;
  }

  saveSettings();

  // avoid interaction with the document while the job is working
  m_doc->disable();

  m_job = m_doc->newBurnJob();

  // will be deleted at close (WDestructiveClose)
  K3bBurnProgressDialog* d = new K3bBurnProgressDialog( k3bMain() );
  connect( d, SIGNAL(closed()), this, SLOT(slotJobFinished()) );
  d->setJob( m_job );

  hide();

  d->show();
  m_job->start();

  done( Burn );
}


void K3bProjectBurnDialog::slotJobFinished()
{
  m_doc->enable();

  delete m_job;
  m_job = 0;
}


K3bDevice* K3bProjectBurnDialog::writerDevice() const
{
  if( m_writerSelectionWidget )
    return m_writerSelectionWidget->writerDevice();
}

int K3bProjectBurnDialog::writerSpeed() const
{
  if( m_writerSelectionWidget )	
    return m_writerSelectionWidget->writerSpeed();
}

void K3bProjectBurnDialog::readSettings()
{
  // read temp dir
  if( m_groupTempDir ) {
    k3bMain()->config()->setGroup( "General Options" );
    QString tempdir = k3bMain()->config()->readEntry( "Temp Dir", locateLocal( "appdata", "temp/" ) );
    m_editDirectory->setText( tempdir );
    slotUpdateFreeTempSpace();
    m_labelCdSize->setText( QString().sprintf( " %.2f MB", ((float)doc()->size())/1024.0/1024.0 ) );
  }
}


void K3bProjectBurnDialog::slotFreeTempSpace(const QString&, unsigned long, unsigned long, unsigned long kbAvail)
{
  if( m_labelFreeSpace )
    m_labelFreeSpace->setText( QString().sprintf( "%.2f MB", (float)kbAvail/1024.0 ) );

  m_freeTempSpace = kbAvail;
}


void K3bProjectBurnDialog::slotUpdateFreeTempSpace()
{
  QString path = m_editDirectory->text();
  
  if( QFile::exists( path ) ) {
    connect( KDiskFreeSp::findUsageInfo( path ), 
	     SIGNAL(foundMountPoint(const QString&, unsigned long, unsigned long, unsigned long)),
	     this, SLOT(slotFreeTempSpace(const QString&, unsigned long, unsigned long, unsigned long)) );
  }
  else {
    path.truncate( path.findRev( '/' ) );
    if( QFile::exists( path ) )
      connect( KDiskFreeSp::findUsageInfo( path ), 
	       SIGNAL(foundMountPoint(const QString&, unsigned long, unsigned long, unsigned long)),
	       this, SLOT(slotFreeTempSpace(const QString&, unsigned long, unsigned long, unsigned long)) );    
    else
      m_labelFreeSpace->setText( "-" );
  }
}


void K3bProjectBurnDialog::slotTempDirButtonPressed()
{
  QString dir = KFileDialog::getExistingDirectory( m_editDirectory->text(), k3bMain(), i18n("Select Temp Directory") );
  if( !dir.isEmpty() ) {
    setTempDir( dir );
  }
}


void K3bProjectBurnDialog::setTempDir( const QString& dir )
{
  m_editDirectory->setText( dir );
}


QString K3bProjectBurnDialog::tempPath() const
{
  if( m_groupTempDir ) {
    return m_editDirectory->text();
  }
  else
    return "";
}


QString K3bProjectBurnDialog::tempDir() const
{
  if( m_groupTempDir ) {
    QString dir( m_editDirectory->text() );

    QFileInfo info(dir);
    if( info.exists() && info.isDir() ) {
      if( dir.at(dir.length()-1) != '/' )
	dir.append("/");
      return dir;
    }
    else {
      dir.truncate( dir.findRev('/')+1 );
      return dir;
    }
  }
  else
    return "";
}


#include "k3bprojectburndialog.moc"
