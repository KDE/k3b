
#include "k3bsetuptab.h"
#include "k3bsetup.h"
#include "k3bsetupwizard.h"

#include <kstandarddirs.h>
#include <kdialog.h>
#include <klocale.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qfont.h>
#include <qcolor.h>
#include <qpixmap.h>


K3bSetupTab::K3bSetupTab( int index, int overall, const QString& info, K3bSetupWizard* parent, const char* name )
  : QWidget( parent, name )
{
  parent->addPage( this, i18n("Step %1 of %2").arg(index).arg(overall) );

  m_setup = parent->setup();


  static QPixmap setupLogo( locate( "data", "k3b/pics/k3bsetup.png" ) );

  m_labelInfoText = new QLabel( info, this, "setuptabinfolabel" );
  m_labelInfoText->setAlignment( AlignCenter | WordBreak );
  m_labelInfoText->setIndent( 10 );
  QFont font( m_labelInfoText->font() );
  font.setBold( true );
  m_labelInfoText->setFont( font );
  m_labelInfoText->setPaletteBackgroundColor( QColor(49, 174, 255) );
  m_labelInfoText->setPaletteForegroundColor( white );

  m_labelSetupLogo = new QLabel( this, "setuplogolabel" );
  m_labelSetupLogo->setPixmap( setupLogo );
  m_labelSetupLogo->setScaledContents( false );
  m_labelSetupLogo->setAlignment( AlignCenter );
  m_labelSetupLogo->setPaletteBackgroundColor( QColor(49, 174, 255) );

  m_mainLayout = new QGridLayout( this );
  m_mainLayout->addWidget( m_labelSetupLogo, 0, 0 );
  m_mainLayout->addWidget( m_labelInfoText, 1, 0 );

  m_mainLayout->setMargin( KDialog::marginHint() );
  m_mainLayout->setSpacing( 0 );
  m_mainLayout->addColSpacing( 1, KDialog::spacingHint() );
  m_mainLayout->setRowStretch( 1, 1 );
  m_mainLayout->setColStretch( 2, 1 );
}


K3bSetupTab::~K3bSetupTab()
{
}


void K3bSetupTab::setMainWidget( QWidget* mainWidget )
{
  m_mainWidget = mainWidget;
  m_mainLayout->addMultiCellWidget( mainWidget, 0, 1, 2, 2 );
}

void K3bSetupTab::readSettings()
{
}


bool K3bSetupTab::saveSettings()
{
  return true;
}


bool K3bSetupTab::appropriate()
{
  return true;
}
