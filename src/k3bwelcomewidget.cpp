/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bwelcomewidget.h"
#include "k3b.h"
#include <k3bstdguiitems.h>
#include <k3bcore.h>
#include <k3bversion.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qpixmap.h>

#include <kurl.h>
#include <kurldrag.h>
#include <klocale.h>
#include <ktextbrowser.h>
#include <kstandarddirs.h>
#include <kapplication.h>



K3bWelcomeWidget::K3bWelcomeWidget( K3bMainWindow* mw, QWidget* parent, const char* name )
  : QWidget( parent, name ),
    m_mainWindow( mw )
{
  setAcceptDrops( true );

  // header
  QFrame* headerFrame = K3bStdGuiItems::purpleFrame( this );
  QLabel* topPixLabel = new QLabel( headerFrame );
  QLabel* topLeftPixLabel = new QLabel( headerFrame );
  QLabel* headerLabel = new QLabel( i18n("Welcome to K3b %1 - The CD/DVD burning facility").arg(k3bcore->version()),
				    headerFrame );
  QFont fnt(headerLabel->font());
  fnt.setBold(true);
  fnt.setPointSize( 14 );
  headerLabel->setFont( fnt );
  headerLabel->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
  headerLabel->setIndent( 5 );
  QGridLayout* headerGrid = new QGridLayout( headerFrame );
  headerGrid->setMargin( 2 );
  headerGrid->setSpacing( 0 );
  headerGrid->addWidget( topLeftPixLabel, 0, 0 );
  headerGrid->addWidget( headerLabel, 0, 1 );
  headerGrid->addWidget( topPixLabel, 0, 2 );
  headerGrid->setColStretch( 1, 1 );

  // text browser
  KTextBrowser* browser = new KTextBrowser( this );
  browser->setNotifyClick( true );

  // layout
  QGridLayout* mainGrid = new QGridLayout( this );
  mainGrid->setMargin( 2 );
  mainGrid->setSpacing( 0 );
  mainGrid->addWidget( headerFrame, 0, 0 );
  mainGrid->addWidget( browser, 1, 0 );
  mainGrid->setRowStretch( 1, 1 );


  topPixLabel->setPixmap( QPixmap(locate( "appdata", "pics/k3bprojectview_right.png" )) );
  topLeftPixLabel->setPixmap( QPixmap(locate( "appdata", "pics/k3bprojectview_left_short.png" )) );

  QFile f( locate( "appdata", "k3bwelcomemessage.html" ) );
  if( f.open( IO_ReadOnly ) ) {
    QTextStream s( &f );
    browser->setText( s.read()
		      .arg( i18n("Use the following links to access the most often used actions:") )
		      .arg( i18n("Create Audio CD") )
  		      .arg( i18n("Create Data DVD") )
    		      .arg( i18n("Create Data CD") )
     		      .arg( i18n("Copy CD") )
		      .arg( i18n("K3b basicly consits of three parts:") )
		      .arg( i18n("The projects:") )
		      .arg( i18n("Projects are created from the <em>file</em> menu and then filled with data to burn.") )
		      .arg( i18n("The tools:") )
		      .arg( i18n("The <em>tools</em> menu offers different tools like CD copy or DVD formatting.") )
		      .arg( i18n("Context sensitive media actions:") )
		      .arg( i18n("When clicking on the Icon representing a CD/DVD drive K3b will present "
				 "it's contents and allow some further action. This is for example the way "
				 "to rip audio CDs.") )
		      );
  }
  else
    browser->setText( i18n("File %1 not found").arg("k3bwelcomemessage.h") );

  connect( browser, SIGNAL(mailClick(const QString&, const QString&)),
	   this, SLOT(slotMailClick(const QString&, const QString&)) );
  connect( browser, SIGNAL(urlClick(const QString&)),
	   this, SLOT(slotUrlClick(const QString&)) );
}


K3bWelcomeWidget::~K3bWelcomeWidget()
{
}


void K3bWelcomeWidget::slotUrlClick( const QString& url )
{
  if( url == "audiocd" )
    m_mainWindow->slotNewAudioDoc();
  else if( url == "datacd" )
    m_mainWindow->slotNewDataDoc();
  else if( url == "datadvd" )
    m_mainWindow->slotNewDvdDoc();
  else if( url == "copycd" )
    m_mainWindow->slotCdCopy();
}


void K3bWelcomeWidget::slotMailClick( const QString& adress, const QString& )
{
  kapp->invokeMailer( adress, "K3b: " );
}

void K3bWelcomeWidget::dragEnterEvent( QDragEnterEvent* event )
{
  event->accept( KURLDrag::canDecode(event) );
}


void K3bWelcomeWidget::dropEvent( QDropEvent* e )
{
  KURL::List urls;
  KURLDrag::decode( e, urls );
  m_mainWindow->addUrls( urls );
}

#include "k3bwelcomewidget.moc"
