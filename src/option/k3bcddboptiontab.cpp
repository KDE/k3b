#include "k3bcddboptiontab.h"

#include <qvariant.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qtabwidget.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qstringlist.h>
#include <qcombobox.h>


#include <kdialog.h>
#include <kiconloader.h>
#include <krun.h>
#include <klistview.h>
#include <klineedit.h>
#include <klocale.h>
#include <knuminput.h>
#include <kconfig.h>
#include <kapplication.h>


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

  m_groupCddbServer->layout()->setMargin( 0 );
  m_groupCddbServerLayout->setMargin( KDialog::marginHint() );
  m_groupCddbServerLayout->setSpacing( KDialog::spacingHint() );

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
  m_boxCddbServerLayout->setSpacing( KDialog::spacingHint() );
  // -----------------------------------------------------------------------------


  m_viewLocalDir->setSorting(-1);
  m_viewLocalDir->setAcceptDrops( true );
  m_viewLocalDir->setDragEnabled( true );
  m_viewLocalDir->setDropVisualizer( true );

  m_viewCddbServer->setSorting(-1);
  m_viewCddbServer->setAcceptDrops( true );
  m_viewCddbServer->setDragEnabled( true );
  m_viewCddbServer->setDropVisualizer( true );

  m_comboCddbType->insertItem( "Http" );
  m_comboCddbType->insertItem( "Cddbp" );

  // setup connections
  // -----------------------------------------------------------------------------
  connect( m_buttonKdeProxySettings, SIGNAL(clicked()), this, SLOT(slotKdeProxySettings()) );

  connect( m_buttonAddLocalDir, SIGNAL(clicked()), this, SLOT(slotLocalDirAdd()) );
  connect( m_buttonRemoveLocalDir, SIGNAL(clicked()), this, SLOT(slotLocalDirRemove()) );

  connect( m_buttonAddCddbServer, SIGNAL(clicked()), this, SLOT(slotCddbServerAdd()) );
  connect( m_buttonRemoveCddbServer, SIGNAL(clicked()), this, SLOT(slotCddbServerRemove()) );

  connect( m_radioUseManualProxy, SIGNAL(toggled(bool)), this, SLOT(enDisableButtons()) );
  connect( m_editLocalDir, SIGNAL(textChanged(const QString&)), this, SLOT(enDisableButtons()) );
  connect( m_editCddbServer, SIGNAL(textChanged(const QString&)), this, SLOT(enDisableButtons()) );
  connect( m_viewLocalDir, SIGNAL(selectionChanged()), this, SLOT(enDisableButtons()) );
  connect( m_viewCddbServer, SIGNAL(selectionChanged()), this, SLOT(enDisableButtons()) );
  connect( m_comboCddbType, SIGNAL(highlighted(int)), 
	   this, SLOT(slotServerTypeChanged()) );
  // -----------------------------------------------------------------------------

  enDisableButtons();
}


K3bCddbOptionTab::~K3bCddbOptionTab()
{
}


void K3bCddbOptionTab::readSettings()
{
  KConfig* c = kapp->config();

  c->setGroup( "Cddb" );

  QStringList cddbpServer = c->readListEntry( "cddbp server" );
  QStringList httpServer = c->readListEntry( "http server" );
  QStringList localCddbDirs = c->readListEntry( "local cddb dirs" );

  m_checkRemoteCddb->setChecked( c->readBoolEntry( "use remote cddb", false ) );
  m_checkUseLocalCddb->setChecked( c->readBoolEntry( "use local cddb query", true ) );
  m_checkSaveLocalEntries->setChecked( c->readBoolEntry( "save cddb entries locally", true ) );
  m_checkManualCgiPath->setChecked( c->readBoolEntry( "use manual cgi path", false ) );
  m_editManualCgiPath->setText( c->readEntry( "cgi path", "~cddb/cddb.cgi" ) );
  m_editProxyServer->setText( c->readEntry( "proxy server" ) );
  m_editProxyPort->setValue( c->readNumEntry( "proxy port", 8080 ) );

  if( localCddbDirs.isEmpty() )
    localCddbDirs.append( "~/.cddb/" );
  if( cddbpServer.isEmpty() && httpServer.isEmpty() )
    httpServer.append( "freedb.org:80" );

  if( c->readEntry( "proxy settings type", "kde" ) == "kde" )
    m_radioUseKdeProxy->setChecked( true );
  else {
    m_radioUseManualProxy->setChecked( true );
    m_checkUseProxy->setChecked( c->readBoolEntry( "use proxy server", false ) );
  }



  for( QStringList::const_iterator it = localCddbDirs.begin(); it != localCddbDirs.end(); ++it )
    (void)new KListViewItem( m_viewLocalDir, m_viewLocalDir->lastItem(), *it );

  for( QStringList::const_iterator it = httpServer.begin(); it != httpServer.end(); ++it ) {
    QString server, port;
    int colPos = (*it).find( ":" );
    if( colPos >= 0 ) {
      server = (*it).left( colPos );
      port = (*it).mid( colPos + 1 );
    }
    else {
      server = *it;
      port = "80";
    }

    (void)new KListViewItem( m_viewCddbServer, m_viewCddbServer->lastItem(), "Http", server, port );
  }

  for( QStringList::const_iterator it = cddbpServer.begin(); it != cddbpServer.end(); ++it ) {
    QString server, port;
    int colPos = (*it).find( ":" );
    if( colPos >= 0 ) {
      server = (*it).left( colPos );
      port = (*it).mid( colPos + 1 );
    }
    else {
      server = *it;
      port = "8880";
    }

    (void)new KListViewItem( m_viewCddbServer, m_viewCddbServer->lastItem(), "Cddbp", server, port );
  }

  enDisableButtons();
}


