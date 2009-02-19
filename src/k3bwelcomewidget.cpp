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

#include "k3bwelcomewidget.h"
#include "k3b.h"
#include "k3bapplication.h"
#include "k3bflatbutton.h"
#include "k3bstdguiitems.h"
#include "k3bthememanager.h"
#include "k3bversion.h"

#include <QCursor>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTextDocument>

#include <k3urldrag.h>
#include <KActionCollection>
#include <KAboutData>
#include <KConfigGroup>
#include <KGlobal>
#include <KGlobalSettings>
#include <KLocale>
#include <KMenu>

static const char* s_allActions[] = {
    "file_new_data",
    "file_continue_multisession",
    "_sep_",
    "file_new_audio",
    "_sep_",
    "file_new_mixed",
    "_sep_",
    "file_new_vcd",
    "file_new_video_dvd",
    "_sep_",
    "file_new_movix",
    "_sep_",
    "tools_copy_medium",
    "_sep_",
    "tools_format_medium",
    "_sep_",
    "tools_write_cd_image",
    "tools_write_dvd_iso",
    "_sep_",
    "tools_cdda_rip",
    "tools_videodvd_rip",
    "tools_videocd_rip",
    0
};

K3bWelcomeWidget::Display::Display( K3bWelcomeWidget* parent )
    : QWidget( parent->viewport() )
{
    m_header = new QTextDocument( this );
    m_infoText = new QTextDocument( this );

    setAcceptDrops( true );
    m_rows = m_cols = 1;

    m_buttonMore = new K3bFlatButton( i18n("More actions..."), this );
    connect( m_buttonMore, SIGNAL(pressed()), parent, SLOT(slotMoreActions()) );

    connect( k3bappcore->themeManager(), SIGNAL(themeChanged()), this, SLOT(slotThemeChanged()) );

    slotThemeChanged();
}


K3bWelcomeWidget::Display::~Display()
{
}


void K3bWelcomeWidget::Display::addAction( QAction* action )
{
    if( action ) {
        m_actions.append(action);
        rebuildGui();
    }
}


void K3bWelcomeWidget::Display::removeAction( QAction* action )
{
    if( action ) {
        m_actions.removeAll( action );
        rebuildGui();
    }
}


void K3bWelcomeWidget::Display::removeButton( K3bFlatButton* b )
{
    removeAction( m_buttonMap[b] );
}


void K3bWelcomeWidget::Display::rebuildGui( const QList<QAction*>& actions )
{
    m_actions = actions;
    rebuildGui();
}


static void calculateButtons( int width, int numActions, int buttonWidth, int& cols, int& rows )
{
    // always try to avoid horizontal scrollbars
    int wa = width - 40;
    cols = qMax( 1, qMin( wa / (buttonWidth+4), numActions ) );
    rows = numActions/cols;
    int over = numActions%cols;
    if( over ) {
        rows++;
        // try to avoid useless cols
        while( over && cols - over - 1 >= rows-1 ) {
            --cols;
            over = numActions%cols;
        }
    }
}


void K3bWelcomeWidget::Display::rebuildGui()
{
    // step 1: delete all old buttons in the buttons QPtrList<K3bFlatButton>
    m_buttonMap.clear();
    qDeleteAll( m_buttons );
    m_buttons.clear();

    int numActions = m_actions.count();
    if( numActions > 0 ) {

        // create buttons
        QList<QAction *> items(m_actions);
        for( QList<QAction *>::const_iterator it = items.constBegin();
            it != items.constEnd(); ++it ) {
            QAction* a = *it;
            K3bFlatButton* b = new K3bFlatButton( a, this );

            m_buttons.append( b );
            m_buttonMap.insert( b, a );
        }

        // determine the needed button size (since all buttons should be equal in size
        // we use the max of all sizes)
        m_buttonSize = m_buttons.first()->sizeHint();
        for ( int i = 0;i<m_buttons.count();i++ )
        {
            m_buttonSize = m_buttonSize.expandedTo( m_buttons.at( i )->sizeHint() );
        }

        repositionButtons();
    }
}


void K3bWelcomeWidget::Display::repositionButtons()
{
    // calculate rows and columns
    calculateButtons( width(), m_actions.count(), m_buttonSize.width(), m_cols, m_rows );

    int availHor = width() - 40;
    int availVert = height() - 20 - 10 - ( int )m_header->size().height() - 10;
    availVert -= ( int )m_infoText->size().height() - 10;
    int leftMargin = 20 + (availHor - (m_buttonSize.width()+4)*m_cols)/2;
    int topOffset = ( int )m_header->size().height() + 20 + ( availVert - (m_buttonSize.height()+4)*m_rows - m_buttonMore->height() )/2;

    int row = 0;
    int col = 0;

    for ( int i = 0;i<m_buttons.count();i++ )
    {
        K3bFlatButton* b = m_buttons.at( i );

        b->setGeometry( QRect( QPoint( leftMargin + (col*(m_buttonSize.width()+4) + 2 ),
                                       topOffset + (row*(m_buttonSize.height()+4)) + 2 ),
                               m_buttonSize ) );
        b->show();

        col++;
        if( col == m_cols ) {
            col = 0;
            row++;
        }
    }
    if( col > 0 )
        ++row;

    m_buttonMore->setGeometry( QRect( QPoint( leftMargin + 2,
                                              topOffset + (row*(m_buttonSize.height()+4)) + 2 ),
                                      QSize( m_cols*(m_buttonSize.width()+4) - 4, m_buttonMore->height() ) ) );
}


