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
#include <qdir.h>
#include <qtimer.h>

#include <klocale.h>
#include <kconfig.h>
#include <kstddirs.h>
#include <kfiledialog.h>
#include "kdiskfreesp.h"


K3bProjectBurnDialog::K3bProjectBurnDialog(K3bDoc* doc, QWidget *parent, const char *name, bool modal )
  : KDialogBase( KDialogBase::Tabbed, i18n("Write CD"), User1|Ok|Cancel, Ok, parent, name, modal, true, i18n("Write") )
{
  m_doc = doc;
	
  setButtonBoxOrientation( Vertical );

  m_groupWriter = 0;
  m_groupTempDir = 0;
  m_labelCdSize = 0;
  m_labelFreeSpace = 0;
  m_freeTempSpace = 0;
}


QGroupBox* K3bProjectBurnDialog::writerBox( QWidget* parent )
{
  if( m_groupWriter == 0 && parent != 0)
    {
      // --- setup device group ----------------------------------------------------
      m_groupWriter = new QGroupBox( parent, "m_groupWriter" );
      m_groupWriter->setTitle( i18n( "Burning Device" ) );
      m_groupWriter->setColumnLayout(0, Qt::Vertical );
      m_groupWriter->layout()->setSpacing( 0 );
      m_groupWriter->layout()->setMargin( 0 );
      QGridLayout* m_groupWriterLayout = new QGridLayout( m_groupWriter->layout() );
      m_groupWriterLayout->setAlignment( Qt::AlignTop );
      m_groupWriterLayout->setSpacing( spacingHint() );
      m_groupWriterLayout->setMargin( marginHint() );

      QLabel* TextLabel1 = new QLabel( m_groupWriter, "TextLabel1" );
      TextLabel1->setText( i18n( "Burning Speed" ) );

      m_groupWriterLayout->addWidget( TextLabel1, 0, 1 );

      m_comboSpeed = new QComboBox( FALSE, m_groupWriter, "m_comboSpeed" );
      m_comboSpeed->setAutoMask( FALSE );
      m_comboSpeed->setDuplicatesEnabled( FALSE );

      m_groupWriterLayout->addWidget( m_comboSpeed, 1, 1 );

      m_comboWriter = new QComboBox( FALSE, m_groupWriter, "m_comboWriter" );

      m_groupWriterLayout->addWidget( m_comboWriter, 1, 0 );

      QLabel* TextLabel1_2 = new QLabel( m_groupWriter, "TextLabel1_2" );
      TextLabel1_2->setText( i18n( "Device" ) );

      m_groupWriterLayout->addWidget( TextLabel1_2, 0, 0 );

      m_groupWriterLayout->setColStretch( 0 , 1);
      // --------------------------------------------------------- device group ---

      connect( m_comboWriter, SIGNAL(activated(int)), this, SLOT(slotRefreshWriterSpeeds()) );
      connect( m_comboWriter, SIGNAL(activated(int)), this, SIGNAL(writerChanged()) );
    }
  
  return m_groupWriter;
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

      m_groupTempDirLayout->setColStretch( 1 , 1);

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
  if( burn )
    actionButton(User1)->show();
  else
    actionButton(User1)->hide();
		
  return QDialog::exec();
}


void K3bProjectBurnDialog::slotOk()
{
  saveSettings();
  done( Saved );
}


void K3bProjectBurnDialog::slotCancel()
{
  done( Canceled );
}

void K3bProjectBurnDialog::slotUser1()
{
  saveSettings();
  done( Burn );
}

void K3bProjectBurnDialog::slotRefreshWriterSpeeds()
{
  if( K3bDevice* _dev = writerDevice() ) {
    // add speeds to combobox
    m_comboSpeed->clear();
    m_comboSpeed->insertItem( "1x" );
    int _speed = 2;
    while( _speed <= _dev->maxWriteSpeed ) {
      m_comboSpeed->insertItem( QString( "%1x" ).arg(_speed) );
      _speed+=2;
    }
  }
}

K3bDevice* K3bProjectBurnDialog::writerDevice() const
{
  const QString s = m_comboWriter->currentText();

  QString strDev = s.mid( s.find('(') + 1, s.find(')') - s.find('(') - 1 );
 
  K3bDevice* dev =  k3bMain()->deviceManager()->deviceByName( strDev );
  if( !dev )
    qDebug( "(K3bProjectBurnDialog) could not find device " + s );
		
  return dev;
}

int K3bProjectBurnDialog::writerSpeed() const
{
  QString _strSpeed = m_comboSpeed->currentText();
  _strSpeed.truncate( _strSpeed.find('x') );
	
  return _strSpeed.toInt();
}

void K3bProjectBurnDialog::readSettings()
{
  if( m_groupWriter ) {
    // -- read cd-writers ----------------------------------------------
    QList<K3bDevice> _devices = k3bMain()->deviceManager()->burningDevices();
    K3bDevice* _dev = _devices.first();
    while( _dev ) {
      m_comboWriter->insertItem( _dev->vendor + " " + _dev->description + " (" + _dev->devicename + ")" );
      _dev = _devices.next();
    }
	
    slotRefreshWriterSpeeds();
    
    // -- reading current speed --------------------------------------
    int _index = 0;
    QString _strSpeed = QString::number(m_doc->speed()) + "x";
    
    for( int i = 0; i < m_comboSpeed->count(); i++ )
      if( m_comboSpeed->text( i ) == _strSpeed )
	_index = i;
    
    m_comboSpeed->setCurrentItem( _index );
  }

  // read temp dir
  if( m_groupTempDir ) {
    k3bMain()->config()->setGroup( "General Options" );
    QString tempdir = k3bMain()->config()->readEntry( "Temp Dir", locateLocal( "appdata", "temp/" ) );
    m_editDirectory->setText( tempdir + "image.iso" );
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
  }
}


void K3bProjectBurnDialog::slotTempDirButtonPressed()
{
  // TODO: ask for confirmation if already exists
  QString dir = KFileDialog::getExistingDirectory( m_editDirectory->text(), k3bMain(), "Select Temp Directory" );
  if( !dir.isEmpty() ) {
    setTempDir( dir );
  }
}


void K3bProjectBurnDialog::setTempDir( const QString& dir )
{
  m_editDirectory->setText( dir );
}


QString K3bProjectBurnDialog::tempDir() const
{
  if( m_groupTempDir )
    return m_editDirectory->text();
  else
    return "";
}
