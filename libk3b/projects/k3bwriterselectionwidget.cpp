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


#include "k3bwriterselectionwidget.h"

#include <k3bdevicecombobox.h>
#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bglobals.h>
#include <k3bcore.h>

#include <klocale.h>
#include <kdialog.h>
#include <kconfig.h>
#include <kcombobox.h>
#include <kmessagebox.h>
#include <kiconloader.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qtooltip.h>
#include <qtoolbutton.h>
#include <qwhatsthis.h>
#include <qmap.h>
#include <qptrvector.h>
#include <qcursor.h>
#include <qapplication.h>
#include <qmap.h>


class K3bWriterSelectionWidget::Private
{
public:
  bool dvd;
  bool forceAutoSpeed;

  QMap<int, int> indexSpeedMap;
  QMap<int, int> speedIndexMap;
};


K3bWriterSelectionWidget::K3bWriterSelectionWidget( bool dvd, QWidget *parent, const char *name )
  : QWidget( parent, name )
{
  d = new Private;
  d->dvd = dvd;
  d->forceAutoSpeed = false;

  QGroupBox* groupWriter = new QGroupBox( this );
  groupWriter->setTitle( i18n( "Burning Device" ) );
  groupWriter->setColumnLayout(0, Qt::Vertical );
  groupWriter->layout()->setSpacing( 0 );
  groupWriter->layout()->setMargin( 0 );

  QGridLayout* groupWriterLayout = new QGridLayout( groupWriter->layout() );
  groupWriterLayout->setAlignment( Qt::AlignTop );
  groupWriterLayout->setSpacing( KDialog::spacingHint() );
  groupWriterLayout->setMargin( KDialog::marginHint() );

  QLabel* labelSpeed = new QLabel( groupWriter, "TextLabel1" );
  labelSpeed->setText( i18n( "Speed:" ) );

  m_comboSpeed = new KComboBox( FALSE, groupWriter, "m_comboSpeed" );
  m_comboSpeed->setAutoMask( FALSE );
  m_comboSpeed->setDuplicatesEnabled( FALSE );

  m_comboWriter = new K3bDeviceComboBox( groupWriter, "m_comboWriter" );

  m_buttonDetermineSpeed = new QToolButton( groupWriter );
  m_buttonDetermineSpeed->setIconSet( SmallIconSet( "reload" ) );

  m_writingAppLabel = new QLabel( i18n("Writing app:"), groupWriter );
  m_comboWritingApp = new KComboBox( groupWriter );

  groupWriterLayout->addWidget( m_comboWriter, 0, 0 );
  groupWriterLayout->addWidget( labelSpeed, 0, 1 );
  groupWriterLayout->addWidget( m_comboSpeed, 0, 2 );
  groupWriterLayout->addWidget( m_buttonDetermineSpeed, 0, 3 );
  groupWriterLayout->addWidget( m_writingAppLabel, 0, 4 );
  groupWriterLayout->addWidget( m_comboWritingApp, 0, 5 );
  groupWriterLayout->setColStretch( 0, 1 );


  QGridLayout* mainLayout = new QGridLayout( this );
  mainLayout->setAlignment( Qt::AlignTop );
  mainLayout->setSpacing( KDialog::spacingHint() );
  mainLayout->setMargin( 0 );

  mainLayout->addWidget( groupWriter, 0, 0 );


  connect( m_comboWriter, SIGNAL(selectionChanged(K3bDevice::Device*)), this, SIGNAL(writerChanged()) );
  connect( m_comboWritingApp, SIGNAL(activated(int)), this, SLOT(slotWritingAppSelected(int)) );
  connect( this, SIGNAL(writerChanged()), SLOT(slotWriterChanged()) );
  connect( m_buttonDetermineSpeed, SIGNAL(clicked()), this, SLOT(slotDetermineSupportedWriteSpeeds()) );
  connect( m_comboSpeed, SIGNAL(activated(int)), this, SLOT(slotSpeedChanged(int)) );

  QToolTip::add( m_buttonDetermineSpeed, i18n("Determine supported writing speeds") );
  QWhatsThis::add( m_buttonDetermineSpeed, i18n("<p>Normally K3b presents static list of writing speeds "
						"that is only based on the maximum writing speed of the "
						"device."
						"<p>If this button is clicked K3b tries to determine the "
						"writing speeds supported with the mounted media."
						"<p>Be aware that this only works with MMC3 compliant "
						"writers.") );
  init();

  slotWriterChanged();
}


