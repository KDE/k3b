#include "k3bburningoptiontab.h"
#include "../k3b.h"

#include <qlabel.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qlineedit.h>
#include <qtoolbutton.h>

#include <knuminput.h>
#include <kconfig.h>
#include <kdialog.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kstddirs.h>


K3bBurningOptionTab::K3bBurningOptionTab( QWidget* parent, const char* name )
  : QWidget( parent, name )
{
  setupGui();

  m_bPregapSeconds = false;
  m_comboPregapFormat->setCurrentItem( 1 );
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


  // misc settings group
  // -----------------------------------------------------------------------
  m_groupMisc = new QGroupBox( this, "m_groupMisc" );
  m_groupMisc->setTitle( i18n( "Misc" ) );
  m_groupMisc->setColumnLayout(0, Qt::Vertical );
  m_groupMisc->layout()->setSpacing( 0 );
  m_groupMisc->layout()->setMargin( KDialog::marginHint() );
  QGridLayout* groupMiscLayout = new QGridLayout( m_groupMisc->layout() );
  groupMiscLayout->setAlignment( Qt::AlignTop );
  groupMiscLayout->setSpacing( KDialog::spacingHint() );
  groupMiscLayout->setMargin( KDialog::marginHint() );

  QLabel* labelTempDir = new QLabel( i18n("Default temp directory:"), m_groupMisc );
  m_editTempDir = new QLineEdit( m_groupMisc );
  m_buttonTempDir = new QToolButton( m_groupMisc );
  m_buttonTempDir->setText( "..." );

  groupMiscLayout->addWidget( labelTempDir, 0, 0 );
  groupMiscLayout->addWidget( m_editTempDir, 1, 0 );
  groupMiscLayout->addWidget( m_buttonTempDir, 1, 1 );

  connect( m_buttonTempDir, SIGNAL(clicked()), this, SLOT(slotGetTempDir()) );
  // -----------------------------------------------------------------------



  // put all in the mainlayout
  // -----------------------------------------------------------------------
  QGridLayout* grid = new QGridLayout( this );
  grid->setSpacing( KDialog::spacingHint() );
  grid->setMargin( KDialog::marginHint() );

  grid->addWidget( m_groupAudio, 0, 0 );
  grid->addWidget( m_groupData, 0, 1 );
  grid->addMultiCellWidget( m_groupMisc, 1, 1, 0, 1 );

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
  QString tempdir = c->readEntry( "Temp Dir", locateLocal( "appdata", "temp/" ) );
  m_editTempDir->setText( tempdir );
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
  c->writeEntry( "Temp Dir", m_editTempDir->text() );
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


void K3bBurningOptionTab::slotGetTempDir()
{
  QString dir = KFileDialog::getExistingDirectory( m_editTempDir->text(), k3bMain(), "Select Temp Directory" );
  if( !dir.isEmpty() ) {
    m_editTempDir->setText( dir );
  }
}
