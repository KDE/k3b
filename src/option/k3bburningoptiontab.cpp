#include "k3bburningoptiontab.h"
#include "../k3b.h"

#include <qlabel.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qgroupbox.h>

#include <knuminput.h>
#include <kconfig.h>
#include <kdialog.h>
#include <klocale.h>



K3bBurningOptionTab::K3bBurningOptionTab( QWidget* parent, const char* name )
  : QWidget( parent, name )
{
  setupGui();

  m_bPregapSeconds = false;
  m_comboPregapFormat->setCurrentItem( 1 );


  //  m_checkAllowWritingAppSelection->setDisabled( true );  // not implemented yet!
}


K3bBurningOptionTab::~K3bBurningOptionTab()
{
}


void K3bBurningOptionTab::setupGui()
{
  // audio settings group
  // -----------------------------------------------------------------------
  m_groupAudio = new QGroupBox( this, "m_groupAudio" );
  m_groupAudio->setTitle( i18n( "Audio project" ) );
  m_groupAudio->setColumnLayout(0, Qt::Vertical );
  m_groupAudio->layout()->setSpacing( 0 );
  m_groupAudio->layout()->setMargin( KDialog::marginHint() );
  QGridLayout* groupAudioLayout = new QGridLayout( m_groupAudio->layout() );
  groupAudioLayout->setAlignment( Qt::AlignTop );
  groupAudioLayout->setSpacing( KDialog::spacingHint() );
  groupAudioLayout->setMargin( KDialog::marginHint() );

  m_editDefaultPregap = new KIntNumInput( m_groupAudio );
  m_comboPregapFormat = new QComboBox( m_groupAudio );

  QLabel* labelDefaultPregap = new QLabel( i18n("Default pregap:"), m_groupAudio );

  groupAudioLayout->addWidget( labelDefaultPregap, 0, 0 );
  groupAudioLayout->addWidget( m_editDefaultPregap, 1, 0 );
  groupAudioLayout->addWidget( m_comboPregapFormat, 1, 1 );

  m_comboPregapFormat->insertItem( i18n( "Seconds" ) );
  m_comboPregapFormat->insertItem( i18n( "Frames" ) );

  connect( m_comboPregapFormat, SIGNAL(activated(const QString&)), 
	   this, SLOT(slotChangePregapFormat(const QString&)) );
  // -----------------------------------------------------------------------


  // data settings group
  // -----------------------------------------------------------------------
  m_groupData = new QGroupBox( this, "m_groupData" );
  m_groupData->setTitle( i18n( "Data project" ) );
  m_groupData->setColumnLayout(0, Qt::Vertical );
  m_groupData->layout()->setSpacing( 0 );
  m_groupData->layout()->setMargin( KDialog::marginHint() );
  QGridLayout* groupDataLayout = new QGridLayout( m_groupData->layout() );
  groupDataLayout->setAlignment( Qt::AlignTop );
  groupDataLayout->setSpacing( KDialog::spacingHint() );
  groupDataLayout->setMargin( KDialog::marginHint() );

  m_checkUseID3Tag = new QCheckBox( "Use ID3 Tags for filenames", m_groupData );

  groupDataLayout->addWidget( m_checkUseID3Tag, 0, 0 );

  // -----------------------------------------------------------------------


  // advanced settings group
  // -----------------------------------------------------------------------
  m_groupAdvanced = new QGroupBox( this, "m_groupAdvanced" );
  m_groupAdvanced->setTitle( i18n( "Advanced" ) );
  m_groupAdvanced->setColumnLayout(0, Qt::Vertical );
  m_groupAdvanced->layout()->setSpacing( 0 );
  m_groupAdvanced->layout()->setMargin( KDialog::marginHint() );
  QGridLayout* groupAdvancedLayout = new QGridLayout( m_groupAdvanced->layout() );
  groupAdvancedLayout->setAlignment( Qt::AlignTop );
  groupAdvancedLayout->setSpacing( KDialog::spacingHint() );
  groupAdvancedLayout->setMargin( KDialog::marginHint() );

  m_checkEject = new QCheckBox( i18n("Do not eject CD after write process"), m_groupAdvanced );
  m_checkManualWritingBufferSize = new QCheckBox( i18n("Manual writing buffer size"), m_groupAdvanced );
  m_editWritingBufferSizeCdrecord = new KIntNumInput( 4, m_groupAdvanced );
  m_editWritingBufferSizeCdrdao = new KIntNumInput( 32, m_groupAdvanced );

  QGridLayout* bufferLayout = new QGridLayout;
  bufferLayout->setMargin( 0 );
  bufferLayout->setSpacing( KDialog::spacingHint() );
  bufferLayout->addWidget( new QLabel( "Cdrecord", m_groupAdvanced ), 0, 1 );
  bufferLayout->addWidget( new QLabel( "Cdrdao", m_groupAdvanced ), 1, 1 );
  bufferLayout->addWidget( m_editWritingBufferSizeCdrecord, 0, 2 );
  bufferLayout->addWidget( m_editWritingBufferSizeCdrdao, 1, 2 );
  bufferLayout->addWidget( new QLabel( i18n("MB"), m_groupAdvanced ), 0, 3 );
  bufferLayout->addWidget( new QLabel( i18n("blocks"), m_groupAdvanced ), 1, 3 );
  bufferLayout->addColSpacing( 0, 30 );
  bufferLayout->setColStretch( 4, 1 );

  m_checkAllowWritingAppSelection = new QCheckBox( i18n("Manual writing app selection"), m_groupAdvanced );

  groupAdvancedLayout->addWidget( m_checkEject, 0, 0 );
  groupAdvancedLayout->addWidget( m_checkManualWritingBufferSize, 1, 0 );
  groupAdvancedLayout->addLayout( bufferLayout, 2, 0 );
  groupAdvancedLayout->addWidget( m_checkAllowWritingAppSelection, 3, 0 );

  connect( m_checkManualWritingBufferSize, SIGNAL(toggled(bool)), 
	   m_editWritingBufferSizeCdrecord, SLOT(setEnabled(bool)) );
  connect( m_checkManualWritingBufferSize, SIGNAL(toggled(bool)), 
	   m_editWritingBufferSizeCdrdao, SLOT(setEnabled(bool)) );
  connect( m_checkManualWritingBufferSize, SIGNAL(toggled(bool)), 
	   this, SLOT(slotSetDefaultBufferSizes(bool)) );

  m_editWritingBufferSizeCdrecord->setDisabled( true );
  m_editWritingBufferSizeCdrdao->setDisabled( true );
  // -----------------------------------------------------------------------



  // put all in the mainlayout
  // -----------------------------------------------------------------------
  QGridLayout* grid = new QGridLayout( this );
  grid->setSpacing( KDialog::spacingHint() );
  grid->setMargin( KDialog::marginHint() );

  grid->addWidget( m_groupAudio, 0, 0 );
  grid->addWidget( m_groupData, 0, 1 );
  grid->addMultiCellWidget( m_groupAdvanced, 1, 1, 0, 1 );

  // we do not want the groups to take more space than they require
  grid->setRowStretch( 2, 1 );
}