K3bWriterSelectionWidget::~K3bWriterSelectionWidget()
{
  delete d;
}


void K3bWriterSelectionWidget::setDvd( bool b )
{
  if( b != d->dvd ) {
    d->dvd = true;
    init();
  }
}

void K3bWriterSelectionWidget::init()
{
  m_comboWriter->clear();

  // -- read cd-writers ----------------------------------------------
  QPtrList<K3bDevice::Device>& devices = ( d->dvd 
				   ? k3bcore->deviceManager()->dvdWriter() 
				   : k3bcore->deviceManager()->cdWriter() );

  K3bDevice::Device* dev = devices.first();
  while( dev ) {
    m_comboWriter->addDevice( dev );
    dev = devices.next();
  }

  k3bcore->config()->setGroup( "General Options" );
  K3bDevice::Device *current = k3bcore->deviceManager()->deviceByName( k3bcore->config()->readEntry( "current_writer" ) );

  if ( current == 0 )
    current = devices.first();
  setWriterDevice( current );
  
  slotRefreshWriterSpeeds();
  
  slotConfigChanged(k3bcore->config());

  if( d->dvd )
    setSupportedWritingApps( K3b::GROWISOFS );
  else
    setSupportedWritingApps( K3b::CDRDAO|K3b::CDRECORD );


  // ToolTips
  // --------------------------------------------------------------------------------
  QToolTip::remove( m_comboWriter );
  QToolTip::remove( m_comboSpeed );
  QToolTip::remove( m_comboWritingApp );

  if( d->dvd ) {
    QToolTip::add( m_comboWriter, i18n("The DVD writer that will write the DVD") );
    QToolTip::add( m_comboSpeed, i18n("The speed at which to write the DVD") );
    QToolTip::add( m_comboWritingApp, i18n("The external application to actually write the DVD") );
  }
  else {
    QToolTip::add( m_comboWriter, i18n("The CD writer that will write the CD") );
    QToolTip::add( m_comboSpeed, i18n("The speed at which to write the CD") );
    QToolTip::add( m_comboWritingApp, i18n("The external application to actually write the CD") );
  }

  // What's This info
  // --------------------------------------------------------------------------------
  QWhatsThis::remove( m_comboWriter );
  QWhatsThis::remove( m_comboSpeed );
  QWhatsThis::remove( m_comboWritingApp );

  if( d->dvd ) {
    QWhatsThis::add( m_comboWriter, i18n("<p>Select the DVD writer that you want to use."
					 "<p>In most cases there will only be one writer available which "
					 "does not leave much choice.") );
    QWhatsThis::add( m_comboSpeed, i18n("<p>Select the speed with which you want the writer to burn."
					"<p><b>Auto</b><br>"
					"This will choose the maximum writing speed possible with the used "
					"medium. "
					"This is the recommended selection for most media.</p>"
					"<p><b>Ignore</b><br>"
					"This will leave the speed selection to the writer device. "
					"Use this if K3b is unable to set the writing speed."
					"<p>1x refers to 1385 KB/s.</p>") );
  }
  else {
    QWhatsThis::add( m_comboWriter, i18n("<p>Select the CD writer that you want to use."
					 "<p>In most cases there will only be one writer available which "
					 "does not leave much choice.") );
    QWhatsThis::add( m_comboSpeed, i18n("<p>Select the speed with which you want the writer to burn."
					"<p><b>Auto</b><br>"
					"This will choose the maximum writing speed possible with the used "
					"medium. Be aware that this might not be a good choice when writing "
					"an Audio-CD on-the-fly since the decoding of the audio files may take "
					"too long to allow a continuous data stream."
					"<p>1x refers to 175 KB/s."
					"<p><b>Caution:</b> Make sure your system is able to send the data "
					"fast enough to prevent buffer underruns.") );
  }
  QWhatsThis::add( m_comboWritingApp, i18n("<p>K3b uses the command line tools cdrecord, growisofs, and cdrdao "
					   "to actually write a CD or DVD."
					   "<p>Normally K3b chooses the best "
					   "suited application for every task automatically but in some cases it "
					   "may be possible that one of the applications does not work as intended "
					   "with a certain writer. In this case one may select the "
					   "application manually.") );
}


