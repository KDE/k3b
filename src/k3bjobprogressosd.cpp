/*
 *
 * Copyright (C) 2005-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bjobprogressosd.h"

#include "k3bthememanager.h"
#include "k3bapplication.h"

#include <kwindowsystem.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kcursor.h>
#include <kconfig.h>
#include <klocale.h>
#include <kmenu.h>
#include <KConfigGroup>
#include <KGlobalSettings>

#include <qpixmap.h>
#include <qpainter.h>
#include <qapplication.h>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QDesktopWidget>


K3b::JobProgressOSD::JobProgressOSD( QWidget* parent )
    : QWidget( parent, Qt::Window | Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint ),
      m_progress(0),
      m_dragging(false),
      m_screen(0),
      m_position(s_outerMargin, s_outerMargin)
{
    setFocusPolicy( Qt::NoFocus );

    // dummy size
    resize( 20, 20 );

    // make sure we are always visible
    KWindowSystem::setOnAllDesktops( winId(), true );

    connect( k3bappcore->themeManager(), SIGNAL(themeChanged()),
             this, SLOT(update()) );
    connect( KGlobalSettings::self(), SIGNAL(appearanceChanged()),
             this, SLOT(update()) );
}


K3b::JobProgressOSD::~JobProgressOSD()
{
}


void K3b::JobProgressOSD::show()
{
    // start with 0 progress
    setProgress(0);

    // reposition the osd
    reposition();

    QWidget::show();
}


void K3b::JobProgressOSD::setText( const QString& text )
{
    if( m_text != text ) {
        m_text = text;
        reposition();
    }
}


void K3b::JobProgressOSD::setProgress( int p )
{
    if( m_progress != p ) {
        m_progress = p;
        update();
    }
}


void K3b::JobProgressOSD::setPosition( const QPoint& p )
{
    m_position = p;
    reposition();
}


void K3b::JobProgressOSD::setScreen( int screen )
{
    const int n = QApplication::desktop()->numScreens();
    m_screen = (screen >= n) ? n-1 : screen;
    reposition();
}


void K3b::JobProgressOSD::reposition()
{
    QPixmap icon = KIconLoader::global()->loadIcon( "k3b", KIconLoader::NoGroup, 32 );
    int margin = 10;
    int textWidth = fontMetrics().width( m_text );

    // do not change the size every time the text changes, just in case we are too small
    QSize newSize( qMax( qMax( 2*margin + icon.width() + margin + textWidth, 100 ), width() ),
                   qMax( 2*margin + icon.height(), 2*margin + fontMetrics().height()*2 ) );

    QPoint newPos = m_position;
    const QRect screen = QApplication::desktop()->screenGeometry( m_screen );

    // now to properly resize if put into one of the corners we interpret the position
    // depending on the quadrant
    int midH = screen.width()/2;
    int midV = screen.height()/2;
    if( newPos.x() > midH )
        newPos.rx() -= newSize.width();
    if( newPos.y() > midV )
        newPos.ry() -= newSize.height();

    newPos = fixupPosition( newPos );

    // correct for screen position
    newPos += screen.topLeft();

    setGeometry( QRect( newPos, newSize ) );

    update();
}


void K3b::JobProgressOSD::paintEvent( QPaintEvent* )
{
    // ----------------------------------------
    // |        Copying CD                    |
    // |  K3B   ========== 40%                |
    // |                                      |
    // ----------------------------------------

    // calculate needed size
    if( K3b::Theme* theme = k3bappcore->themeManager()->currentTheme() ) {
        QPixmap icon = KIconLoader::global()->loadIcon( "k3b", KIconLoader::NoGroup, 32 );
        int margin = 10;
        int textWidth = fontMetrics().width( m_text );

        // do not change the size every time the text changes, just in case we are too small
        QSize newSize( qMax( qMax( 2*margin + icon.width() + margin + textWidth, 100 ), width() ),
                       qMax( 2*margin + icon.height(), 2*margin + fontMetrics().height()*2 ) );

        QPainter p( this );

        p.setPen( theme->foregroundColor() );

        // draw the background and the frame
        QRect thisRect( 0, 0, newSize.width(), newSize.height() );
        p.fillRect( thisRect, theme->backgroundColor() );
        p.drawRect( thisRect );

        // draw the k3b icon
        p.drawPixmap( margin, (newSize.height()-icon.height())/2, icon );

        // draw the text
        QSize textSize = fontMetrics().size( 0, m_text );
        int textX = 2*margin + icon.width();
        int textY = margin + fontMetrics().ascent();
        p.drawText( textX, textY, m_text );

        // draw the progress
        textY += fontMetrics().descent() + 4;
        QRect progressRect( textX, textY, newSize.width()-textX-margin, newSize.height()-textY-margin );
        p.drawRect( progressRect );
        progressRect.setWidth( m_progress > 0 ? m_progress*progressRect.width()/100 : 0 );
        p.fillRect( progressRect, theme->foregroundColor() );
    }
}


void K3b::JobProgressOSD::mousePressEvent( QMouseEvent* e )
{
    m_dragOffset = e->pos();

    if( e->button() == Qt::LeftButton && !m_dragging ) {
        grabMouse( Qt::SizeAllCursor );
        m_dragging = true;
    }
    else if( e->button() == Qt::RightButton ) {
        KMenu m;
        QAction *hideOSD = m.addAction(i18n("Hide OSD"));
        QAction *a = m.exec( e->globalPos() );
        if( a == hideOSD)
            hide();
    }
}


void K3b::JobProgressOSD::mouseReleaseEvent( QMouseEvent* )
{
    if( m_dragging ) {
        m_dragging = false;
        releaseMouse();
    }
}


void K3b::JobProgressOSD::mouseMoveEvent( QMouseEvent* e )
{
    if( m_dragging && this == mouseGrabber() ) {

        // check if the osd has been dragged out of the current screen
        int currentScreen = QApplication::desktop()->screenNumber( e->globalPos() );
        if( currentScreen != -1 )
            m_screen = currentScreen;

        const QRect screen = QApplication::desktop()->screenGeometry( m_screen );

        // make sure the position is valid
        m_position = fixupPosition( e->globalPos() - m_dragOffset - screen.topLeft() );

        // move us to the new position
        move( m_position );

        // fix the position
        int midH = screen.width()/2;
        int midV = screen.height()/2;
        if( m_position.x() + width() > midH )
            m_position.rx() += width();
        if( m_position.y() + height() > midV )
            m_position.ry() += height();
    }
}


QPoint K3b::JobProgressOSD::fixupPosition( const QPoint& pp )
{
    QPoint p(pp);
    const QRect& screen = QApplication::desktop()->screenGeometry( m_screen );
    int maxY = screen.height() - height() - s_outerMargin;
    int maxX = screen.width() - width() - s_outerMargin;

    if( p.y() < s_outerMargin )
        p.ry() = s_outerMargin;
    else if( p.y() > maxY )
        p.ry() = maxY;

    if( p.x() < s_outerMargin )
        p.rx() = s_outerMargin;
    else if( p.x() > maxX )
        p.rx() = screen.width() - s_outerMargin - width();

    p += screen.topLeft();

    return p;
}


void K3b::JobProgressOSD::readSettings( const KConfigGroup& c )
{
    setPosition( c.readEntry( "Position", QPoint() ) );
    setScreen( c.readEntry( "Screen", 0 ) );
}


void K3b::JobProgressOSD::saveSettings( KConfigGroup c )
{
    c.writeEntry( "Position", m_position );
    c.writeEntry( "Screen", m_screen );
}

#include "k3bjobprogressosd.moc"
