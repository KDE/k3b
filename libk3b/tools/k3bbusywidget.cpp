/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bbusywidget.h"

#include <QTimer>
#include <QPainter>
#include <QFrame>


K3b::BusyWidget::BusyWidget( QWidget* parent )
    : QFrame( parent )
{
    m_busyTimer = new QTimer( this );
    m_iBusyPosition = 0;

    connect( m_busyTimer, SIGNAL(timeout()), this, SLOT(animateBusy()) );

    m_bBusy = false;
}

K3b::BusyWidget::~BusyWidget()
{
}


void K3b::BusyWidget::showBusy( bool b )
{
    m_bBusy = b;

//   if( b ) {
//     m_iBusyCounter++;
//   }
//   else if( m_iBusyCounter > 0 ) {
//     m_iBusyCounter--;
//   }

    if( m_bBusy ) {
        if( !m_busyTimer->isActive() )
            m_busyTimer->start( 500 );
    }
    else {
        if( m_busyTimer->isActive() )
            m_busyTimer->stop();
        update();
        m_iBusyPosition = 0;
    }
}


void K3b::BusyWidget::animateBusy()
{
    m_iBusyPosition++;
    update();
}


QSize K3b::BusyWidget::sizeHint() const
{
    return minimumSizeHint();
}


QSize K3b::BusyWidget::minimumSizeHint() const
{
    return QSize( 2*frameWidth() + 62, 10 );
}


void K3b::BusyWidget::paintEvent( QPaintEvent*  )
{
    QPainter p(this);
    QRect rect = contentsRect();

    int squareSize = 8;

    int pos = 2 + m_iBusyPosition * (squareSize + 2);

    // check if the position is in the visible area
    if( pos + 8 + 2> rect.width() ) {
        m_iBusyPosition = 0;
        pos = 2;
    }

    //  p->eraseRect( rect );
    if( m_bBusy )
        p.fillRect( pos, (rect.height() - squareSize)/2, squareSize, squareSize, palette().highlight() );
}

#include "moc_k3bbusywidget.cpp"