void K3bWriterSelectionWidget::slotConfigChanged( KConfig* c )
{
  QString oldGroup = c->group();
  c->setGroup("General Options");
  bool manualAppSelect = c->readBoolEntry( "Manual writing app selection", false );
  c->setGroup( oldGroup );
  if( manualAppSelect ) {
    m_comboWritingApp->show();
    m_writingAppLabel->show();
  }
  else {
    m_comboWritingApp->hide();
    m_writingAppLabel->hide();
  }
}


void K3bWriterSelectionWidget::slotRefreshWriterSpeeds()
{
  clearSpeedCombo();
  m_comboSpeed->insertItem( i18n("Auto") );
  if( d->dvd )
    m_comboSpeed->insertItem( i18n("Ignore") );
  if( !d->forceAutoSpeed ) {
    if( K3bDevice::Device* dev = writerDevice() ) {
      // add speeds to combobox
      int i = 1;
      int speed = ( d->dvd ? 1385 : 175 );
      while( i*speed <= dev->maxWriteSpeed() ) {
	insertSpeedItem( i*speed );
	i = ( i == 1 ? 2 : i+2 );
      }
    }
  }
}


void K3bWriterSelectionWidget::clearSpeedCombo()
{
  m_comboSpeed->clear();
  d->indexSpeedMap.clear();
  d->speedIndexMap.clear();
}


void K3bWriterSelectionWidget::insertSpeedItem( int speed )
{
  if( !d->speedIndexMap.contains( speed ) ) {
    d->indexSpeedMap[m_comboSpeed->count()] = speed;
    d->speedIndexMap[speed] = m_comboSpeed->count();

    if( d->dvd )
      m_comboSpeed->insertItem( ( speed%1385 > 0
				? QString::number( (float)speed/1385.0, 'f', 1 )  // example: DVD+R(W): 2.4x
				: QString::number( speed/1385 ) ) 
			      + "x" );
    else
      m_comboSpeed->insertItem( QString("%1x").arg(speed/175) );
  }
}


void K3bWriterSelectionWidget::slotWritingAppSelected( int )
{
  emit writingAppChanged( selectedWritingApp() );
}


K3bDevice::Device* K3bWriterSelectionWidget::writerDevice() const
{
  return m_comboWriter->selectedDevice();
}


void K3bWriterSelectionWidget::setWriterDevice( K3bDevice::Device* dev )
{
  m_comboWriter->setSelectedDevice( dev );
}


void K3bWriterSelectionWidget::setSpeed( int s )
{
  if( d->dvd && s < 0 )
    m_comboSpeed->setCurrentItem( 1 ); // Ignore
  else if( d->speedIndexMap.contains( s ) )
    m_comboSpeed->setCurrentItem( d->speedIndexMap[s] );
  else
    m_comboSpeed->setCurrentItem( 0 ); // Auto
}


void K3bWriterSelectionWidget::setWritingApp( int app )
{
  switch( app ) {
  case K3b::CDRECORD:
    m_comboWritingApp->setCurrentItem( "cdrecord" );
    break;
  case K3b::CDRDAO:
    m_comboWritingApp->setCurrentItem( "cdrdao" );
    break;
  case K3b::DVDRECORD:
    m_comboWritingApp->setCurrentItem( "dvdrecord" );
    break;
  case K3b::GROWISOFS:
    m_comboWritingApp->setCurrentItem( "growisofs" );
    break;
  case K3b::DVD_RW_FORMAT:
    m_comboWritingApp->setCurrentItem( "dvd+rw-format" );
    break;
  default:
    m_comboWritingApp->setCurrentItem( 0 );  // Auto
    break;
  }
}


int K3bWriterSelectionWidget::writerSpeed() const
{
  if( m_comboSpeed->currentItem() == 0 )
    return 0; // Auto
  else if( d->dvd && m_comboSpeed->currentItem() == 1 )
    return -1; // Ignore
  else
    return d->indexSpeedMap[m_comboSpeed->currentItem()];
}


int K3bWriterSelectionWidget::writingApp() const
{
  KConfig* c = k3bcore->config();
  QString oldGroup = c->group();
  c->setGroup("General Options");
  bool b = c->readBoolEntry( "Manual writing app selection", false );
  c->setGroup( oldGroup );
  if( b ) {
    return selectedWritingApp();
  }
  else
    return K3b::DEFAULT;
}


