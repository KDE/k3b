#include "k3bburningoptiontab.h"
#include "../k3b.h"

#include <qlabel.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qtabwidget.h>
#include <qradiobutton.h>
#include <qvalidator.h>
#include <qbuttongroup.h>

#include <knuminput.h>
#include <kconfig.h>
#include <kdialog.h>
#include <klocale.h>
#include <klineedit.h>


K3bBurningOptionTab::K3bBurningOptionTab( QWidget* parent, const char* name )
  : QWidget( parent, name )
{
  setupGui();

  m_bPregapSeconds = false;
  m_comboPregapFormat->setCurrentItem( 1 );


  m_checkAllowWritingAppSelection->setDisabled( true );  // not implemented yet!
}


K3bBurningOptionTab::~K3bBurningOptionTab()
{
}


void K3bBurningOptionTab::setupGui()
{
  QVBoxLayout* box = new QVBoxLayout( this );
  box->setAutoAdd( true );

  QTabWidget* mainTabbed = new QTabWidget( this );


  // PROJECT TAB
  // //////////////////////////////////////////////////////////////////////
  QWidget* projectTab = new QWidget( mainTabbed );

  // audio settings group
  // -----------------------------------------------------------------------
  QGroupBox* m_groupAudio = new QGroupBox( projectTab, "m_groupAudio" );
  m_groupAudio->setTitle( i18n( "Audio project" ) );
  m_groupAudio->setColumnLayout(0, Qt::Vertical );
  m_groupAudio->layout()->setSpacing( 0 );
  m_groupAudio->layout()->setMargin( 0 );
  QGridLayout* groupAudioLayout = new QGridLayout( m_groupAudio->layout() );
  groupAudioLayout->setAlignment( Qt::AlignTop );
  groupAudioLayout->setSpacing( KDialog::spacingHint() );
  groupAudioLayout->setMargin( KDialog::marginHint() );

  m_editDefaultPregap = new KIntNumInput( m_groupAudio );
  m_comboPregapFormat = new QComboBox( m_groupAudio );

  QLabel* labelDefaultPregap = new QLabel( i18n("Default pregap:"), m_groupAudio );

  groupAudioLayout->addWidget( labelDefaultPregap, 0, 0 );
  groupAudioLayout->addWidget( m_editDefaultPregap, 0, 1 );
  groupAudioLayout->addWidget( m_comboPregapFormat, 0, 2 );

  m_comboPregapFormat->insertItem( i18n( "Seconds" ) );
  m_comboPregapFormat->insertItem( i18n( "Frames" ) );

  connect( m_comboPregapFormat, SIGNAL(activated(const QString&)), 
	   this, SLOT(slotChangePregapFormat(const QString&)) );
  // -----------------------------------------------------------------------


  // data settings group
  // -----------------------------------------------------------------------
  QGroupBox* m_groupData = new QGroupBox( 2, Qt::Vertical, i18n( "Data project" ), projectTab, "m_groupData" );
  m_groupData->layout()->setSpacing( KDialog::spacingHint() );
  m_groupData->layout()->setMargin( KDialog::marginHint() );

  m_checkUseID3Tag = new QCheckBox( i18n("Use audio tags for filenames"), m_groupData );
  m_checkDropDoubles = new QCheckBox( i18n("Discard identical names"), m_groupData );
  // -----------------------------------------------------------------------

  // default cd size group
  // -----------------------------------------------------------------------
  QButtonGroup* groupCdSize = new QButtonGroup( 0, Qt::Vertical, i18n("Default CD Size"), projectTab );
  groupCdSize->layout()->setSpacing( 0 );
  groupCdSize->layout()->setMargin( 0 );
  QGridLayout* groupCdSizeLayout = new QGridLayout( groupCdSize->layout() );
  groupCdSizeLayout->setAlignment( Qt::AlignTop );
  groupCdSizeLayout->setSpacing( KDialog::spacingHint() );
  groupCdSizeLayout->setMargin( KDialog::marginHint() );

  m_radio74Minutes    = new QRadioButton( i18n("%1 minutes (%2 MB)").arg(74).arg(650), groupCdSize );
  m_radio80Minutes    = new QRadioButton( i18n("%1 minutes (%2 MB)").arg(80).arg(700), groupCdSize );
  m_radio100Minutes   = new QRadioButton( i18n("%1 minutes (%2 MB)").arg(100).arg(880), groupCdSize );
  m_radioCustomCdSize = new QRadioButton( i18n("Custom:"), groupCdSize );
  m_editCustomCdSize  = new KLineEdit( groupCdSize );

  m_editCustomCdSize->setValidator( new QIntValidator( m_editCustomCdSize ) );

  groupCdSizeLayout->addMultiCellWidget( m_radio74Minutes, 0, 0, 0, 2 );
  groupCdSizeLayout->addMultiCellWidget( m_radio80Minutes, 1, 1, 0, 2 );
  groupCdSizeLayout->addMultiCellWidget( m_radio100Minutes, 2, 2, 0, 2 );
  groupCdSizeLayout->addWidget( m_radioCustomCdSize, 3, 0 );
  groupCdSizeLayout->addWidget( m_editCustomCdSize, 3, 1 );
  groupCdSizeLayout->addWidget( new QLabel( i18n("minutes"), groupCdSize ), 3, 2 );

  connect( m_radioCustomCdSize, SIGNAL(toggled(bool)), m_editCustomCdSize, SLOT(setEnabled(bool)) );
  connect( m_radioCustomCdSize, SIGNAL(toggled(bool)), m_editCustomCdSize, SLOT(setFocus()) );
  m_editCustomCdSize->setDisabled( true );
  // -----------------------------------------------------------------------


  QGridLayout* projectGrid = new QGridLayout( projectTab );
  projectGrid->setSpacing( KDialog::spacingHint() );
  projectGrid->setMargin( KDialog::marginHint() );

  projectGrid->addWidget( m_groupAudio, 0, 0 );
  projectGrid->addWidget( m_groupData, 1, 0 );
  projectGrid->addWidget( groupCdSize, 2, 0 );
  projectGrid->setRowStretch( 3, 1 );

  // ///////////////////////////////////////////////////////////////////////



  // advanced settings tab
  // -----------------------------------------------------------------------
  QWidget* advancedTab = new QWidget( mainTabbed );
  QGridLayout* groupAdvancedLayout = new QGridLayout( advancedTab );
  groupAdvancedLayout->setAlignment( Qt::AlignTop );
  groupAdvancedLayout->setSpacing( KDialog::spacingHint() );
  groupAdvancedLayout->setMargin( KDialog::marginHint() );

  m_checkEject = new QCheckBox( i18n("Do not eject CD after write process"), advancedTab );
  m_checkOverburn = new QCheckBox( i18n("Allow overburning"), advancedTab );
  m_checkManualWritingBufferSize = new QCheckBox( i18n("Manual writing buffer size"), advancedTab );
  m_editWritingBufferSizeCdrecord = new KIntNumInput( 4, advancedTab );
  m_editWritingBufferSizeCdrdao = new KIntNumInput( 32, advancedTab );

  QGridLayout* bufferLayout = new QGridLayout;
  bufferLayout->setMargin( 0 );
  bufferLayout->setSpacing( KDialog::spacingHint() );
  bufferLayout->addWidget( new QLabel( "Cdrecord", advancedTab ), 0, 1 );
  bufferLayout->addWidget( new QLabel( "Cdrdao", advancedTab ), 1, 1 );
  bufferLayout->addWidget( m_editWritingBufferSizeCdrecord, 0, 2 );
  bufferLayout->addWidget( m_editWritingBufferSizeCdrdao, 1, 2 );
  bufferLayout->addWidget( new QLabel( i18n("MB"), advancedTab ), 0, 3 );
  bufferLayout->addWidget( new QLabel( i18n("blocks"), advancedTab ), 1, 3 );
  bufferLayout->addColSpacing( 0, 30 );
  bufferLayout->setColStretch( 4, 1 );

  m_checkAllowWritingAppSelection = new QCheckBox( i18n("Manual writing app selection"), advancedTab );

  groupAdvancedLayout->addWidget( m_checkOverburn, 0, 0 );
  groupAdvancedLayout->addWidget( m_checkEject, 1, 0 );
  groupAdvancedLayout->addWidget( m_checkManualWritingBufferSize, 2, 0 );
  groupAdvancedLayout->addLayout( bufferLayout, 3, 0 );
  groupAdvancedLayout->addWidget( m_checkAllowWritingAppSelection, 4, 0 );

  connect( m_checkManualWritingBufferSize, SIGNAL(toggled(bool)), 
	   m_editWritingBufferSizeCdrecord, SLOT(setEnabled(bool)) );
  connect( m_checkManualWritingBufferSize, SIGNAL(toggled(bool)), 
	   m_editWritingBufferSizeCdrdao, SLOT(setEnabled(bool)) );
  connect( m_checkManualWritingBufferSize, SIGNAL(toggled(bool)), 
	   this, SLOT(slotSetDefaultBufferSizes(bool)) );

  m_editWritingBufferSizeCdrecord->setDisabled( true );
  m_editWritingBufferSizeCdrdao->setDisabled( true );
  // -----------------------------------------------------------------------



  // put all in the main tabbed
  // -----------------------------------------------------------------------
  mainTabbed->addTab( projectTab, i18n("Projects") );
  mainTabbed->addTab( advancedTab, i18n("Advanced") );
}


