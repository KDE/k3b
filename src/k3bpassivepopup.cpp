/*
 *
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bpassivepopup.h"
#include "k3b.h"
#include "k3bapplication.h"
#include "k3bthememanager.h"
#include "k3btimeoutwidget.h"
#include "k3bminibutton.h"
#include "k3bwidgetshoweffect.h"

#include <KApplication>
#include <KIconLoader>
#include <KLocale>

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>


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
        icon_name = "dialog-information";
        break;
    case K3bPassivePopup::Warning:
        qIcon = QMessageBox::Warning;
        icon_name = "dialog-warning";
        break;
    case K3bPassivePopup::Error:
        qIcon = QMessageBox::Critical;
        icon_name = "dialog-error";
        break;
    default:
        return QPixmap();
        break;
    }
    
    QPixmap ret = KIconLoader::global()->loadIcon(icon_name, KIconLoader::NoGroup, KIconLoader::SizeMedium, KIconLoader::DefaultState, QStringList(), 0,true);
    
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
    QLabel* messageLabel;
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
    setAutoFillBackground( true );
    
    d->titleLabel = new QLabel( this );
    d->titleLabel->setMargin( 5 );
    d->titleLabel->setAlignment( Qt::AlignCenter );
    d->titleLabel->setAutoFillBackground( true );
    QFont fnt( d->titleLabel->font() );
    fnt.setBold( true );
    d->titleLabel->setFont( fnt );
    
    d->messageLabel = new QLabel( this );
    d->messageLabel->setTextInteractionFlags( Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse );
    
    d->pixmapLabel = new QLabel( this );
    d->pixmapLabel->setAlignment( Qt::AlignTop );
    d->pixmapLabel->setPixmap( themedMessageBoxIcon( Information ) );
    
    d->timeoutWidget = new K3bTimeoutWidget( this );
    connect( d->timeoutWidget, SIGNAL(timeout()), this, SLOT(slotClose()) );
    
    d->closeButton = new K3bMiniButton( d->titleLabel );
    d->closeButton->setIcon( style()->standardIcon( QStyle::SP_TitleBarCloseButton ) );
    d->closeButton->setFixedSize( 16, 16 );
    d->closeButton->setToolTip( i18n("Close") );
    connect( d->closeButton, SIGNAL(clicked()), this, SLOT(slotClose()) );
    
    d->stickyButton = new K3bMiniButton( d->titleLabel );
    d->stickyButton->setCheckable( true );
    d->stickyButton->setIcon( QIcon( QPixmap( const_cast< const char** >( sticky_xpm ) ) ) );
    d->stickyButton->setFixedSize( 16, 16 );
    d->stickyButton->setToolTip( i18n("Keep Open") );
    connect( d->stickyButton, SIGNAL(toggled(bool)), this, SLOT(slotSticky(bool)) );

    QHBoxLayout* titleLay = new QHBoxLayout( d->titleLabel );
    titleLay->setMargin( d->titleLabel->margin() );
    titleLay->setSpacing( 2 );
    titleLay->addStretch();
    titleLay->addWidget( d->stickyButton );
    titleLay->addWidget( d->closeButton );

    QHBoxLayout* msgLay = new QHBoxLayout;
    msgLay->setMargin( 9 );
    msgLay->setSpacing( 6 );
    msgLay->addWidget( d->pixmapLabel );
    msgLay->addWidget( d->messageLabel, 1 );
    msgLay->addWidget( d->timeoutWidget );

    QVBoxLayout* mainLay = new QVBoxLayout( this );
    mainLay->setMargin( frameWidth() );
    mainLay->setSpacing( 0 );
    mainLay->addWidget( d->titleLabel );
    mainLay->addLayout( msgLay, 1 );

    if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
        QPalette titlePalette;
        titlePalette.setColor( d->titleLabel->backgroundRole(), theme->backgroundColor() );
        titlePalette.setColor( d->titleLabel->foregroundRole(), theme->foregroundColor() );
        d->titleLabel->setPalette( titlePalette );
    }
}


K3bPassivePopup::~K3bPassivePopup()
{
    delete d;
}


void K3bPassivePopup::setShowCloseButton( bool b )
{
    d->closeButton->setVisible( b );
    adjustSize();
}


void K3bPassivePopup::setShowCountdown( bool b )
{
    d->timeoutWidget->setVisible( b );
    d->stickyButton->setVisible( b );
}


void K3bPassivePopup::setMessage( const QString& m )
{
    d->messageLabel->setText( m );
    adjustSize();
}


void K3bPassivePopup::setTitle( const QString& t )
{
    d->titleLabel->setText( t );
    //  d->titleLabel->setVisible( !t.isEmpty() );
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
    K3bPassivePopup* pop = new K3bPassivePopup( k3bappcore->k3bMainWindow()->centralWidget() );
    pop->setMessage( message );
    pop->setTitle( title );
    pop->setMessageType( messageType );
    pop->setShowCloseButton( button );
    pop->setShowCountdown( countdown );
    pop->slideIn();
}

#include "k3bpassivepopup.moc"
