
#include "k3bexternalbinoptiontab.h"
#include "../tools/k3bexternalbinmanager.h"
#include "../tools/k3bexternalbinwidget.h"

#include <kmessagebox.h>
#include <kdialog.h>
#include <klocale.h>
#include <kiconloader.h>
#include <klistview.h>

#include <qgroupbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qfile.h>
#include <qptrlist.h>



K3bExternalBinOptionTab::K3bExternalBinOptionTab( K3bExternalBinManager* manager, QWidget* parent, const char* name )
  : QWidget( parent, name )
{
  m_manager = manager;

  QGridLayout* frameLayout = new QGridLayout( this );
  frameLayout->setSpacing( KDialog::spacingHint() );
  frameLayout->setMargin( 0 );

  m_externalBinWidget = new K3bExternalBinWidget( manager, this );
  frameLayout->addWidget( m_externalBinWidget, 1, 0 );

  QLabel* m_labelInfo = new QLabel( this, "m_labelInfo" );
  m_labelInfo->setText( i18n( "Specify the paths to the external programs that K3b needs to work properly, "
			      "or press \"Search\" to let K3b search for the programs." ) );
  m_labelInfo->setScaledContents( FALSE );
  m_labelInfo->setAlignment( int( QLabel::WordBreak | QLabel::AlignVCenter | QLabel::AlignLeft ) );

  frameLayout->addWidget( m_labelInfo, 0, 0 );
}


K3bExternalBinOptionTab::~K3bExternalBinOptionTab()
{
}


void K3bExternalBinOptionTab::readSettings()
{
  m_externalBinWidget->load();
}


void K3bExternalBinOptionTab::saveSettings()
{
  m_externalBinWidget->save();
}


#include "k3bexternalbinoptiontab.moc"
