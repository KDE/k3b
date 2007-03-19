/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bpassivepopup.h"
#include "k3bwidgetshoweffect.h"
#include "k3btimeoutwidget.h"
#include "k3bminibutton.h"

#include "k3bthememanager.h"
#include <k3bapplication.h>

#include <kapplication.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kguiitem.h>
#include <kpushbutton.h>
#include <kactivelabel.h>

#include <qtimer.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qmainwindow.h>
#include <qmessagebox.h>
#include <qstyle.h>
#include <qtooltip.h>
#include <qfont.h>


static const char* const sticky_xpm[] = {
  "5 5 2 1",
  "# c black",
  ". c None",
  "#####",
  "#...#",
  "#...#",
  "#...#",
  "#####"
};

static QPixmap themedMessageBoxIcon( K3bPassivePopup::MessageType mt )
{
  QString icon_name;
  QMessageBox::Icon qIcon;
  
  switch( mt ) {
  case K3bPassivePopup::Information:
    qIcon = QMessageBox::Information;
    icon_name = "messagebox_info";
    break;
  case K3bPassivePopup::Warning:
    qIcon = QMessageBox::Warning;
    icon_name = "messagebox_warning";
    break;
  case K3bPassivePopup::Error:
    qIcon = QMessageBox::Critical;
    icon_name = "messagebox_critical";
    break;
  default:
    return QPixmap();
    break;
  }

  QPixmap ret = KApplication::kApplication()->iconLoader()->loadIcon(icon_name, KIcon::NoGroup, KIcon::SizeMedium, KIcon::DefaultState, 0, true);
  
  if( ret.isNull() )
    return QMessageBox::standardIcon( qIcon );
  else
    return ret;
}


class K3bPassivePopup::Private
{
public:
  int timeout;
  int showEffect;

  K3bTimeoutWidget* timeoutWidget;
  QLabel* titleLabel;
  KActiveLabel* messageLabel;
  QLabel* pixmapLabel;
  K3bMiniButton* closeButton;
  K3bMiniButton* stickyButton;
};


K3bPassivePopup::K3bPassivePopup( QWidget* parent )
  : QFrame( parent )
{
  d = new Private;
  d->timeout = 6000;
  d->showEffect = 0;

  setFrameStyle( QFrame::StyledPanel | QFrame::Raised );
  //  setWFlags( Qt::WX11BypassWM );

  QVBoxLayout* mainLay = new QVBoxLayout( this );
  mainLay->setMargin( frameWidth() );
  mainLay->setSpacing( 0 );

  QGridLayout* grid = new QGridLayout;
  grid->setMargin( 9 );
  grid->setSpacing( 6 );

  d->titleLabel = new QLabel( this );
  d->titleLabel->setMargin( 5 );
  d->titleLabel->setAlignment( Qt::AlignCenter );
  QFont fnt( d->titleLabel->font() );
  fnt.setBold( true );
  d->titleLabel->setFont( fnt );

  d->messageLabel = new KActiveLabel( this );

  d->pixmapLabel = new QLabel( this );
  d->pixmapLabel->setAlignment( Qt::AlignTop );

  d->timeoutWidget = new K3bTimeoutWidget( this );
  connect( d->timeoutWidget, SIGNAL(timeout()), this, SLOT(slotClose()) );

  d->closeButton = new K3bMiniButton( d->titleLabel );
  d->closeButton->setPixmap( style().stylePixmap( QStyle::SP_TitleBarCloseButton, this ) );
  d->closeButton->setFixedSize( d->closeButton->pixmap()->width(), d->closeButton->pixmap()->height() );
  QToolTip::add( d->closeButton, i18n("Close") );
  connect( d->closeButton, SIGNAL(clicked()), this, SLOT(slotClose()) );

  d->stickyButton = new K3bMiniButton( d->titleLabel );
  d->stickyButton->setToggleButton( true );
  d->stickyButton->setPixmap( const_cast< const char** >( sticky_xpm ) );
  d->stickyButton->setFixedSize( d->closeButton->pixmap()->width(), d->closeButton->pixmap()->height() );
  QToolTip::add( d->stickyButton, i18n("Keep Open") );
  connect( d->stickyButton, SIGNAL(toggled(bool)), this, SLOT(slotSticky(bool)) );

  grid->addWidget( d->pixmapLabel, 0, 0 );
  grid->addWidget( d->messageLabel, 0, 1 );
  grid->addWidget( d->timeoutWidget, 0, 2 );
  grid->setColStretch( 1, 1 );

  mainLay->addWidget( d->titleLabel );
  mainLay->addLayout( grid, 1 );

  QHBoxLayout* titleLay = new QHBoxLayout( d->titleLabel );
  titleLay->setMargin( d->titleLabel->margin() );
  titleLay->setSpacing( 2 );
  titleLay->addStretch();
  titleLay->addWidget( d->stickyButton );
  titleLay->addWidget( d->closeButton );

  if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
    d->titleLabel->setPaletteBackgroundColor( theme->backgroundColor() );
    d->titleLabel->setPaletteForegroundColor( theme->foregroundColor() );
  }

  setTitle( QString::null );
  setMessageType( Information );
}


