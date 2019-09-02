/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bsplash.h"
#include "k3bthememanager.h"
#include "k3bapplication.h"

#include <KAboutData>

#include <QEvent>
#include <QString>
#include <QFontMetrics>
#include <QPainter>
#include <QPixmap>
#include <QApplication>
#include <QLabel>
#include <QScreen>
#include <QVBoxLayout>

K3b::Splash::Splash( QWidget* parent )
    : QWidget( parent)
{
    setAttribute( Qt::WA_DeleteOnClose );
    setWindowFlags(Qt::FramelessWindowHint|
                   Qt::SplashScreen|
                   Qt::WindowStaysOnTopHint|
                   Qt::X11BypassWindowManagerHint);

    QPalette pal( palette() );
    pal.setColor( QPalette::Window, Qt::black );
    pal.setColor( QPalette::WindowText, Qt::white );
    setPalette( pal );

    QLabel* copyrightLabel = new QLabel( KAboutData::applicationData().copyrightStatement(), this );
    copyrightLabel->setContentsMargins( 5, 5, 5, 5 );
    copyrightLabel->setAlignment( Qt::AlignRight );

    QLabel* picLabel = new QLabel( this );
    if( K3b::Theme* theme = k3bappcore->themeManager()->currentTheme() ) {
        picLabel->setPalette( theme->palette() );
        picLabel->setPixmap( theme->pixmap( K3b::Theme::SPLASH ) );
    }

    m_infoBox = new QLabel( this );
    m_infoBox->setContentsMargins( 5, 5, 5, 5 );

    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setSpacing( 0 );
    layout->addWidget( copyrightLabel );
    layout->addWidget( picLabel );
    layout->addWidget( m_infoBox );

    // Set geometry, with support for Xinerama systems
    QRect r;
    r.setSize(sizeHint());
    const QScreen *ps = QGuiApplication::primaryScreen();
    r.moveCenter( ps->geometry().center() );
    setGeometry(r);
}


K3b::Splash::~Splash()
{
}


void K3b::Splash::mousePressEvent( QMouseEvent* )
{
    close();
}


void K3b::Splash::show()
{
    QWidget::show();
    // make sure the splash screen is shown immediately
    qApp->processEvents();
}


void K3b::Splash::addInfo( const QString& s )
{
    m_infoBox->setText( s );

    qApp->processEvents();
}


void K3b::Splash::hide()
{
    QWidget::hide();
    qApp->processEvents();
}
