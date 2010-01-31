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
#include <QStyle>
#include <QTextDocument>

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
    "file_new_mixed",
    "_sep_",
    "file_new_vcd",
    "file_new_video_dvd",
    "_sep_",
    "file_new_movix",
    "_sep_",
    "tools_copy_medium",
    "tools_format_medium",
    "tools_write_image",
    "_sep_",
    "tools_cdda_rip",
    "tools_videocd_rip",
    "tools_videodvd_rip",
    0
};

K3b::WelcomeWidget::Display::Display( K3b::WelcomeWidget* parent )
    : QWidget( parent->viewport() )
{
    m_header = new QTextDocument( this );
    m_infoText = new QTextDocument( this );

    setAcceptDrops( true );
    m_rows = m_cols = 1;

    m_buttonMore = new K3b::FlatButton( i18n("More actions..."), this );
    connect( m_buttonMore, SIGNAL(pressed()), parent, SLOT(slotMoreActions()) );

    connect( k3bappcore->themeManager(), SIGNAL(themeChanged()), this, SLOT(slotThemeChanged()) );

    slotThemeChanged();
}


K3b::WelcomeWidget::Display::~Display()
{
}


void K3b::WelcomeWidget::Display::addAction( QAction* action )
{
    if( action ) {
        m_actions.append(action);
        rebuildGui();
    }
}


void K3b::WelcomeWidget::Display::removeAction( QAction* action )
{
    if( action ) {
        m_actions.removeAll( action );
        rebuildGui();
    }
}


void K3b::WelcomeWidget::Display::removeButton( K3b::FlatButton* b )
{
    removeAction( m_buttonMap[b] );
}


void K3b::WelcomeWidget::Display::rebuildGui( const QList<QAction*>& actions )
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


void K3b::WelcomeWidget::Display::rebuildGui()
{
    // step 1: delete all old buttons in the buttons QPtrList<K3b::FlatButton>
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
            K3b::FlatButton* b = new K3b::FlatButton( a, this );

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


void K3b::WelcomeWidget::Display::repositionButtons()
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
        K3b::FlatButton* b = m_buttons.at( i );

        QRect rect( QPoint( leftMargin + (col*(m_buttonSize.width()+4) + 2 ),
                            topOffset + (row*(m_buttonSize.height()+4)) + 2 ), m_buttonSize );
        b->setGeometry( QStyle::visualRect( layoutDirection(), contentsRect(), rect ) );
        b->show();

        col++;
        if( col == m_cols ) {
            col = 0;
            row++;
        }
    }
    if( col > 0 )
        ++row;
    
    QRect rect( leftMargin + 2, topOffset + (row*(m_buttonSize.height()+4)) + 2,
                m_cols*(m_buttonSize.width()+4) - 4, m_buttonMore->height() );
    m_buttonMore->setGeometry( QStyle::visualRect( layoutDirection(), contentsRect(), rect ) );
}


QSizePolicy K3b::WelcomeWidget::Display::sizePolicy () const
{
    return QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
}


int K3b::WelcomeWidget::Display::heightForWidth( int w ) const
{
    int ow = ( int )m_infoText->idealWidth();
    m_infoText->setTextWidth( w );
    int h = ( int )m_infoText->size().height();
    m_infoText->setTextWidth( ow );

    int cols, rows;
    calculateButtons( w, m_actions.count(), m_buttonSize.width(), cols, rows );

    return (20 + ( int )m_header->size().height() + 20 + 10 + ((m_buttonSize.height()+4)*rows) + 4 + m_buttonMore->height() + 10 + h + 20);
}


QSize K3b::WelcomeWidget::Display::minimumSizeHint() const
{
    QSize size( qMax(40+( int )m_header->idealWidth(), 40+m_buttonSize.width()),
                20 + ( int )m_header->size().height() + 20 + 10 + m_buttonSize.height() + 10 + ( int )m_infoText->size().height() + 20 );

    return size;
}


