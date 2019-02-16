/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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

#include <KConfigGroup>
#include <KAboutData>
#include <KIconLoader>
#include <KLocalizedString>
#include <KActionCollection>

#include <QMimeData>
#include <QUrl>
#include <QIcon>
#include <QCursor>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTextDocument>
#include <QMenu>
#include <QStyle>

namespace {

const char* s_allActions[] = {
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

const int MARGIN = 20;
const int HEADER_BUTTON_SPACING = 10;
const int BUTTON_SPACING = 4;

} // namespace

K3b::WelcomeWidget::WelcomeWidget( MainWindow* mainWindow, QWidget* parent )
    : QWidget( parent ),
      m_mainWindow( mainWindow )
{
    m_header = new QTextDocument( this );
    m_infoText = new QTextDocument( this );

    setAcceptDrops( true );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    m_rows = m_cols = 1;

    m_buttonMore = new K3b::FlatButton( i18n("More actions..."), this );

    connect( m_buttonMore, SIGNAL(pressed()), this, SLOT(slotMoreActions()) );
    connect( k3bappcore->themeManager(), SIGNAL(themeChanged()), this, SLOT(slotThemeChanged()) );

    slotThemeChanged();
}


K3b::WelcomeWidget::~WelcomeWidget()
{
}


void K3b::WelcomeWidget::addAction( QAction* action )
{
    if( action ) {
        m_actions.append(action);
        rebuildGui();
    }
}


void K3b::WelcomeWidget::removeAction( QAction* action )
{
    if( action ) {
        m_actions.removeAll( action );
        rebuildGui();
    }
}


void K3b::WelcomeWidget::removeButton( K3b::FlatButton* b )
{
    removeAction( m_buttonMap[b] );
}


void K3b::WelcomeWidget::rebuildGui( const QList<QAction*>& actions )
{
    m_actions = actions;
    rebuildGui();
}


static void calculateButtons( int width, int numActions, int buttonWidth, int& cols, int& rows )
{
    // always try to avoid horizontal scrollbars
    int wa = width - 2*MARGIN;
    cols = qMax( 1, qMin( wa / (buttonWidth+BUTTON_SPACING), numActions ) );
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


void K3b::WelcomeWidget::rebuildGui()
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


void K3b::WelcomeWidget::repositionButtons()
{
    // calculate rows and columns
    calculateButtons( width(), m_actions.count(), m_buttonSize.width(), m_cols, m_rows );

    int availHor = width() - 2*MARGIN;
    int availVert = height() - MARGIN - HEADER_BUTTON_SPACING - ( int )m_header->size().height() - HEADER_BUTTON_SPACING;
    availVert -= ( int )m_infoText->size().height() - HEADER_BUTTON_SPACING;
    int leftMargin = MARGIN + (availHor - (m_buttonSize.width()+BUTTON_SPACING)*m_cols)/2;
    int topOffset = ( int )m_header->size().height() + MARGIN + ( availVert - (m_buttonSize.height()+BUTTON_SPACING)*m_rows - m_buttonMore->height() )/2;

    int row = 0;
    int col = 0;

    for ( int i = 0;i<m_buttons.count();i++ )
    {
        K3b::FlatButton* b = m_buttons.at( i );

        QRect rect( QPoint( leftMargin + (col*(m_buttonSize.width()+BUTTON_SPACING) + 2 ),
                            topOffset + (row*(m_buttonSize.height()+BUTTON_SPACING)) + 2 ), m_buttonSize );
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

    QRect rect( leftMargin + 2, topOffset + (row*(m_buttonSize.height()+BUTTON_SPACING)) + 2,
                m_cols*(m_buttonSize.width()+BUTTON_SPACING) - BUTTON_SPACING, m_buttonMore->height() );
    m_buttonMore->setGeometry( QStyle::visualRect( layoutDirection(), contentsRect(), rect ) );

    setMinimumHeight( heightForWidth( width() ) );
}


int K3b::WelcomeWidget::heightForWidth(int width) const
{
    int ow = (int)m_infoText->idealWidth() + 10;
    m_infoText->setTextWidth(ow);
    int h = (int)m_infoText->size().height();
    int cols, rows;
    calculateButtons(width, m_actions.count(), m_buttonSize.width(), cols, rows);
    int height = MARGIN +
                 m_header->size().toSize().height() +
                 HEADER_BUTTON_SPACING +
                 ((m_buttonSize.height() + BUTTON_SPACING) * rows) + m_buttonMore->height() +
                 HEADER_BUTTON_SPACING + h + MARGIN;
    return height;
}


bool K3b::WelcomeWidget::event( QEvent *event )
{
    if( event->type() == QEvent::StyleChange ) {
        update();
    }
    return QWidget::event( event );
}


void K3b::WelcomeWidget::resizeEvent( QResizeEvent* e )
{
    m_infoText->setTextWidth( width() - MARGIN );
    QWidget::resizeEvent(e);
    repositionButtons();
    if( e->size() != m_bgPixmap.size() )
        updateBgPix();
}


void K3b::WelcomeWidget::slotThemeChanged()
{
    if( K3b::Theme* theme = k3bappcore->themeManager()->currentTheme() ) {
//         if( theme->backgroundMode() == K3b::Theme::BG_SCALE )
//             m_bgImage = theme->pixmap( K3b::Theme::WELCOME_BG ).convertToImage();
        m_header->setDefaultStyleSheet( QString("body { font-size: 16pt; font-weight: bold; color: %1 }")
                                        .arg(theme->foregroundColor().name()) );
        m_infoText->setDefaultStyleSheet( QString("body { color: %1 }")
                                          .arg(theme->foregroundColor().name()) );
    }

    m_header->setHtml( "<html><body align=\"center\">" + i18n("Welcome to K3b &ndash; The CD, DVD, and Blu-ray Kreator") + "</body></html>" );
    m_infoText->setHtml( "<html><body align=\"center\">" 
                         + i18n("K3b %1 Copyright &copy; 1998&ndash;2018 K3b authors",
                                KAboutData::applicationData().version())
                         + "</body></html>" );
    setMinimumWidth( 2*MARGIN + qMax(( int )m_header->idealWidth(), m_buttonSize.width()) );
    updateBgPix();
    update();
}


void K3b::WelcomeWidget::slotMoreActions()
{
    QMenu popup;

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


void K3b::WelcomeWidget::updateBgPix()
{
    if( K3b::Theme* theme = k3bappcore->themeManager()->currentTheme() ) {
        if( theme->backgroundMode() == K3b::Theme::BG_SCALE )
            m_bgPixmap = theme->pixmap( K3b::Theme::WELCOME_BG ).scaled( rect().width(), rect().height() );
        else
            m_bgPixmap = theme->pixmap( K3b::Theme::WELCOME_BG );
    }
}


void K3b::WelcomeWidget::paintEvent( QPaintEvent* )
{
    if( K3b::Theme* theme = k3bappcore->themeManager()->currentTheme() ) {
        QPainter p( this );
        p.setPen( theme->foregroundColor() );

        // draw the background including first filling with the bg color for transparent images
        p.fillRect( rect(), theme->backgroundColor() );
        p.drawTiledPixmap( rect(), m_bgPixmap );

        // rect around the header
        QRect rect( 10, 10, qMax( ( int )m_header->idealWidth() + MARGIN, width() - MARGIN ), ( int )m_header->size().height() + MARGIN );
        p.fillRect( rect, theme->backgroundColor() );
        p.drawRect( rect );

        // big rect around the whole thing
        p.drawRect( 10, 10, width()-MARGIN, height()-MARGIN );

        // draw the header text
        int pos = MARGIN;
        pos += qMax( (width()-2*MARGIN-( int )m_header->idealWidth())/2, 0 );
        p.save();
        p.translate( pos, MARGIN );
        m_header->drawContents( &p );
        p.restore();

        // draw the info box
        //    int boxWidth = MARGIN + m_infoText->widthUsed();
        int boxHeight = 10 + ( int )m_infoText->size().height();
        QRect infoBoxRect( 10/*qMax( (width()-MARGIN-m_infoText->widthUsed())/2, 10 )*/,
                           height()-10-boxHeight,
                           width()-MARGIN/*boxWidth*/,
                           boxHeight );
        p.fillRect( infoBoxRect, theme->backgroundColor() );
        p.drawRect( infoBoxRect );
        p.save();
        p.translate( infoBoxRect.left()+5, infoBoxRect.top()+5 );
        m_infoText->drawContents( &p );
        p.restore();
    }
}


void K3b::WelcomeWidget::dragEnterEvent( QDragEnterEvent* event )
{
    event->setAccepted( event->mimeData()->hasUrls() );
}


void K3b::WelcomeWidget::dropEvent( QDropEvent* e )
{
    QList<QUrl> urls;
    Q_FOREACH( const QUrl& url, e->mimeData()->urls() )
    {
        urls.push_back( url );
    }

    QMetaObject::invokeMethod( m_mainWindow, "addUrls", Qt::QueuedConnection, Q_ARG( QList<QUrl>, urls ) );
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

    rebuildGui( actions );
}


void K3b::WelcomeWidget::saveConfig( KConfigGroup c )
{
    QStringList sl;
    QList<QAction *> items(m_actions);
    for( QList<QAction *>::const_iterator it = items.constBegin();
            it != items.constEnd(); ++it )
        sl.append( (*it)->objectName() );

    c.writeEntry( "welcome_actions", sl );
}


void K3b::WelcomeWidget::mousePressEvent ( QMouseEvent* e )
{
    if( e->button() == Qt::RightButton ) {
        QMap<QAction*, QAction*> map;
        QMenu addPop;
        addPop.setTitle( i18n("Add Button") );

        QAction* firstAction = 0;
        for ( int i = 0; s_allActions[i]; ++i ) {
            if ( s_allActions[i][0] != '_' ) {
                QAction* a = m_mainWindow->actionCollection()->action( s_allActions[i] );
                if ( a && !m_actions.contains(a) ) {
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

        QWidget* widgetAtPos = childAt(e->pos());
        if( widgetAtPos && widgetAtPos->inherits( "K3b::FlatButton" ) ) {
            QMenu pop;
            removeAction = pop.addAction( SmallIcon("list-remove"), i18n("Remove Button") );
            if ( addPop.actions().count() > 0 )
                pop.addMenu( &addPop );
            r = pop.exec( e->globalPos() );
        }
        else {
            addPop.insertSection( firstAction, addPop.title() );
            r = addPop.exec( e->globalPos() );
        }

        if( r != 0 ) {
            if( r == removeAction )
                removeButton( static_cast<K3b::FlatButton*>(widgetAtPos) );
            else
                addAction( map[r] );
        }
    }
}


