#include "k3bblankingdialog.h"

#include "device/k3bdevice.h"
#include "device/k3bdevicemanager.h"
#include "k3b.h"
#include "k3bblankingjob.h"

#include <klocale.h>

#include <qgroupbox.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qlist.h>
#include <qframe.h>
#include <qtextview.h>
#include <qcombobox.h>
#include <qlabel.h>



K3bBlankingDialog::K3bBlankingDialog( QWidget* parent, const char* name )
  : KDialogBase( parent, name, false, i18n("Blanking CD-RW"), 
		 KDialogBase::Help|KDialogBase::Cancel|KDialogBase::User2|KDialogBase::User1, 
		 KDialogBase::User1, true, i18n("Blank"), i18n("Close") )  // the Close button for some reason does not work
{
  setupGui();
  setButtonBoxOrientation( Qt::Vertical );
  m_groupBlankType->setButton( 0 );

  // disable cancel button by default
  actionButton( KDialogBase::Cancel )->setDisabled( true );


  // -- read cd-writers ----------------------------------------------
  QList<K3bDevice> _devices = k3bMain()->deviceManager()->burningDevices();
  K3bDevice* _dev = _devices.first();
  while( _dev ) {
    m_comboWriter->insertItem( _dev->vendor() + " " + _dev->description() + " (" + _dev->devicename() + ")" );
    _dev = _devices.next();
  }
  
  slotRefreshWriterSpeeds(); 

  m_job = 0;
}


K3bBlankingDialog::~K3bBlankingDialog()
{
  qDebug("(K3bBlankingDialog) destruction" );
}


void K3bBlankingDialog::setupGui()
{
  QFrame* frame = makeMainWidget();

  // --- setup device group ----------------------------------------------------
  m_groupWriter = new QGroupBox( frame, "m_groupWriter" );
  m_groupWriter->setTitle( i18n( "Burning Device" ) );
  m_groupWriter->setColumnLayout(0, Qt::Vertical );
  m_groupWriter->layout()->setSpacing( 0 );
  m_groupWriter->layout()->setMargin( 0 );
  QGridLayout* groupWriterLayout = new QGridLayout( m_groupWriter->layout() );
  groupWriterLayout->setAlignment( Qt::AlignTop );
  groupWriterLayout->setSpacing( spacingHint() );
  groupWriterLayout->setMargin( marginHint() );

  QLabel* labelSpeed = new QLabel( m_groupWriter, "TextLabel1" );
  labelSpeed->setText( i18n( "Burning Speed" ) );
    
  m_comboSpeed = new QComboBox( FALSE, m_groupWriter, "m_comboSpeed" );
  m_comboSpeed->setAutoMask( FALSE );
  m_comboSpeed->setDuplicatesEnabled( FALSE );
    
  m_comboWriter = new QComboBox( FALSE, m_groupWriter, "m_comboWriter" );

  QLabel* labelDevice = new QLabel( m_groupWriter, "TextLabel1_2" );
  labelDevice->setText( i18n( "Device" ) );

  groupWriterLayout->addWidget( labelDevice, 0, 0 );
  groupWriterLayout->addWidget( labelSpeed, 0, 1 );
  groupWriterLayout->addWidget( m_comboWriter, 1, 0 );  
  groupWriterLayout->addWidget( m_comboSpeed, 1, 1 );
  
  groupWriterLayout->setColStretch( 0, 1 );
  
  connect( m_comboWriter, SIGNAL(activated(int)), this, SLOT(slotRefreshWriterSpeeds()) );
  // --------------------------------------------------------- device group ---


  // --- setup the blynking type button group -----------------------------
  m_groupBlankType = new QButtonGroup( i18n("Blanking type"), frame );
  m_groupBlankType->setExclusive( true );
  m_groupBlankType->setColumnLayout(0, Qt::Vertical );
  m_groupBlankType->layout()->setSpacing( 0 );
  m_groupBlankType->layout()->setMargin( 0 );
  QVBoxLayout* groupBlankTypeLayout = new QVBoxLayout( m_groupBlankType->layout() );
  groupBlankTypeLayout->setAlignment( Qt::AlignTop );
  groupBlankTypeLayout->setSpacing( spacingHint() );
  groupBlankTypeLayout->setMargin( marginHint() );

  m_radioFastBlank = new QRadioButton( i18n("Fast"), m_groupBlankType );
  m_radioCompleteBlank = new QRadioButton( i18n("Complete"), m_groupBlankType );
  m_radioBlankTrack = new QRadioButton( i18n("Blank last track"), m_groupBlankType );
  m_radioUncloseSession = new QRadioButton( i18n("Unclose last session"), m_groupBlankType );
  m_radioBlankSession = new QRadioButton( i18n("Blank last session"), m_groupBlankType );

  groupBlankTypeLayout->addWidget( m_radioFastBlank );
  groupBlankTypeLayout->addWidget( m_radioCompleteBlank );
  groupBlankTypeLayout->addWidget( m_radioBlankTrack);
  groupBlankTypeLayout->addWidget( m_radioUncloseSession );
  groupBlankTypeLayout->addWidget( m_radioBlankSession );
  // ----------------------------------------------------------------------


  // ----- setup the putput group ------------------------------------------
  m_groupOutput = new QGroupBox( i18n("Output"), frame );
  m_groupOutput->setColumnLayout(0, Qt::Vertical );
  m_groupOutput->layout()->setSpacing( 0 );
  m_groupOutput->layout()->setMargin( 0 );
  QGridLayout* groupOutputLayout = new QGridLayout( m_groupOutput->layout() );
  groupOutputLayout->setAlignment( Qt::AlignTop );
  groupOutputLayout->setSpacing( spacingHint() );
  groupOutputLayout->setMargin( marginHint() );

  m_viewOutput = new QTextView( m_groupOutput );
  groupOutputLayout->addWidget( m_viewOutput, 0, 0 );
  // ------------------------------------------------------------------------

  // -- setup option group --------------------------------------------------
  m_groupOptions = new QGroupBox( i18n("Options"), frame );
  m_groupOptions->setColumnLayout(0, Qt::Vertical );
  m_groupOptions->layout()->setSpacing( 0 );
  m_groupOutput->layout()->setMargin( 0 );
  QVBoxLayout* groupOptionsLayout = new QVBoxLayout( m_groupOptions->layout() );
  groupOptionsLayout->setAlignment( Qt::AlignTop );
  groupOptionsLayout->setSpacing( spacingHint() );
  groupOptionsLayout->setMargin( marginHint() );

  m_checkForce = new QCheckBox( i18n("Force"), m_groupOptions );
  groupOptionsLayout->addWidget( m_checkForce );
  // ------------------------------------------------------------------------


  QGridLayout* grid = new QGridLayout( frame );
  grid->setSpacing( spacingHint() );
  grid->setMargin( marginHint() );

  grid->addMultiCellWidget( m_groupWriter, 0, 0, 0, 1 );
  grid->addWidget( m_groupBlankType, 1, 0 );
  grid->addWidget( m_groupOptions, 1, 1 );
  grid->addMultiCellWidget( m_groupOutput, 2, 2, 0, 1 );
}


