/***************************************************************************
                          k3boptioncddb.cpp  -  description
                             -------------------
    begin                : Fri Nov 2 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3bcddboptiontab.h"

#include <qwidget.h>
#include <qframe.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qgroupbox.h>
#include <qvgroupbox.h>
#include <qhgroupbox.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlistbox.h>
#include <qstring.h>

#include <klineedit.h>
#include <klistbox.h>
#include <klocale.h>
#include <kconfig.h>
#include <kapp.h>
#include <kdialog.h>
#include <kstddirs.h>

#define DEFAULT_CDDB_HOST  "localhost:888"

K3bCddbOptionTab::K3bCddbOptionTab(QFrame *parent, const char *name)
: QWidget(parent, name) {
	setup();
}

K3bCddbOptionTab::~K3bCddbOptionTab(){
}

void K3bCddbOptionTab::setup(){

  QGridLayout* frameLayout = new QGridLayout( this );
  frameLayout->setSpacing( KDialog::spacingHint() );
  frameLayout->setMargin( KDialog::marginHint() );

  QGroupBox *groupCddbOptions = new QGroupBox( this, "cddb_settings_options" );
  groupCddbOptions->setColumnLayout(0, Qt::Vertical );
  groupCddbOptions->setTitle( i18n( "Options" ) );
  QHBoxLayout *optionsLayout = new QHBoxLayout( groupCddbOptions->layout() );
  optionsLayout->setSpacing(  KDialog::spacingHint() );
  optionsLayout->setMargin(  KDialog::marginHint() );
  m_cddbLockup = new QCheckBox(i18n("Enable CDDB Lockups"), groupCddbOptions, "cddb_settings_lockup");
  optionsLayout->addWidget(m_cddbLockup);

  // edit settings
  m_groupCddbServer = new QGroupBox( this, "cddb_settings_server" );
  m_groupCddbServer->setTitle( i18n( "Remote CDDB Access" ) );
  m_groupCddbServer->setColumnLayout(0, Qt::Vertical );
  m_groupCddbServer->layout()->setSpacing( KDialog::spacingHint() );
  m_groupCddbServer->layout()->setMargin( KDialog::marginHint() );
  m_groupCddbServer->setDisabled(true);
	
  QGridLayout* serverLayout = new QGridLayout( m_groupCddbServer->layout() );
  serverLayout->setSpacing( KDialog::spacingHint() );
  serverLayout->setMargin( KDialog::marginHint() );
  QHBox *serverSettings = new QHBox(m_groupCddbServer);
  QLabel *host = new QLabel(i18n("Hostname:"), serverSettings );
  m_cddbServerInput = new KLineEdit(serverSettings, "cddb_settings_serverInput");
  QLabel *port = new QLabel(i18n("Port:"), serverSettings);
  m_cddbPortInput = new KLineEdit(serverSettings, "cddb_settings_portInput");
  m_cddbPortInput->setMaxLength(5);
  m_cddbPortInput->setFixedWidth(60);
  serverSettings->setStretchFactor(host, 5);
  serverSettings->setStretchFactor(m_cddbServerInput, 30);
  m_cddbServerList = new KListBox( m_groupCddbServer, "cddb_settings" );
  m_addButton = new QPushButton(i18n("Add"), m_groupCddbServer);
  m_delButton = new QPushButton(i18n("Delete"), m_groupCddbServer);
  m_delButton->setDisabled(true);

  /*
  QVGroupBox *localServerSettings = new QVGroupBox(m_groupCddbServer);
  localServerSettings->layout()->setSpacing(0);
  localServerSettings->layout()->setMargin(0);
  localServerSettings->setFrameStyle( QFrame::NoFrame );
  localServerSettings->addSpace( 3 );
  QLabel *localServer = new QLabel( i18n("File to save cddb entries of ripped 'wavs'."), localServerSettings );
  m_songListPath = new KLineEdit( localServerSettings, "local_input");
  localServerSettings->addSpace( 5 );
  QFrame* line = new QFrame( localServerSettings, "line" );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  localServerSettings->addSpace( 10 );
  */
  //serverLayout->addMultiCellWidget( localServerSettings, 0, 0, 0, 3 );
  serverLayout->addMultiCellWidget( serverSettings, 1, 1, 0, 2 );
  serverLayout->addMultiCellWidget( m_addButton, 1, 1, 3, 3 );
  serverLayout->addMultiCellWidget( m_delButton, 2, 2, 3, 3 );
  serverLayout->addMultiCellWidget( m_cddbServerList, 2, 3, 0, 2 );
  serverLayout->setRowStretch(3, 10);
  serverLayout->setColStretch(0, 50);
  frameLayout->addWidget( groupCddbOptions, 0, 0 );
  frameLayout->addWidget( m_groupCddbServer, 1, 0 );

  connect( m_cddbLockup, SIGNAL(toggled(bool)), this, SLOT(toggled(bool)) );
  connect( m_addButton, SIGNAL(clicked()), this, SLOT(addCddbServer()) );
  connect( m_delButton, SIGNAL(clicked()), this, SLOT(delCddbServer()) );
  connect( m_cddbServerList, SIGNAL( executed(QListBoxItem*) ), this, SLOT(serverSelected(QListBoxItem*)) );
}
// slots
// ------------------------------------------------------
void K3bCddbOptionTab::toggled(bool enabled){
	if (enabled){
		m_groupCddbServer->setEnabled(true);
	} else {
	   m_groupCddbServer->setDisabled(true);
	}
}