void K3bCddbOptionTab::apply()
{
  KConfig* c = kapp->config();

  c->setGroup( "Cddb" );

  c->writeEntry( "use remote cddb", m_checkRemoteCddb->isChecked() );
  c->writeEntry( "use local cddb query", m_checkUseLocalCddb->isChecked() );
  c->writeEntry( "use proxy server", m_checkUseProxy->isChecked() );
  c->writeEntry( "save cddb entries locally", m_checkSaveLocalEntries->isChecked() );
  c->writeEntry( "use manual cgi path", m_checkManualCgiPath->isChecked() );
  c->writeEntry( "cgi path", m_editManualCgiPath->text() );
  c->writeEntry( "proxy server", m_editProxyServer->text() );
  c->writeEntry( "proxy port", m_editProxyPort->value() );

  c->writeEntry( "proxy settings type", m_radioUseKdeProxy->isChecked() ? "kde" : "manual" );

  QStringList cddbServer;
  QStringList localCddbDirs;

  QListViewItemIterator it( m_viewLocalDir );
  while( it.current() ) {
    localCddbDirs.append( it.current()->text(0) );
    ++it;
  }

  QListViewItemIterator it1( m_viewCddbServer );
  while( it1.current() ) {
    cddbServer.append( it1.current()->text(1) + ":" + it1.current()->text(2) );
    ++it1;
  }

  // new config
  c->writeEntry( "cddb server", cddbServer );

  // old config <= 0.7.3
  if( c->hasKey( "http server" ) )
    c->deleteEntry( "http server" );
  if( c->hasKey( "cddbp server" ) )
    c->deleteEntry( "cddbp server" );

  c->writeEntry( "local cddb dirs", localCddbDirs );
}


void K3bCddbOptionTab::slotKdeProxySettings()
{
  KRun::runCommand( "kcmshell proxy" );
}

  
void K3bCddbOptionTab::slotLocalDirAdd()
{
  if( !m_editLocalDir->text().isEmpty() ) {
    (void)new KListViewItem( m_viewLocalDir, m_viewLocalDir->lastItem(), 
			     m_editLocalDir->text() );

    enDisableButtons();
  }
}


void K3bCddbOptionTab::slotLocalDirRemove()
{
  if( QListViewItem* item = m_viewLocalDir->selectedItem() )
    delete item;

  enDisableButtons();
}


void K3bCddbOptionTab::slotCddbServerAdd()
{
  if( !m_editCddbServer->text().isEmpty() ) {
    (void)new KListViewItem( m_viewCddbServer, m_viewCddbServer->lastItem(), 
			     m_comboCddbType->currentText(),
			     m_editCddbServer->text(), 
			     QString::number( m_editCddbPort->value() ) );

    enDisableButtons();
  }
}


void K3bCddbOptionTab::slotCddbServerRemove()
{
  if( QListViewItem* item = m_viewCddbServer->selectedItem() )
    delete item;

  enDisableButtons();
}


void K3bCddbOptionTab::enDisableButtons()
{
  m_buttonAddLocalDir->setDisabled( m_editLocalDir->text().isEmpty() );
  m_buttonRemoveLocalDir->setDisabled( m_viewLocalDir->selectedItem() == 0 );

  m_buttonAddCddbServer->setDisabled( m_editCddbServer->text().isEmpty() );
  m_buttonRemoveCddbServer->setDisabled( m_viewCddbServer->selectedItem() == 0 );

  m_boxProxyServer->setEnabled( m_radioUseManualProxy->isChecked() && m_checkUseProxy->isChecked() );
}


void K3bCddbOptionTab::slotServerTypeChanged()
{
  m_editCddbPort->setValue( m_comboCddbType->currentText() == "Http" ? 80 : 8080 );
}


#include "k3bcddboptiontab.moc"
