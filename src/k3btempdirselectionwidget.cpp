/***************************************************************************
                          k3btempdirselectionwidget.cpp  -  description
                             -------------------
    begin                : Sun Mar 24 2002
    copyright            : (C) 2002 by Sebastian Trueg
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

#include "k3btempdirselectionwidget.h"
#include "k3b.h"

#include <qlabel.h>
#include <qgroupbox.h>
#include <qtoolbutton.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qtimer.h>

#include <klocale.h>
#include <kfiledialog.h>
#include <kdialog.h>
#include <kstandarddirs.h>

#include "kdiskfreesp.h"


K3bTempDirSelectionWidget::K3bTempDirSelectionWidget( QWidget *parent, const char *name ) 
  : QWidget( parent, name )
{
  QVBoxLayout* mainLayout = new QVBoxLayout( this );
  mainLayout->setMargin( 0 );
  mainLayout->setAutoAdd( true );

  // setup temp dir selection
  // -----------------------------------------------
  m_groupTempDir = new QGroupBox( this, "m_groupTempDir" );
  m_groupTempDir->setFrameShape( QGroupBox::Box );
  m_groupTempDir->setFrameShadow( QGroupBox::Sunken );
  m_groupTempDir->setTitle( i18n( "Temp directory" ) );
  m_groupTempDir->setColumnLayout(0, Qt::Vertical );
  m_groupTempDir->layout()->setSpacing( 0 );
  m_groupTempDir->layout()->setMargin( 0 );
  QGridLayout* m_groupTempDirLayout = new QGridLayout( m_groupTempDir->layout() );
  m_groupTempDirLayout->setAlignment( Qt::AlignTop );
  m_groupTempDirLayout->setSpacing( KDialog::spacingHint() );
  m_groupTempDirLayout->setMargin( KDialog::marginHint() );

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


  m_mode = DIR;

  k3bMain()->config()->setGroup( "General Options" );
  QString tempdir = k3bMain()->config()->readEntry( "Temp Dir", locateLocal( "appdata", "temp/" ) );
  m_editDirectory->setText( tempdir );
  slotUpdateFreeTempSpace();

  m_freeTempSpaceTimer->start( 1000 );
}


K3bTempDirSelectionWidget::~K3bTempDirSelectionWidget()
{
}


void K3bTempDirSelectionWidget::slotFreeTempSpace(const QString&, 
						  unsigned long, 
						  unsigned long, 
						  unsigned long kbAvail)
{
  m_labelFreeSpace->setText( QString().sprintf( "%.2f MB", (float)kbAvail/1024.0 ) );

  m_freeTempSpace = kbAvail;
}


void K3bTempDirSelectionWidget::slotUpdateFreeTempSpace()
{
  QString path = m_editDirectory->text();
  qDebug("(K3bProjectBurnDialg) Check free disk space");
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


void K3bTempDirSelectionWidget::slotTempDirButtonPressed()
{
  QString path;
  if( m_mode == DIR )
    path = KFileDialog::getExistingDirectory( m_editDirectory->text(), k3bMain(), i18n("Select temp directory") );
  else
    path = KFileDialog::getSaveFileName( m_editDirectory->text(), QString::null, k3bMain(), i18n("Select temp file") );

  if( !path.isEmpty() ) {
    setTempPath( path );
  }
}


void K3bTempDirSelectionWidget::setTempPath( const QString& dir )
{
  m_editDirectory->setText( dir );
  slotUpdateFreeTempSpace();
}


QString K3bTempDirSelectionWidget::tempPath() const
{
  return m_editDirectory->text();
}


void K3bTempDirSelectionWidget::setSelectionMode( int mode )
{
  m_mode = mode;

  if( m_mode == DIR )
    m_groupTempDir->setTitle( i18n("Temp directory") );
  else
    m_groupTempDir->setTitle( i18n("Temp file") );
}


void K3bTempDirSelectionWidget::setNeededSize( unsigned long bytes )
{
  m_labelCdSize->setText( QString().sprintf( " %.2f MB", ((float)bytes)/1024.0/1024.0 ) );
}


#include "k3btempdirselectionwidget.moc"