int K3bWriterSelectionWidget::selectedWritingApp() const
{
  return K3b::writingAppFromString( m_comboWritingApp->currentText() );
}


void K3bWriterSelectionWidget::slotSpeedChanged( int )
{
  if( K3bDevice::Device* dev = writerDevice() )
    dev->setCurrentWriteSpeed( writerSpeed() );
}


void K3bWriterSelectionWidget::slotWriterChanged()
{
  slotRefreshWriterSpeeds();

  // save last selected writer
  if( K3bDevice::Device* dev = writerDevice() ) {
    QString oldGroup = k3bcore->config()->group();
    k3bcore->config()->setGroup( "General Options" );
    k3bcore->config()->writeEntry( "current_writer", dev->devicename() );
    k3bcore->config()->setGroup( oldGroup );
  }
}


void K3bWriterSelectionWidget::setSupportedWritingApps( int i )
{
  int oldApp = writingApp();

  m_comboWritingApp->clear();

  m_comboWritingApp->insertItem( i18n("Auto") );

  if( i & K3b::CDRDAO )
    m_comboWritingApp->insertItem( "cdrdao" );
  if( i & K3b::CDRECORD )
    m_comboWritingApp->insertItem( "cdrecord" );
  if( i & K3b::DVDRECORD )
    m_comboWritingApp->insertItem( "dvdrecord" );
  if( i & K3b::GROWISOFS )
    m_comboWritingApp->insertItem( "growisofs" );
  if( i & K3b::DVD_RW_FORMAT )
    m_comboWritingApp->insertItem( "dvd+rw-format" );

  setWritingApp( oldApp );
}


void K3bWriterSelectionWidget::loadConfig( KConfig* c )
{
  setWriterDevice( k3bcore->deviceManager()->findDevice( c->readEntry( "writer_device" ) ) );
  setSpeed( c->readNumEntry( "writing_speed",  0 ) );
  setWritingApp( K3b::writingAppFromString( c->readEntry( "writing_app" ) ) );
}


void K3bWriterSelectionWidget::saveConfig( KConfig* c )
{
  c->writeEntry( "writing_speed", writerSpeed() );
  c->writeEntry( "writer_device", writerDevice() ? writerDevice()->devicename() : QString::null );
  c->writeEntry( "writing_app", m_comboWritingApp->currentText() );
}

void K3bWriterSelectionWidget::loadDefaults()
{
  // ignore the writer
  m_comboSpeed->setCurrentItem( 0 ); // Auto
  setWritingApp( K3b::DEFAULT );
}


void K3bWriterSelectionWidget::setForceAutoSpeed( bool b )
{
  d->forceAutoSpeed = b;
  m_buttonDetermineSpeed->setDisabled(b);
  slotRefreshWriterSpeeds();
}


void K3bWriterSelectionWidget::slotDetermineSupportedWriteSpeeds()
{
  if( writerDevice() && !d->forceAutoSpeed ) {
    // change the cursor since we block the gui here :(
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    int media = writerDevice()->dvdMediaType();
    bool isDvd = ( media & K3bDevice::MEDIA_WRITABLE_DVD ) && ( media > 0 );

    if( !isDvd && d->dvd ) {
      QApplication::restoreOverrideCursor();
      KMessageBox::error( this, i18n("No writable DVD media found.") );
    }
    else if( isDvd && !d->dvd ) {
      QApplication::restoreOverrideCursor();
      KMessageBox::error( this, i18n("No writable CD media found.") );
    }
    else {
      QValueList<int> speeds = writerDevice()->determineSupportedWriteSpeeds();
      if( speeds.isEmpty() ) {
	QApplication::restoreOverrideCursor();
	KMessageBox::error( this, i18n("Unable to determine the supported writing speeds.") );
      }
      else {
	int lastSpeed = writerSpeed();
	
	clearSpeedCombo();
	m_comboSpeed->insertItem( i18n("Auto") );
	if( d->dvd )
	  m_comboSpeed->insertItem( i18n("Ignore") );

	for( QValueList<int>::iterator it = speeds.begin(); it != speeds.end(); ++it )
	  insertSpeedItem( *it );
	
	// try to reload last set speed
	setSpeed( lastSpeed );

	QApplication::restoreOverrideCursor();
      }
    }
  }
}


#include "k3bwriterselectionwidget.moc"
