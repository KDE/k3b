#include "k3bcddboptiontab.h"

#include <qvariant.h>
#include <klineedit.h>
#include <klocale.h>
#include <knuminput.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qtabwidget.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include <kdialog.h>
#include <kiconloader.h>

K3bCddbOptionTab::K3bCddbOptionTab( QWidget* parent,  const char* name )
    : base_K3bCddbOptionTab( parent, name )
{
  // fix all the margins and spacings that have been corrupted by QDesigner ;-)
  // -----------------------------------------------------------------------------

  base_K3bCddbOptionTabLayout->setMargin( 0 );
  base_K3bCddbOptionTabLayout->setSpacing( KDialog::spacingHint() );

  m_mainTabbed->page(0)->layout()->setMargin( KDialog::marginHint() );
  m_mainTabbed->page(0)->layout()->setSpacing( KDialog::spacingHint() );
  m_mainTabbed->page(1)->layout()->setMargin( KDialog::marginHint() );
  m_mainTabbed->page(1)->layout()->setSpacing( KDialog::spacingHint() );

  m_groupLocalDir->layout()->setMargin( 0 );
  m_groupLocalDirLayout->setMargin( KDialog::marginHint() );
  m_groupLocalDirLayout->setSpacing( KDialog::spacingHint() );

  m_groupCddbp->layout()->setMargin( 0 );
  m_groupCddbpLayout->setMargin( KDialog::marginHint() );
  m_groupCddbpLayout->setSpacing( KDialog::spacingHint() );

  m_groupAdvancedOptions->layout()->setMargin( 0 );
  m_groupAdvancedOptionsLayout->setMargin( KDialog::marginHint() );
  m_groupAdvancedOptionsLayout->setSpacing( KDialog::spacingHint() );

  m_groupProxy->layout()->setMargin( 0 );
  m_groupProxyLayout->setMargin( KDialog::marginHint() );
  m_groupProxyLayout->setSpacing( KDialog::spacingHint() );

  m_groupProxySettingsSource->layout()->setMargin( 0 );
  m_groupProxySettingsSourceLayout->setSpacing( KDialog::spacingHint() );

  m_groupCgi->layout()->setMargin( 0 );
  m_groupCgiLayout->setMargin( KDialog::marginHint() );
  m_groupCgiLayout->setSpacing( KDialog::spacingHint() );

  m_boxProxyServerLayout->setSpacing( KDialog::spacingHint() );
  m_boxLocalDirectoryLayout->setSpacing( KDialog::spacingHint() );
  m_boxCddbpServerLayout->setSpacing( KDialog::spacingHint() );
  // -----------------------------------------------------------------------------

  m_buttonLocalDirUp->setPixmap( SmallIcon( "up" ) );
  m_buttonLocalDirDown->setPixmap( SmallIcon( "down" ) );
  m_buttonCddbpServerUp->setPixmap( SmallIcon( "up" ) );
  m_buttonCddbpServerDown->setPixmap( SmallIcon( "down" ) );

}


K3bCddbOptionTab::~K3bCddbOptionTab()
{
}


void K3bCddbOptionTab::readSettings()
{
}


void K3bCddbOptionTab::apply()
{
}


#include "k3bcddboptiontab.moc"