QSizePolicy K3bWelcomeWidget::Display::sizePolicy () const
{
    return QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
}


int K3bWelcomeWidget::Display::heightForWidth( int w ) const
{
    int ow = ( int )m_infoText->idealWidth();
    m_infoText->setTextWidth( w );
    int h = ( int )m_infoText->size().height();
    m_infoText->setTextWidth( ow );

    int cols, rows;
    calculateButtons( w, m_actions.count(), m_buttonSize.width(), cols, rows );

    return (20 + ( int )m_header->size().height() + 20 + 10 + ((m_buttonSize.height()+4)*rows) + 4 + m_buttonMore->height() + 10 + h + 20);
}


QSize K3bWelcomeWidget::Display::minimumSizeHint() const
{
    QSize size( qMax(40+( int )m_header->idealWidth(), 40+m_buttonSize.width()),
                20 + ( int )m_header->size().height() + 20 + 10 + m_buttonSize.height() + 10 + ( int )m_infoText->size().height() + 20 );

    return size;
}


void K3bWelcomeWidget::Display::resizeEvent( QResizeEvent* e )
{
    m_infoText->setTextWidth( width() - 20 );
    QWidget::resizeEvent(e);
    repositionButtons();
    if( e->size() != m_bgPixmap.size() )
        updateBgPix();
}


void K3bWelcomeWidget::Display::slotThemeChanged()
{
    if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
//         if( theme->backgroundMode() == K3bTheme::BG_SCALE )
//             m_bgImage = theme->pixmap( K3bTheme::WELCOME_BG ).convertToImage();
        m_header->setDefaultStyleSheet( QString("body { font-size: 16pt; font-weight: bold; color: %1 }")
                                        .arg(theme->foregroundColor().name()) );
        m_infoText->setDefaultStyleSheet( QString("body { color: %1 }")
                                          .arg(theme->foregroundColor().name()) );
    }

    m_header->setHtml( "<html><body align=\"center\">" + i18n("Welcome to K3b - The CD and DVD Kreator") + "</body></html>" );
    m_infoText->setHtml( QString::fromUtf8("<html><body align=\"center\">K3b %1 (c) 1999 - 2009 Sebastian Trüg</body></html>")
                         .arg(KGlobal::mainComponent().aboutData()->version()) );
    updateBgPix();
    update();
}


void K3bWelcomeWidget::Display::updateBgPix()
{
    if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
        if( theme->backgroundMode() == K3bTheme::BG_SCALE )
            m_bgPixmap = theme->pixmap( K3bTheme::WELCOME_BG ).scaled( rect().width(), rect().height() );
        else
            m_bgPixmap = theme->pixmap( K3bTheme::WELCOME_BG );
    }
}


void K3bWelcomeWidget::Display::paintEvent( QPaintEvent* )
{
    if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
        QPainter p( this );
        p.setPen( theme->foregroundColor() );

        // draw the background including first filling with the bg color for transparent images
        p.fillRect( rect(), theme->backgroundColor() );
        p.drawTiledPixmap( rect(), m_bgPixmap );

        // rect around the header
        QRect rect( 10, 10, qMax( ( int )m_header->idealWidth() + 20, width() - 20 ), ( int )m_header->size().height() + 20 );
        p.fillRect( rect, theme->backgroundColor() );
        p.drawRect( rect );

        // big rect around the whole thing
        p.drawRect( 10, 10, width()-20, height()-20 );

        // draw the header text
        int pos = 20;
        pos += qMax( (width()-40-( int )m_header->idealWidth())/2, 0 );
        p.save();
        p.translate( pos, 20 );
        m_header->drawContents( &p );
        p.restore();

        // draw the info box
        //    int boxWidth = 20 + m_infoText->widthUsed();
        int boxHeight = 10 + ( int )m_infoText->size().height();
        QRect infoBoxRect( 10/*qMax( (width()-20-m_infoText->widthUsed())/2, 10 )*/,
                           height()-10-boxHeight,
                           width()-20/*boxWidth*/,
                           boxHeight );
        p.fillRect( infoBoxRect, theme->backgroundColor() );
        p.drawRect( infoBoxRect );
        p.save();
        p.translate( infoBoxRect.left()+5, infoBoxRect.top()+5 );
        m_infoText->drawContents( &p );
        p.restore();
    }
}