void K3bBurningOptionTab::readSettings()
{
  KConfig* c = kapp->config();

  c->setGroup( "Data project settings" );
  m_checkUseID3Tag->setChecked( c->readBoolEntry("Use ID3 Tag for mp3 renaming", false) );
  m_checkDropDoubles->setChecked( c->readBoolEntry("Drop doubles", false) );

  c->setGroup( "Audio project settings" );
  m_editDefaultPregap->setValue( c->readNumEntry( "default pregap", 150 ) );
  m_bPregapSeconds = false;
  m_comboPregapFormat->setCurrentItem( 1 );

  c->setGroup( "General Options" );
  m_checkEject->setChecked( c->readBoolEntry( "No cd eject", false ) );
  m_checkOverburn->setChecked( c->readBoolEntry( "Allow overburning", false ) );
  bool manualBufferSize = c->readBoolEntry( "Manual buffer size", false );
  m_checkManualWritingBufferSize->setChecked( manualBufferSize );
  if( manualBufferSize ) {
    m_editWritingBufferSizeCdrecord->setValue( c->readNumEntry( "Cdrecord buffer", 4 ) );
    m_editWritingBufferSizeCdrdao->setValue( c->readNumEntry( "Cdrdao buffer", 32 ) );
  }
  m_checkAllowWritingAppSelection->setChecked( c->readBoolEntry( "Manual writing app selection", false ) );

  int defaultCdSize = c->readNumEntry( "Default cd size", 74 );
  switch( defaultCdSize ) {
  case 74:
    m_radio74Minutes->setChecked( true );
    break;
  case 80:
    m_radio80Minutes->setChecked( true );
    break;
  case 100:
    m_radio100Minutes->setChecked( true );
    break;
  default:
    m_radioCustomCdSize->setChecked( true );
    m_editCustomCdSize->setText( QString::number(defaultCdSize) );
    break;
  }
}


