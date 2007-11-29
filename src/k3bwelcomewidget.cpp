/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bwelcomewidget.h"
#include "k3b.h"
#include "k3bflatbutton.h"
#include <k3bstdguiitems.h>
#include "k3bapplication.h"
#include <k3bversion.h>
#include "k3bthememanager.h"

#include <qpixmap.h>
#include <qtoolbutton.h>
#include <qlabel.h>
#include <qpainter.h>
#include <q3simplerichtext.h>
#include <q3ptrlist.h>
#include <qmap.h>
#include <qtooltip.h>
#include <qcursor.h>
#include <qimage.h>
//Added by qt3to4:
#include <QDropEvent>
#include <QShowEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>

#include <kurl.h>
#include <k3urldrag.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kmenu.h>
#include <kaboutdata.h>

#include <KActionMenu>
#include <KToggleAction>
#include <KActionCollection>

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
    QFont fnt(font());
    fnt.setBold(true);
    fnt.setPointSize( 16 );
    m_header = new Q3SimpleRichText( i18n("Welcome to K3b - The CD and DVD Kreator"), fnt );
    m_infoText = new Q3SimpleRichText( QString::fromUtf8("<qt align=\"center\">K3b %1 (c) 1999 - 2007 Sebastian Trüg")
                                       .arg(KGlobal::mainComponent().aboutData()->version()), font() );

    // set a large width just to be sure no linebreak occurs
    m_header->setWidth( 800 );

    setAcceptDrops( true );
    m_rows = m_cols = 1;

    m_buttonMore = new K3bFlatButton( i18n("Further actions..."), this );
    connect( m_buttonMore, SIGNAL(pressed()), parent, SLOT(slotMoreActions()) );

    connect( k3bappcore->themeManager(), SIGNAL(themeChanged()), this, SLOT(slotThemeChanged()) );

    slotThemeChanged();
}


K3bWelcomeWidget::Display::~Display()
{
    delete m_header;
    delete m_infoText;
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
    m_buttons.setAutoDelete(true);
    m_buttons.clear();

    int numActions = m_actions.count();
    if( numActions > 0 ) {

        // create buttons
        QList<QAction *> items(m_actions);
        for( QList<QAction *>::const_iterator it = items.begin();
            it != items.end(); ++it ) {
            QAction* a = *it;

            K3bFlatButton* b = new K3bFlatButton( a, this );

            m_buttons.append( b );
            m_buttonMap.insert( b, a );
        }

        // determine the needed button size (since all buttons should be equal in size
        // we use the max of all sizes)
        m_buttonSize = m_buttons.first()->sizeHint();
        for( Q3PtrListIterator<K3bFlatButton> it( m_buttons ); it.current(); ++it ) {
            m_buttonSize = m_buttonSize.expandedTo( it.current()->sizeHint() );
        }

        repositionButtons();
    }
}


void K3bWelcomeWidget::Display::repositionButtons()
{
    // calculate rows and columns
    calculateButtons( width(), m_actions.count(), m_buttonSize.width(), m_cols, m_rows );

    int availHor = width() - 40;
    int availVert = height() - 20 - 10 - m_header->height() - 10;
    availVert -= m_infoText->height() - 10;
    int leftMargin = 20 + (availHor - (m_buttonSize.width()+4)*m_cols)/2;
    int topOffset = m_header->height() + 20 + ( availVert - (m_buttonSize.height()+4)*m_rows - m_buttonMore->height() )/2;

    int row = 0;
    int col = 0;

    for( Q3PtrListIterator<K3bFlatButton> it( m_buttons ); it.current(); ++it ) {
        K3bFlatButton* b = it.current();

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
    return QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum, true );
}


int K3bWelcomeWidget::Display::heightForWidth( int w ) const
{
    int ow = m_infoText->width();
    m_infoText->setWidth( w );
    int h = m_infoText->height();
    m_infoText->setWidth( ow );

    int cols, rows;
    calculateButtons( w, m_actions.count(), m_buttonSize.width(), cols, rows );

    return (20 + m_header->height() + 20 + 10 + ((m_buttonSize.height()+4)*rows) + 4 + m_buttonMore->height() + 10 + h + 20);
}


QSize K3bWelcomeWidget::Display::minimumSizeHint() const
{
    QSize size( qMax(40+m_header->widthUsed(), 40+m_buttonSize.width()),
                20 + m_header->height() + 20 + 10 + m_buttonSize.height() + 10 + m_infoText->height() + 20 );

    return size;
}


void K3bWelcomeWidget::Display::resizeEvent( QResizeEvent* e )
{
    m_infoText->setWidth( width() - 20 );
    QWidget::resizeEvent(e);
    repositionButtons();
    if( e->size() != m_bgPixmap.size() )
        updateBgPix();
}


