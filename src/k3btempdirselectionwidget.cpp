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

#include <qlabel.h>
#include <qgroupbox.h>
#include <qtoolbutton.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qtimer.h>
#include <qhbox.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include <kapplication.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kdialog.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <kdiskfreesp.h>


K3bTempDirSelectionWidget::K3bTempDirSelectionWidget( QWidget *parent, const char *name ) 
  : QGroupBox( 4, Qt::Vertical, i18n( "Temp Directory" ), parent, name )
{
  layout()->setSpacing( KDialog::spacingHint() );
  layout()->setMargin( KDialog::marginHint() );

  QLabel* imageFileLabel = new QLabel( i18n( "Wri&te image file to:" ), this );


  // FIXME: Use KURLRequester!

  QHBox* urlRequesterBox = new QHBox( this );
  urlRequesterBox->setSpacing( KDialog::spacingHint() );
  m_editDirectory = new QLineEdit( urlRequesterBox, "m_editDirectory" );
  m_buttonFindIsoImage = new QToolButton( urlRequesterBox, "m_buttonFindDir" );
  m_buttonFindIsoImage->setIconSet( SmallIconSet( "fileopen" ) );

  imageFileLabel->setBuddy( m_editDirectory );

  QHBox* freeTempSpaceBox = new QHBox( this );
  freeTempSpaceBox->setSpacing( KDialog::spacingHint() );
  (void)new QLabel( i18n( "Free space in temporary directory" ), freeTempSpaceBox, "TextLabel2" );
  m_labelFreeSpace = new QLabel( "                       ",freeTempSpaceBox, "m_labelFreeSpace" );
  m_labelFreeSpace->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

  QHBox* cdSizeBox = new QHBox( this );
  cdSizeBox->setSpacing( KDialog::spacingHint() );
  (void)new QLabel( i18n( "Size of CD" ), cdSizeBox, "TextLabel4" );
  m_labelCdSize = new QLabel( "                        ", cdSizeBox, "m_labelCdSize" );
  m_labelCdSize->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

  m_freeTempSpaceTimer = new QTimer( this );

  connect( m_freeTempSpaceTimer, SIGNAL(timeout()), this, SLOT(slotUpdateFreeTempSpace()) );
  connect( m_buttonFindIsoImage, SIGNAL(clicked()), this, SLOT(slotTempDirButtonPressed()) );
  connect( m_editDirectory, SIGNAL(textChanged(const QString&)), this, SLOT(slotUpdateFreeTempSpace()) );


  m_mode = DIR;

  kapp->config()->setGroup( "General Options" );
  QString tempdir = kapp->config()->readEntry( "Temp Dir", locateLocal( "appdata", "temp/" ) );
  m_editDirectory->setText( tempdir );
  slotUpdateFreeTempSpace();

  m_freeTempSpaceTimer->start( 1000 );


  // ToolTips
  // --------------------------------------------------------------------------------
  QToolTip::add( m_editDirectory, i18n("The directory in which to save the image files") );

  // What's This info
  // --------------------------------------------------------------------------------
  QWhatsThis::add( m_editDirectory, i18n("<p>This is the directory in which K3b will save the <em>image files</em>."
					 "<p>Please make sure that it resides on a partition that has enough free space.") );
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

  if( m_freeTempSpace * 1024 < m_requestedSize )
    m_labelCdSize->setPaletteForegroundColor( red );
  else
    m_labelCdSize->setPaletteForegroundColor( m_labelFreeSpace->paletteForegroundColor() );
}


void K3bTempDirSelectionWidget::slotUpdateFreeTempSpace()
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


void K3bTempDirSelectionWidget::slotTempDirButtonPressed()
{
  QString path;
  if( m_mode == DIR )
    path = KFileDialog::getExistingDirectory( m_editDirectory->text(), this, i18n("Select Temporary Directory") );
  else
    path = KFileDialog::getSaveFileName( m_editDirectory->text(), QString::null, this, i18n("Select Temporary File") );

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
    setTitle( i18n("Temporary directory") );
  else
    setTitle( i18n("Temporary file") );
}


void K3bTempDirSelectionWidget::setNeededSize( unsigned long bytes )
{
  m_requestedSize = bytes;
  m_labelCdSize->setText( QString().sprintf( " %.2f MB", ((float)bytes)/1024.0/1024.0 ) );
}


#include "k3btempdirselectionwidget.moc"