void K3b::WelcomeWidget::Display::resizeEvent( QResizeEvent* e )
{
    m_infoText->setTextWidth( width() - 20 );
    QWidget::resizeEvent(e);
    repositionButtons();
    if( e->size() != m_bgPixmap.size() )
        updateBgPix();
}


void K3b::WelcomeWidget::Display::slotThemeChanged()
{
    if( K3b::Theme* theme = k3bappcore->themeManager()->currentTheme() ) {
//         if( theme->backgroundMode() == K3b::Theme::BG_SCALE )
//             m_bgImage = theme->pixmap( K3b::Theme::WELCOME_BG ).convertToImage();
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


void K3b::WelcomeWidget::Display::updateBgPix()
{
    if( K3b::Theme* theme = k3bappcore->themeManager()->currentTheme() ) {
        if( theme->backgroundMode() == K3b::Theme::BG_SCALE )
            m_bgPixmap = theme->pixmap( K3b::Theme::WELCOME_BG ).scaled( rect().width(), rect().height() );
        else
            m_bgPixmap = theme->pixmap( K3b::Theme::WELCOME_BG );
    }
}


void K3b::WelcomeWidget::Display::paintEvent( QPaintEvent* )
{
    if( K3b::Theme* theme = k3bappcore->themeManager()->currentTheme() ) {
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


void K3b::WelcomeWidget::Display::dragEnterEvent( QDragEnterEvent* event )
{
    event->setAccepted( event->mimeData()->hasUrls() );
}


void K3b::WelcomeWidget::Display::dropEvent( QDropEvent* e )
{
    KUrl::List urls;
    Q_FOREACH( const QUrl& url, e->mimeData()->urls() )
    {
        urls.push_back( url );
    }
    
    emit dropped( urls );
}



K3b::WelcomeWidget::WelcomeWidget( K3b::MainWindow* mw, QWidget* parent )
    : QScrollArea( parent ),
      m_mainWindow( mw )
{
    main = new Display( this );
    setWidget( main );
    setFrameStyle( QFrame::NoFrame );

    connect( main, SIGNAL(dropped(const KUrl::List&)), m_mainWindow, SLOT(addUrls(const KUrl::List&)), Qt::QueuedConnection );

    connect( KGlobalSettings::self(), SIGNAL(appearanceChanged()), main, SLOT(update()) );
}


K3b::WelcomeWidget::~WelcomeWidget()
{
}


void K3b::WelcomeWidget::loadConfig( const KConfigGroup& c )
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


void K3b::WelcomeWidget::saveConfig( KConfigGroup c )
{
    QStringList sl;
    QList<QAction *> items(main->m_actions);
    for( QList<QAction *>::const_iterator it = items.constBegin();
            it != items.constEnd(); ++it )
        sl.append( (*it)->objectName() );

    c.writeEntry( "welcome_actions", sl );
}


void K3b::WelcomeWidget::resizeEvent( QResizeEvent* e )
{
    fixSize();
    QScrollArea::resizeEvent( e );
}


void K3b::WelcomeWidget::showEvent( QShowEvent* e )
{
    fixSize();
    QScrollArea::showEvent( e );
}


void K3b::WelcomeWidget::fixSize()
{
    QSize s = contentsRect().size();
    s.setWidth( qMax( main->minimumSizeHint().width(), s.width() ) );
    s.setHeight( qMax( main->heightForWidth(s.width()), s.height() ) );

    main->resize( s );
    viewport()->resize( s );
}


void K3b::WelcomeWidget::mousePressEvent ( QMouseEvent* e )
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
        if( widgetAtPos && widgetAtPos->inherits( "K3b::FlatButton" ) ) {
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
                main->removeButton( static_cast<K3b::FlatButton*>(widgetAtPos) );
            else
                main->addAction( map[r] );
        }

        fixSize();
    }
}


void K3b::WelcomeWidget::slotMoreActions()
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