void K3bBurningOptionTab::readSettings()
{
  KConfig* c = kapp->config();

  c->setGroup( "Data project settings" );
  m_checkUseID3Tag->setChecked( c->readBoolEntry("Use ID3 Tag for mp3 renaming", false) );

  c->setGroup( "Audio project settings" );
  m_editDefaultPregap->setValue( c->readNumEntry( "default pregap", 150 ) );
  m_bPregapSeconds = false;
  m_comboPregapFormat->setCurrentItem( 1 );

  c->setGroup( "General Options" );
  m_checkEject->setChecked( c->readBoolEntry( "No cd eject", false ) );
  bool manualBufferSize = c->readBoolEntry( "Manual buffer size", false );
  m_checkManualWritingBufferSize->setChecked( manualBufferSize );
  if( manualBufferSize ) {
    m_editWritingBufferSizeCdrecord->setValue( c->readNumEntry( "Cdrecord buffer", 4 ) );
    m_editWritingBufferSizeCdrdao->setValue( c->readNumEntry( "Cdrdao buffer", 32 ) );
  }
  m_checkAllowWritingAppSelection->setChecked( c->readBoolEntry( "Manual writing app selection", false ) );
}


void K3bBurningOptionTab::saveSettings()
{
  KConfig* c = kapp->config();

  c->setGroup( "Data project settings" );
  c->writeEntry( "Use ID3 Tag for mp3 renaming", m_checkUseID3Tag->isChecked() );
	
  k3bMain()->setUseID3TagForMp3Renaming( m_checkUseID3Tag->isChecked() );

  c->setGroup( "Audio project settings" );
  c->writeEntry( "default pregap", m_bPregapSeconds ? m_editDefaultPregap->value() * 75 : m_editDefaultPregap->value() );

  c->setGroup( "General Options" );
  c->writeEntry( "No cd eject", m_checkEject->isChecked() );
  c->writeEntry( "Manual buffer size", m_checkManualWritingBufferSize->isChecked() );
  c->writeEntry( "Cdrecord buffer", m_editWritingBufferSizeCdrecord->value() );
  c->writeEntry( "Cdrdao buffer", m_editWritingBufferSizeCdrdao->value() );
  c->writeEntry( "Manual writing app selection", m_checkAllowWritingAppSelection->isChecked() );
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