K3bPassivePopup::~K3bPassivePopup()
{
  delete d;
}


void K3bPassivePopup::setShowCloseButton( bool b )
{
  d->closeButton->setShown( b );
  adjustSize();
}


void K3bPassivePopup::setShowCountdown( bool b )
{
  d->timeoutWidget->setShown( b );
  d->stickyButton->setShown( b );
}


void K3bPassivePopup::setMessage( const QString& m )
{
  d->messageLabel->setText( "<qt>" + m );
  adjustSize();
}


void K3bPassivePopup::setTitle( const QString& t )
{
  d->titleLabel->setText( t );
  //  d->titleLabel->setShown( !t.isEmpty() );
  adjustSize();
}


void K3bPassivePopup::setTimeout( int msecs )
{
  d->timeout = msecs;
}


void K3bPassivePopup::setMessageType( MessageType m )
{
  d->pixmapLabel->setPixmap( themedMessageBoxIcon( m ) );
  adjustSize();
}


void K3bPassivePopup::slideIn()
{
  d->showEffect = K3bWidgetShowEffect::Slide;
  connect( K3bWidgetShowEffect::showWidget( this, (K3bWidgetShowEffect::Effect)d->showEffect ), SIGNAL(widgetShown(QWidget*)),
	   this, SLOT(slotShown()) );  
}


void K3bPassivePopup::slotShown()
{
  if( d->timeoutWidget->isVisible() ) {
    d->timeoutWidget->setTimeout( d->timeout );
    d->timeoutWidget->start();
  }
  else
    QTimer::singleShot( d->timeout, this, SLOT(slotClose()) );
}


void K3bPassivePopup::slotHidden()
{
  deleteLater();
}


void K3bPassivePopup::slotClose()
{
  if( d->showEffect != 0 ) {
    connect( K3bWidgetShowEffect::hideWidget( this, (K3bWidgetShowEffect::Effect)d->showEffect ), SIGNAL(widgetHidden(QWidget*)),
	     this, SLOT(slotHidden()) );
  }
  else
    deleteLater();
}


void K3bPassivePopup::slotSticky( bool b )
{
  if( b ) {
    d->timeoutWidget->pause();
  }
  else {
    d->timeoutWidget->resume();
  }
}


void K3bPassivePopup::showPopup( const QString& message, 
				 const QString& title, 
				 MessageType messageType,
				 bool countdown,
				 bool button )
{
  K3bPassivePopup* pop = new K3bPassivePopup( static_cast<QMainWindow*>(qApp->mainWidget())->centralWidget() );
  pop->setMessage( message );
  pop->setTitle( title );
  pop->setMessageType( messageType );
  pop->setShowCloseButton( button );
  pop->setShowCountdown( countdown );
  pop->slideIn();
}

#include "k3bpassivepopup.moc"