void K3bCddbOptionTab::addCddbServer(){
   QString server = m_cddbServerInput->text() + ":" + m_cddbPortInput->text();
	QListBoxText *item = new QListBoxText(m_cddbServerList, server);
}

void K3bCddbOptionTab::delCddbServer(){
    m_cddbServerList->removeItem(m_cddbServerList->currentItem() );
    m_delButton->setDisabled(true);
    m_cddbServerList->setCurrentItem( m_cddbServerList->currentItem() );
}

void K3bCddbOptionTab::serverSelected(QListBoxItem *item){
    m_delButton->setEnabled(true);
    QString hostString = m_cddbServerList->currentText(); //item( m_cddbServerList->currentItem() )->text();
    fillInputFields(hostString);
}
// reading and writing settings
// --------------------------------------------------
void K3bCddbOptionTab::apply(){
    KConfig* c = kapp->config();
    c->setGroup("Cddb");
    //c->writeEntry( "songlistPath", m_songListPath->text() );
    c->writeEntry( "useCddb", m_cddbLockup->isChecked() );
    QStringList list;
    for( unsigned int i = 0; i < m_cddbServerList->count(); i++){
        list.append(m_cddbServerList->text(i));
   }
   c->writeEntry("cddbServers", list);
   c->writeEntry("cddbServer",   m_cddbServerInput->text() + ":" + m_cddbPortInput->text() );
   c->sync();
}

void K3bCddbOptionTab::readSettings(){
  KConfig *c = kapp->config();
  c->setGroup("Cddb");
  m_cddbLockup->setChecked( c->readBoolEntry("useCddb", false) );
  /*
  QString path = c->readEntry("songlistPath");
  if( path == 0 )
    path = locateLocal("appdata", "k3b") + "/" + DEFAULT_SONGLIST_FILE;
  m_songListPath->setText( path );
  */
  QStringList list = c->readListEntry("cddbServers");
  if( !list.isEmpty() ){
     for( QStringList::Iterator it = list.begin(); it != list.end(); ++it ) {
	      QListBoxText *item = new QListBoxText(m_cddbServerList, (*it).latin1() );
     }
  }
  QString host = c->readEntry("cddbServer", DEFAULT_CDDB_HOST);
  fillInputFields(host);
}
//  helpers
// -------------------------------------------------
void K3bCddbOptionTab::fillInputFields(QString hostString){
   int index = hostString.find(":");
   m_cddbServerInput->setText(hostString.left(index) );
   m_cddbPortInput->setText(hostString.right(hostString.length()-index-1) );
}


#include "k3bcddboptiontab.moc"