void K3bWelcomeWidget::Display::slotThemeChanged()
{
//   if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() )
//     if( theme->backgroundMode() == K3bTheme::BG_SCALE )
//       m_bgImage = theme->pixmap( K3bTheme::WELCOME_BG ).convertToImage();

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
        QRect rect( 10, 10, qMax( m_header->widthUsed() + 20, width() - 20 ), m_header->height() + 20 );
        p.fillRect( rect, theme->backgroundColor() );
        p.drawRect( rect );

        // big rect around the whole thing
        p.drawRect( 10, 10, width()-20, height()-20 );

        // draw the header text
        QColorGroup grp( colorGroup() );
        grp.setColor( QColorGroup::Text, theme->foregroundColor() );
        int pos = 20;
        pos += qMax( (width()-40-m_header->widthUsed())/2, 0 );
        m_header->draw( &p, pos, 20, QRect(), grp );

        // draw the info box
        //    int boxWidth = 20 + m_infoText->widthUsed();
        int boxHeight = 10 + m_infoText->height();
        QRect infoBoxRect( 10/*qMax( (width()-20-m_infoText->widthUsed())/2, 10 )*/,
                           height()-10-boxHeight,
                           width()-20/*boxWidth*/,
                           boxHeight );
        p.fillRect( infoBoxRect, theme->backgroundColor() );
        p.drawRect( infoBoxRect );
        m_infoText->draw( &p, infoBoxRect.left()+5, infoBoxRect.top()+5, QRect(), grp );
    }
}


void K3bWelcomeWidget::Display::dragEnterEvent( QDragEnterEvent* event )
{
    event->accept( K3URLDrag::canDecode(event) );
}


void K3bWelcomeWidget::Display::dropEvent( QDropEvent* e )
{
    KUrl::List urls;
    K3URLDrag::decode( e, urls );
    emit dropped( urls );
}



K3bWelcomeWidget::K3bWelcomeWidget( K3bMainWindow* mw, QWidget* parent )
    : Q3ScrollView( parent ),
      m_mainWindow( mw )
{
    main = new Display( this );
    addChild( main );

    connect( main, SIGNAL(dropped(const KUrl::List&)), m_mainWindow, SLOT(addUrls(const KUrl::List&)) );

    connect( kapp, SIGNAL(appearanceChanged()), main, SLOT(update()) );
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
    for( QStringList::const_iterator it = sl.begin(); it != sl.end(); ++it )
        if( QAction* a = m_mainWindow->actionCollection()->action( (*it).latin1() ) )
            actions.append(a);

    main->rebuildGui( actions );

    fixSize();
}


void K3bWelcomeWidget::saveConfig( KConfigGroup& c )
{
    QStringList sl;
    QList<QAction *> items(main->m_actions);
    for( QList<QAction *>::const_iterator it = items.begin();
            it != items.end(); ++it )
        sl.append( (*it)->name() );

    c.writeEntry( "welcome_actions", sl );
}


void K3bWelcomeWidget::resizeEvent( QResizeEvent* e )
{
    Q3ScrollView::resizeEvent( e );
    fixSize();
}


void K3bWelcomeWidget::showEvent( QShowEvent* e )
{
    Q3ScrollView::showEvent( e );
    fixSize();
}


void K3bWelcomeWidget::fixSize()
{
    QSize s = contentsRect().size();
    s.setWidth( qMax( main->minimumSizeHint().width(), s.width() ) );
    s.setHeight( qMax( main->heightForWidth(s.width()), s.height() ) );

    main->resize( s );
    viewport()->resize( s );
}


void K3bWelcomeWidget::contentsMousePressEvent( QMouseEvent* e )
{
    if( e->button() == Qt::RightButton ) {
        QMap<int, QAction*> map;
        KMenu addPop;

        for ( int i = 0; s_allActions[i]; ++i ) {
            if ( s_allActions[i][0] != '_' ) {
                QAction* a = m_mainWindow->actionCollection()->action( s_allActions[i] );
                if ( a && main->m_actions.count(a)==0 ) {
                    map.insert( addPop.insertItem( a->iconSet(), a->text() ), a );
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
            if ( addPop.count() > 0 )
                pop.insertItem( i18n("Add Button"), &addPop );
            pop.insertSeparator();
            r = pop.exec( e->globalPos() );
        }
        else {
            addPop.addTitle( i18n("Add Button"));
            addPop.insertSeparator();
            r = addPop.exec( e->globalPos() );
        }

        if( r != 0 ) {
            if( r == removeAction )
                main->removeButton( static_cast<K3bFlatButton*>(widgetAtPos) );
            else
                main->addAction( /*map[r]*/r );
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