void K3bBlankingDialog::slotRefreshWriterSpeeds()
{
  if( K3bDevice* _dev = writerDevice() ) {
    // add speeds to combobox
    m_comboSpeed->clear();
    m_comboSpeed->insertItem( "1x" );
    int _speed = 2;
    while( _speed <= _dev->maxWriteSpeed() ) {
      m_comboSpeed->insertItem( QString( "%1x" ).arg(_speed) );
      _speed+=2;
    }
  }
}

K3bDevice* K3bBlankingDialog::writerDevice() const
{
  const QString s = m_comboWriter->currentText();

  QString strDev = s.mid( s.find('(') + 1, s.find(')') - s.find('(') - 1 );
 
  K3bDevice* dev =  k3bMain()->deviceManager()->deviceByName( strDev );
  if( !dev )
    qDebug( "(K3bBlankingDialog) could not find device " + s );
		
  return dev;
}

int K3bBlankingDialog::writerSpeed() const
{
  QString _strSpeed = m_comboSpeed->currentText();
  _strSpeed.truncate( _strSpeed.find('x') );
	
  return _strSpeed.toInt();
}


void K3bBlankingDialog::slotUser1()
{
  // start the blankingjob and connect to the info-signal
  // disable the user1 button and enable the cancel button
  actionButton( KDialogBase::User1 )->setDisabled( true );
  actionButton( KDialogBase::Cancel )->setEnabled( true );
  m_viewOutput->setText("");

  if( m_job == 0 ) {
    m_job = new K3bBlankingJob();
    connect( m_job, SIGNAL(infoMessage(const QString&)), this, SLOT(slotInfoMessage(const QString&)) );
    connect( m_job, SIGNAL(finished(K3bJob*)), this, SLOT(slotJobFinished()) );
  }

  m_job->setDevice( writerDevice() );
  m_job->setSpeed( writerSpeed() );
  m_job->setForce( m_checkForce->isChecked() );

  switch( m_groupBlankType->id( m_groupBlankType->selected() ) ) {
  case 1:
    m_job->setMode( K3bBlankingJob::Fast );
    break;
  case 2:
    m_job->setMode( K3bBlankingJob::Complete );
    break;
  case 3:
    m_job->setMode( K3bBlankingJob::Track );
    break;
  case 4:
    m_job->setMode( K3bBlankingJob::Unclose );
    break;
  case 5:
    m_job->setMode( K3bBlankingJob::Session );
    break;
  }

  m_job->start();
}


void K3bBlankingDialog::slotUser2()
{
  close();
}


void K3bBlankingDialog::slotCancel()
{
  if( m_job )
    m_job->cancel();
}


void K3bBlankingDialog::slotInfoMessage( const QString& str )
{
  m_viewOutput->append( str + "\n" );
}


void K3bBlankingDialog::slotJobFinished()
{
  actionButton( KDialogBase::User1 )->setEnabled( true );
  actionButton( KDialogBase::Cancel )->setDisabled( true );
}


void K3bBlankingDialog::closeEvent( QCloseEvent* e )
{
  if( m_job )
    m_job->cancel();

  // do a delayed destruct
  delayedDestruct();

  KDialogBase::closeEvent( e );
}
