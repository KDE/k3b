/***************************************************************************
                          k3bcddblocaldbtab.cpp  -  description
                             -------------------
    begin                : Mon Feb 11 2002
    copyright            : (C) 2002 by Sebastian Trueg
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

#include "k3bcddblocaldbtab.h"
#include "../rip/songdb/k3bsongmanager.h"
#include "../k3b.h"

#include <qframe.h>
#include <qlayout.h>
#include <qvgroupbox.h>
#include <qhgroupbox.h>
#include <qlabel.h>
#include <qdir.h>
#include <qmultilineedit.h>
#include <qmessagebox.h>
#include <qtabwidget.h>
#include <qpushbutton.h>

#include <klocale.h>
#include <kconfig.h>
#include <kdialog.h>
#include <klineedit.h>
#include <kapplication.h>
#include <kstddirs.h>
#include <kfiledialog.h>
#include <kdebug.h>

#define DEFAULT_SONGLIST_FILE "songlist.xml"

K3bCddbLocalDBTab::K3bCddbLocalDBTab( QFrame *parent, const char *name)
 : QWidget( parent, name ){
     setup();
}
K3bCddbLocalDBTab::~K3bCddbLocalDBTab(){
}
void K3bCddbLocalDBTab::setup(){

  QGridLayout* frameLayout = new QGridLayout( this );
  frameLayout->setSpacing( KDialog::spacingHint() );
  frameLayout->setMargin( KDialog::marginHint() );

/*  QGroupBox *_groupLocalServer = new QGroupBox( this, "cddb_local" );
  _groupLocalServer->setTitle( i18n( "Local Database" ) );
  _groupLocalServer->setColumnLayout(0, Qt::Vertical );
  _groupLocalServer->layout()->setSpacing( KDialog::spacingHint() );
  _groupLocalServer->layout()->setMargin( KDialog::marginHint() );
  //_groupLocalServer->setDisabled(true);
*/
  QVGroupBox *localServerSettings = new QVGroupBox( this );
  localServerSettings->setTitle( i18n( "Local Database" ) );
  localServerSettings->layout()->setSpacing(KDialog::spacingHint());
  localServerSettings->layout()->setMargin(KDialog::marginHint());
  localServerSettings->addSpace( 3 );
  QLabel *localServer = new QLabel( i18n("File to save cddb entries of ripped 'wavs'."), localServerSettings );
  QHGroupBox *localEditGroup = new QHGroupBox( localServerSettings );
  localEditGroup->setFrameStyle( QFrame::NoFrame );
  localEditGroup->layout()->setSpacing( 0 );
  localEditGroup->layout()->setMargin( 0 );
  m_songListPath = new KLineEdit( localEditGroup, "local_input");
  QPushButton *browse = new QPushButton( "...", localEditGroup );

  QGroupBox *_groupExtension = new QGroupBox( this, "cddb_local_ext" );
  _groupExtension->setTitle( i18n( "Database Handling" ) );
  _groupExtension->setColumnLayout(0, Qt::Vertical );
  _groupExtension->layout()->setSpacing( KDialog::spacingHint() );
  _groupExtension->layout()->setMargin( KDialog::marginHint() );

  QGridLayout *_extLayout = new QGridLayout( _groupExtension->layout() );
  m_dbHandlingTab = new QTabWidget( _groupExtension, "tabs");
  m_logOutput = new QMultiLineEdit( m_dbHandlingTab, "log" );
  m_dbHandlingTab->addTab( m_logOutput, i18n("Log") );
  QPushButton *clear = new QPushButton( i18n("Clear"), _groupExtension );
  QPushButton *verify = new QPushButton( i18n("Verify"), _groupExtension );
  QPushButton *find = new QPushButton( i18n("Find"), _groupExtension );
  QPushButton *add = new QPushButton( i18n("Add"), _groupExtension );
  QFrame *line = new QFrame( _groupExtension, "line");
  line->setFrameStyle( QFrame::VLine | QFrame::Sunken );
  _extLayout->addMultiCellWidget( m_dbHandlingTab, 0,4,0,1 );
  _extLayout->addMultiCellWidget( line, 0,4,2,2 );
  _extLayout->addMultiCellWidget( clear, 0,0,3,3 );
  _extLayout->addMultiCellWidget( verify, 1,1,3,3 );
  _extLayout->addMultiCellWidget( find, 2,2,3,3 );
  _extLayout->addMultiCellWidget( add, 3,3,3,3 );
  _extLayout->setColStretch( 0, 1 );

  find->setDisabled( true );
  add->setDisabled( true );

  frameLayout->addWidget( localServerSettings, 0, 0 );
  frameLayout->addWidget( _groupExtension, 1, 0 );
  frameLayout->setRowStretch( 1, 1 );
  connect( browse, SIGNAL( clicked() ), this, SLOT( browseDb( )) );
  connect( clear, SIGNAL( clicked() ), this, SLOT( clearDb( )) );
  connect( verify, SIGNAL( clicked() ), this, SLOT( verifyDb( )) );
}
// slots
// ------------------------------------------------------
void K3bCddbLocalDBTab::browseDb(){
  QString path = KFileDialog::getOpenFileName( QDir::homeDirPath(), QString("*.xml"), this, i18n("Select Ripping Directory") );
  if( !path.isEmpty() ) {
    m_songListPath->setText( path );
  }
}
void K3bCddbLocalDBTab::clearDb(){
    kdDebug() << "(K3bCddbLocalDBTab) Clear Database." << endl;
    K3bSongManager *sm = k3bMain()->songManager();
    QStringList::Iterator it;
    for( it = m_missingSongList.begin(); it != m_missingSongList.end(); ++it ){
        sm->deleteSong( (*it) );
    }
    sm->save();

}
void K3bCddbLocalDBTab::verifyDb(){
    kdDebug() << "(K3bCddbLocalDBTab) Verify Database." << endl;
    K3bSongManager *sm = k3bMain()->songManager();
    QFile f( m_songListPath->text() );
    if( f.exists() ) {
        sm->load( m_songListPath->text() );
        m_missingSongList = sm->verify();
        kdDebug() << "(K3bCddbLocalDBTab) Have missing songs." << endl;
        QStringList::Iterator it;
        for( it = m_missingSongList.begin(); it != m_missingSongList.end(); ++it ){
            m_logOutput->insertLine( (*it).local8Bit() );
        }
    } else {
        QMessageBox::critical( this, i18n("Database Error"), 
			       i18n("File doesn't exists <%1>.").arg(m_songListPath->text()), i18n("OK") );
    }
}
void K3bCddbLocalDBTab::findDbEntries(){
    kdDebug() << "(K3bCddbLocalDBTab) find Database." << endl;
}
void K3bCddbLocalDBTab::addDbEntry(){
    kdDebug() << "(K3bCddbLocalDBTab) add Database." << endl;
}
// reading and writing settings
// --------------------------------------------------
void K3bCddbLocalDBTab::apply(){
    KConfig* c = kapp->config();
    c->setGroup("Cddb");
    c->writeEntry( "songlistPath", m_songListPath->text() );
   c->sync();
}

void K3bCddbLocalDBTab::readSettings(){
  KConfig *c = kapp->config();
  c->setGroup("Cddb");
  QString path = c->readEntry("songlistPath");
  if( path == 0 )
    path = locateLocal("appdata", "k3b") + "/" + DEFAULT_SONGLIST_FILE;
  m_songListPath->setText( path );
}

#include "k3bcddblocaldbtab.moc"
