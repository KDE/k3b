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

#include <QtWidgets/QApplication>
#include <QDesktopWidget>
#include <QtCore/QEvent>
#include <QtGui/QFontMetrics>
#include <QtWidgets/QLabel>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtCore/QString>

#include <K4AboutData>
#include <KApplication>
#include <KDELibs4Support/KDE/KGlobal>
#include <KDELibs4Support/KDE/KStandardDirs>

K3b::Splash::Splash( QWidget* parent )
    : KVBox( parent)
{
    setMargin( 0 );
    setSpacing( 0 );
    setAttribute( Qt::WA_DeleteOnClose );
    setWindowFlags(Qt::FramelessWindowHint|
                   Qt::SplashScreen|
                   Qt::WindowStaysOnTopHint|
                   Qt::X11BypassWindowManagerHint);

    QPalette pal( palette() );
    pal.setColor( QPalette::Window, Qt::black );
    pal.setColor( QPalette::WindowText, Qt::white );
    setPalette( pal );

    QLabel* copyrightLabel = new QLabel( KGlobal::mainComponent().aboutData()->copyrightStatement(), this );
    copyrightLabel->setMargin( 5 );
    copyrightLabel->setAlignment( Qt::AlignRight );

    QLabel* picLabel = new QLabel( this );
    if( K3b::Theme* theme = k3bappcore->themeManager()->currentTheme() ) {
        picLabel->setPalette( theme->palette() );
        picLabel->setPixmap( theme->pixmap( K3b::Theme::SPLASH ) );
    }

    m_infoBox = new QLabel( this );
    m_infoBox->setMargin( 5 );

    // Set geometry, with support for Xinerama systems
    QRect r;
    r.setSize(sizeHint());
    int ps = QApplication::desktop()->primaryScreen();
    r.moveCenter( QApplication::desktop()->screenGeometry(ps).center() );
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
    KVBox::show();
    // make sure the splash screen is shown immediately
    qApp->processEvents();
}


void K3b::Splash::addInfo( const QString& s )
{
    m_infoBox->setText( s );

    qApp->processEvents();
}


// void K3b::Splash::paintEvent( QPaintEvent* e )
// {
//   // first let the window paint the background and the child widget
//   QWidget::paintEvent( e );

//   // now create the text we want to display
//   // find the lower left corner and paint it on top of the pixmap
//   QPainter p( this );
//   p.setPen( Qt::blue );

//   QFontMetrics fm = p.fontMetrics();

//   QString line1 = QString( "K3b version %1" ).arg(VERSION);
//   QString line2( "(c) 2001 by Sebastian Trueg" );
//   QString line3( "licensed under the GPL" );

//   QRect rect1 = fm.boundingRect( line1 );
//   QRect rect2 = fm.boundingRect( line2 );
//   QRect rect3 = fm.boundingRect( line3 );

//   int textH = rect1.height() + rect2.height() + rect3.height() + 2 * fm.leading() + 2 + rect2.height() /*hack because the boundingRect method seems not to work properly! :-(*/;
//   int textW = qMax( rect1.width(), qMax( rect2.width(), rect3.width() ) ) + 2;

//   int startX = 10;
//   int startY = height() - 10 - textH;

//   p.drawText( startX, startY, textW, textH, 0, QString("%1\n%2\n%3").arg(line1).arg(line2).arg(line3) );
// }



