#include "k3bblankingdialog.h"

#include "device/k3bdevice.h"
#include "device/k3bdevicemanager.h"
#include "k3b.h"
#include "k3bblankingjob.h"
#include "k3bwriterselectionwidget.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <klistview.h>
#include <kiconloader.h>
#include <kguiitem.h>
#include <kstdguiitem.h>

#include <qgroupbox.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qptrlist.h>
#include <qframe.h>
#include <qtextview.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qheader.h>


K3bBlankingDialog::K3bBlankingDialog( QWidget* parent, const char* name )
  : KDialogBase( parent, name, true, i18n("Erase CD-RW"), /*Help|*/User2|User1, User1, 
		 false, KGuiItem( i18n("&Erase"), "blank", i18n("Start erasing") ), KStdGuiItem::close() )
{
  setupGui();
  setButtonBoxOrientation( Qt::Vertical );
  m_groupBlankType->setButton( 0 );

  m_job = 0;

  connect( m_writerSelectionWidget, SIGNAL(writerChanged()), this, SLOT(slotWriterChanged()) );

  slotWriterChanged();
}


K3bBlankingDialog::~K3bBlankingDialog()
{
  if( m_job )
    delete m_job;
}


void K3bBlankingDialog::setupGui()
{
  QFrame* frame = makeMainWidget();

  m_writerSelectionWidget = new K3bWriterSelectionWidget( frame );


  // --- setup the blanking type button group -----------------------------
  m_groupBlankType = new QButtonGroup( i18n("&Erase Type"), frame );
  m_groupBlankType->setExclusive( true );
  m_groupBlankType->setColumnLayout(0, Qt::Vertical );
  m_groupBlankType->layout()->setSpacing( 0 );
  m_groupBlankType->layout()->setMargin( 0 );
  QVBoxLayout* groupBlankTypeLayout = new QVBoxLayout( m_groupBlankType->layout() );
  groupBlankTypeLayout->setAlignment( Qt::AlignTop );
  groupBlankTypeLayout->setSpacing( spacingHint() );
  groupBlankTypeLayout->setMargin( marginHint() );

  m_radioFastBlank = new QRadioButton( i18n("&Fast"), m_groupBlankType );
  m_radioCompleteBlank = new QRadioButton( i18n("&Complete"), m_groupBlankType );
  m_radioBlankTrack = new QRadioButton( i18n("Erase last &track"), m_groupBlankType );
  m_radioUncloseSession = new QRadioButton( i18n("&Unclose last session"), m_groupBlankType );
  m_radioBlankSession = new QRadioButton( i18n("Erase last &session"), m_groupBlankType );

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

  m_viewOutput = new KListView( m_groupOutput );
  m_viewOutput->addColumn( i18n("type") );
  m_viewOutput->addColumn( i18n("message") );
  m_viewOutput->header()->hide();
  groupOutputLayout->addWidget( m_viewOutput, 0, 0 );
  // ------------------------------------------------------------------------

  // -- setup option group --------------------------------------------------
  m_groupOptions = new QGroupBox( i18n("Options"), frame );
  m_groupOptions->setColumnLayout(0, Qt::Vertical );
  m_groupOptions->layout()->setSpacing( 0 );
  m_groupOptions->layout()->setMargin( 0 );
  QVBoxLayout* groupOptionsLayout = new QVBoxLayout( m_groupOptions->layout() );
  groupOptionsLayout->setAlignment( Qt::AlignTop );
  groupOptionsLayout->setSpacing( spacingHint() );
  groupOptionsLayout->setMargin( marginHint() );

  m_checkForce = new QCheckBox( m_groupOptions );
  m_checkForce->setText( i18n("F&orce\n(Try this if K3b\nis not able to\nblank a CD-RW in\nnormal mode)") );

  groupOptionsLayout->addWidget( m_checkForce );
  // ------------------------------------------------------------------------


  QGridLayout* grid = new QGridLayout( frame );
  grid->setSpacing( spacingHint() );
  grid->setMargin( 0 );

  grid->addMultiCellWidget( m_writerSelectionWidget, 0, 0, 0, 1 );
  grid->addWidget( m_groupBlankType, 1, 0 );
  grid->addWidget( m_groupOptions, 1, 1 );
  grid->addMultiCellWidget( m_groupOutput, 2, 2, 0, 1 );
}


void K3bBlankingDialog::slotUser1()
{
  // start the blankingjob and connect to the info-signal
  // disable the user1 button and enable the cancel button
  actionButton( KDialogBase::User1 )->setDisabled( true );
  actionButton( KDialogBase::User2 )->setText( i18n("&Cancel") );
  m_viewOutput->clear();

  if( m_job == 0 ) {
    m_job = new K3bBlankingJob();
    connect( m_job, SIGNAL(infoMessage(const QString&,int)), this, SLOT(slotInfoMessage(const QString&,int)) );
    connect( m_job, SIGNAL(finished(bool)), this, SLOT(slotJobFinished(bool)) );
  }

  m_job->setDevice( m_writerSelectionWidget->writerDevice() );
  m_job->setSpeed( m_writerSelectionWidget->writerSpeed() );
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
  if( m_job && m_job->active() ) {
    if( KMessageBox::questionYesNo( this, i18n("Are you sure you want to cancel?"), i18n("Cancel") ) == KMessageBox::Yes )
      m_job->cancel();
    }
  else
    done(0);
}


void K3bBlankingDialog::slotInfoMessage( const QString& str, int type )
{
  QListViewItem* item = new QListViewItem( m_viewOutput, m_viewOutput->lastItem(), QString::null, str );

  // set the icon
  switch( type ) {
  case K3bJob::ERROR:
    item->setPixmap( 0, SmallIcon( "stop" ) );
    break;
  case K3bJob::PROCESS:
    item->setPixmap( 0, SmallIcon( "cdwriter_unmount" ) );
    break;
  case K3bJob::STATUS:
  default:
    item->setPixmap( 0, SmallIcon( "ok" ) );
  }
}


void K3bBlankingDialog::slotJobFinished(bool)
{
  actionButton( KDialogBase::User1 )->setEnabled( true );
  actionButton( KDialogBase::User2 )->setText( i18n("Close") );
}


void K3bBlankingDialog::closeEvent( QCloseEvent* e )
{
  if( m_job && m_job->active() ) {
    if( KMessageBox::questionYesNo( this, i18n("Are you sure you want to cancel?"), i18n("Cancel") ) == KMessageBox::Yes ) {
      m_job->cancel();
      
      e->accept();
    }
    else
      e->ignore();
  }
  else {
    e->accept();
  }
}


void K3bBlankingDialog::slotWriterChanged()
{
  // check if it is a cdrw writer
  K3bDevice* dev = m_writerSelectionWidget->writerDevice();

  if( !dev )
    return;

  if( dev->writesCdrw() )
    actionButton( KDialogBase::User1 )->setEnabled( true );
  else {
    actionButton( KDialogBase::User1 )->setEnabled( false );
    QListViewItem* item = new QListViewItem( m_viewOutput, m_viewOutput->lastItem(), 
					     i18n("%1 does not support CD-RW writing.").arg(dev->devicename()) );
    item->setPixmap( 0, SmallIcon( "stop" ) );
  }
}


#include "k3bblankingdialog.moc"