void K3bWelcomeWidget::Display::dragEnterEvent( QDragEnterEvent* event )
{
    event->setAccepted( K3URLDrag::canDecode(event) );
}


void K3bWelcomeWidget::Display::dropEvent( QDropEvent* e )
{
    KUrl::List urls;
    K3URLDrag::decode( e, urls );
    emit dropped( urls );
}



K3bWelcomeWidget::K3bWelcomeWidget( K3bMainWindow* mw, QWidget* parent )
    : QScrollArea( parent ),
      m_mainWindow( mw )
{
    main = new Display( this );
    setWidget( main );
    setFrameStyle( QFrame::NoFrame );

    connect( main, SIGNAL(dropped(const KUrl::List&)), m_mainWindow, SLOT(addUrls(const KUrl::List&)) );

    connect( KGlobalSettings::self(), SIGNAL(appearanceChanged()), main, SLOT(update()) );
}


K3bWelcomeWidget::~K3bWelcomeWidget()
{
}


void K3bWelcomeWidget::loadConfig( const KConfigGroup& c )
{
    QStringList sl = c.readEntry( "welcome_actions", QStringList() );

    if( sl.isEmpty() ) {
        sl.append( "file_new_data" );
        sl.append( "file_new_audio" );
        sl.append( "tools_copy_medium" );
    }

    QList<QAction*> actions;
    for( QStringList::const_iterator it = sl.constBegin(); it != sl.constEnd(); ++it )
        if( QAction* a = m_mainWindow->actionCollection()->action( *it ) )
            actions.append(a);

    main->rebuildGui( actions );

    fixSize();
}


void K3bWelcomeWidget::saveConfig( KConfigGroup& c )
{
    QStringList sl;
    QList<QAction *> items(main->m_actions);
    for( QList<QAction *>::const_iterator it = items.constBegin();
            it != items.constEnd(); ++it )
        sl.append( (*it)->objectName() );

    c.writeEntry( "welcome_actions", sl );
}


void K3bWelcomeWidget::resizeEvent( QResizeEvent* e )
{
    fixSize();
    QScrollArea::resizeEvent( e );
}


void K3bWelcomeWidget::showEvent( QShowEvent* e )
{
    fixSize();
    QScrollArea::showEvent( e );
}


void K3bWelcomeWidget::fixSize()
{
    QSize s = contentsRect().size();
    s.setWidth( qMax( main->minimumSizeHint().width(), s.width() ) );
    s.setHeight( qMax( main->heightForWidth(s.width()), s.height() ) );

    main->resize( s );
    viewport()->resize( s );
}


void K3bWelcomeWidget::mousePressEvent ( QMouseEvent* e )
{
    if( e->button() == Qt::RightButton ) {
        QMap<QAction*, QAction*> map;
        KMenu addPop;
        addPop.setTitle( i18n("Add Button") );

        QAction* firstAction = 0;
        for ( int i = 0; s_allActions[i]; ++i ) {
            if ( s_allActions[i][0] != '_' ) {
                QAction* a = m_mainWindow->actionCollection()->action( s_allActions[i] );
                if ( a && !main->m_actions.contains(a) ) {
                    QAction* addAction = addPop.addAction( a->icon(), a->text() );
                    map.insert( addAction, a );
                    if ( !firstAction )
                        firstAction = addAction;
                }
            }
        }

        // menu identifiers in QT are always < 0 (when automatically generated)
        // and unique throughout the entire application!
        QAction *r = 0;
        QAction *removeAction = 0;

        QWidget* widgetAtPos = viewport()->childAt(e->pos());
        if( widgetAtPos && widgetAtPos->inherits( "K3bFlatButton" ) ) {
            KMenu pop;
            removeAction = pop.addAction( SmallIcon("list-remove"), i18n("Remove Button") );
            if ( addPop.actions().count() > 0 )
                pop.addMenu( &addPop );
            r = pop.exec( e->globalPos() );
        }
        else {
            addPop.addTitle( addPop.title(), firstAction );
            r = addPop.exec( e->globalPos() );
        }

        if( r != 0 ) {
            if( r == removeAction )
                main->removeButton( static_cast<K3bFlatButton*>(widgetAtPos) );
            else
                main->addAction( map[r] );
        }

        fixSize();
    }
}


void K3bWelcomeWidget::slotMoreActions()
{
    KMenu popup;

    for ( int i = 0; s_allActions[i]; ++i ) {
        if ( s_allActions[i][0] == '_' ) {
            popup.addSeparator();
        }
        else {
            popup.addAction(m_mainWindow->actionCollection()->action( s_allActions[i] ));
        }
    }

    popup.exec( QCursor::pos() );
}

#include "k3bwelcomewidget.moc"