void K3bBurningOptionTab::saveSettings()
{
  KConfig* c = kapp->config();

  c->setGroup( "Data project settings" );
  c->writeEntry( "Use ID3 Tag for mp3 renaming", m_checkUseID3Tag->isChecked() );
  c->writeEntry( "Drop doubles", m_checkDropDoubles->isChecked() );

  k3bMain()->setUseID3TagForMp3Renaming( m_checkUseID3Tag->isChecked() );

  c->setGroup( "Audio project settings" );
  c->writeEntry( "default pregap", m_bPregapSeconds ? m_editDefaultPregap->value() * 75 : m_editDefaultPregap->value() );

  c->setGroup( "General Options" );
  c->writeEntry( "No cd eject", m_checkEject->isChecked() );
  c->writeEntry( "Allow overburning", m_checkOverburn->isChecked() );
  c->writeEntry( "Manual buffer size", m_checkManualWritingBufferSize->isChecked() );
  c->writeEntry( "Cdrecord buffer", m_editWritingBufferSizeCdrecord->value() );
  c->writeEntry( "Cdrdao buffer", m_editWritingBufferSizeCdrdao->value() );
  c->writeEntry( "Manual writing app selection", m_checkAllowWritingAppSelection->isChecked() );

  if( m_radio74Minutes->isChecked() )
    c->writeEntry( "Default cd size", 74 );
  else if( m_radio80Minutes->isChecked() )
    c->writeEntry( "Default cd size", 80 );
  else if( m_radio100Minutes->isChecked() )
    c->writeEntry( "Default cd size", 100 );
  if( m_radioCustomCdSize->isChecked() )
    c->writeEntry( "Default cd size", m_editCustomCdSize->text().toInt() );
}


void K3bBurningOptionTab::slotChangePregapFormat( const QString& format )
{
  if( format == i18n( "Seconds" ) ) {
    if( !m_bPregapSeconds ) {
      m_bPregapSeconds = true;
      m_editDefaultPregap->setValue( m_editDefaultPregap->value() / 75 );
    }
  }
  else {
    if( m_bPregapSeconds ) {
      m_bPregapSeconds = false;
      m_editDefaultPregap->setValue( m_editDefaultPregap->value() * 75 );
    }
  }
}


void K3bBurningOptionTab::slotSetDefaultBufferSizes( bool b )
{
  if( !b ) {
    m_editWritingBufferSizeCdrecord->setValue( 4 );
    m_editWritingBufferSizeCdrdao->setValue( 32 );
  }
}


#include "k3bburningoptiontab.moc"
