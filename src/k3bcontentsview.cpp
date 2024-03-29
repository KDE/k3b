/*
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "k3bcontentsview.h"

#include "k3bthemedheader.h"

#include <QLabel>
#include <QVBoxLayout>


K3b::ContentsView::ContentsView( bool withHeader,
                                 QWidget* parent )
    : QWidget( parent ),
      m_header(0),
      m_centerWidget(0),
      m_active(false)
{
    if( withHeader ) {
        QVBoxLayout* lay = new QVBoxLayout( this );
        lay->setContentsMargins( 0, 0, 0, 0 );
        lay->setSpacing( 0 );

        m_header = new K3b::ThemedHeader( this );
        lay->addWidget( m_header );

        m_header->setLeftPixmap( K3b::Theme::MEDIA_LEFT );
        m_header->setRightPixmap( K3b::Theme::MEDIA_NONE );
    }
}


K3b::ContentsView::~ContentsView()
{
}


void K3b::ContentsView::setMainWidget( QWidget* w )
{
    m_centerWidget = w;
    layout()->addWidget( w );
}


QWidget* K3b::ContentsView::mainWidget()
{
    if( !m_centerWidget )
        setMainWidget( new QWidget( this ) );
    return m_centerWidget;
}


void K3b::ContentsView::setTitle( const QString& title, const QString& subtitle )
{
    if( m_header )
        m_header->setTitle( title, subtitle );
}


void K3b::ContentsView::setLeftPixmap( K3b::Theme::PixmapType s )
{
    if( m_header )
        m_header->setLeftPixmap( s );
}


void K3b::ContentsView::setRightPixmap( K3b::Theme::PixmapType s )
{
    if( m_header )
        m_header->setRightPixmap( s );
}


void K3b::ContentsView::activate( bool active )
{
    m_active = active;
}

#include "moc_k3bcontentsview.cpp"
