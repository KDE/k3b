/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2006 Sebastian Trueg <trueg@k3b.org>
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


K3bPassivePopup::K3bPassivePopup( QWidget* parent )
: QFrame( parent ),
  m_timeout(10000),
  m_showEffect(0),
  m_timeoutWidget(0)
{
  setFrameStyle( QFrame::StyledPanel | QFrame::Raised );
  setWFlags( Qt::WX11BypassWM );

  QVBoxLayout* mainLay = new QVBoxLayout( this );
  mainLay->setMargin( frameWidth() );
  mainLay->setSpacing( 0 );

  QGridLayout* grid = new QGridLayout;
  grid->setMargin( 9 );
  grid->setSpacing( 6 );

  m_titleLabel = new QLabel( this );
  m_titleLabel->setMargin( 3 );
  m_titleLabel->setAlignment( Qt::AlignCenter );
  //  m_titleLabel->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );

  m_messageLabel = new KActiveLabel( this );

  m_pixmapLabel = new QLabel( this );
  m_pixmapLabel->setAlignment( Qt::AlignTop );

  m_timeoutWidget = new K3bTimeoutWidget( this );
  connect( m_timeoutWidget, SIGNAL(timeout()), this, SLOT(slotClose()) );

  m_closeButton = new KPushButton( KStdGuiItem::close(), this );
  connect( m_closeButton, SIGNAL(clicked()), this, SLOT(slotClose()) );


  QHBoxLayout* buttonLay = new QHBoxLayout;
  buttonLay->addItem( new QSpacerItem( 4, 4, QSizePolicy::Expanding, QSizePolicy::Preferred ) );
  buttonLay->addWidget( m_closeButton );

  grid->addWidget( m_pixmapLabel, 0, 0 );
  grid->addWidget( m_messageLabel, 0, 1 );
  grid->addWidget( m_timeoutWidget, 0, 2 );
  grid->addMultiCellLayout( buttonLay, 1, 1, 0, 2 );
  grid->setColStretch( 1, 1 );

  mainLay->addWidget( m_titleLabel );
  mainLay->addLayout( grid );

  if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
    m_titleLabel->setPaletteBackgroundColor( theme->backgroundColor() );
    m_titleLabel->setPaletteForegroundColor( theme->foregroundColor() );
  }

  setTitle( QString::null );
  setMessageType( Information );
}


K3bPassivePopup::~K3bPassivePopup()
{
}


void K3bPassivePopup::setShowCloseButton( bool b )
{
  m_closeButton->setShown( b );
  adjustSize();
}


void K3bPassivePopup::setShowCountdown( bool b )
{
  m_timeoutWidget->setShown( b );
}


void K3bPassivePopup::setMessage( const QString& m )
{
  m_messageLabel->setText( "<qt>" + m );
  adjustSize();
}


void K3bPassivePopup::setTitle( const QString& t )
{
  m_titleLabel->setText( "<qt><b>" + t );
  m_titleLabel->setShown( !t.isEmpty() );
  adjustSize();
}


void K3bPassivePopup::setTimeout( int msecs )
{
  m_timeout = msecs;
}


void K3bPassivePopup::setMessageType( MessageType m )
{
  m_pixmapLabel->setPixmap( themedMessageBoxIcon( m ) );
  adjustSize();
}


void K3bPassivePopup::slideIn()
{
  m_showEffect = K3bWidgetShowEffect::Slide;
  connect( K3bWidgetShowEffect::showWidget( this, (K3bWidgetShowEffect::Effect)m_showEffect ), SIGNAL(widgetShown(QWidget*)),
	   this, SLOT(slotShown()) );  
}


void K3bPassivePopup::slotShown()
{
  if( m_timeoutWidget->isVisible() ) {
    m_timeoutWidget->setTimeout( m_timeout );
    m_timeoutWidget->start();
  }
  else
    QTimer::singleShot( m_timeout, this, SLOT(slotClose()) );
}


void K3bPassivePopup::slotHidden()
{
  deleteLater();
}


void K3bPassivePopup::slotClose()
{
  if( m_showEffect != 0 ) {
    connect( K3bWidgetShowEffect::hideWidget( this, (K3bWidgetShowEffect::Effect)m_showEffect ), SIGNAL(widgetHidden(QWidget*)),
	     this, SLOT(slotHidden()) );
  }
  else
    deleteLater();
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
