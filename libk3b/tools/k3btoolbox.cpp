/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#include "k3btoolbox.h"

#include <k3bcore.h>

#include <kaction.h>
#include <kpopupmenu.h>
#include <ktoolbarbutton.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <klocale.h>
#include <kconfig.h>

#include <qtoolbutton.h>
#include <qsizepolicy.h>
#include <qlayout.h>
#include <q3whatsthis.h>
#include <qtooltip.h>
#include <qlabel.h>
#include <q3vbox.h>
#include <qstyle.h>
#include <qpainter.h>
#include <qevent.h>
#include <qobject.h>
//Added by qt3to4:
#include <Q3ValueList>
#include <QMouseEvent>

// copied from ktoolbar.cpp
enum {
    CONTEXT_TOP = 0,
    CONTEXT_LEFT = 1,
    CONTEXT_RIGHT = 2,
    CONTEXT_BOTTOM = 3,
    CONTEXT_FLOAT = 4,
    CONTEXT_FLAT = 5,
    CONTEXT_ICONS = 6,
    CONTEXT_TEXT = 7,
    CONTEXT_TEXTRIGHT = 8,
    CONTEXT_TEXTUNDER = 9,
    CONTEXT_ICONSIZES = 50 // starting point for the icon size list, put everything else before
};


// sadly KToolBar is not designed to be used without a MainWindow. As a result we have to reimplement
// a lot of things such as the context menu or the loading and saving of settings


class K3bToolBox::Private
{
public:
    Private( K3bToolBox* p )
        : m_parent( p ),
          m_contextMenu( 0 ) {
    }

    KPopupMenu* contextMenu();

    Q3ValueList<int> iconSizes;

private:
    K3bToolBox* m_parent;
    KPopupMenu* m_contextMenu;
};

static int s_toolboxCnt = 0;

K3bToolBox::K3bToolBox( QWidget* parent, const char* name )
  : KToolBar( parent, name == 0 ? QString( "K3bToolBox%1" ).arg( ++s_toolboxCnt ).latin1() : name )
{
    d = new Private( this );
    setMovingEnabled(false);
    setFlat(true);
    setIconSize( 16 );
    setEnableContextMenu( false );

    loadSettings();
}


K3bToolBox::~K3bToolBox()
{
    saveSettings();
    delete d;
}


KToolBarButton* K3bToolBox::addButton( KAction* action, bool forceText )
{
  if( action ) {
      action->plug( this );
      if ( forceText ) {
          if ( KToolBarButton* button = getButton( idAt( count()-1 ) ) ) {
              button->setUsesTextLabel( true );
          }
      }
      return getButton( idAt( count()-1 ) );
  }
  return 0;
}


KToolBarButton* K3bToolBox::addButton( const QString& text, const QString& icon,
                                       const QString& tooltip, const QString& whatsthis,
                                       QObject* receiver, const char* slot,
                                       bool forceText )
{
    KToolBarButton* button = getButton( idAt( insertButton( icon, -1, 0, receiver, slot, true, text ) ) );
    Q3WhatsThis::add( button, whatsthis );
    QToolTip::add( button, tooltip );
    if ( forceText ) {
        button->setUsesTextLabel( true );
    }
    return button;
}


void K3bToolBox::addSpacing()
{
    insertSeparator();
}


void K3bToolBox::addSeparator()
{
    insertLineSeparator();
}


void K3bToolBox::addStretch()
{
    QWidget* w = new QWidget( this );
    addWidget( w );
    setStretchableWidget( w );
}


void K3bToolBox::addLabel( const QString& text )
{
    addWidget( new QLabel( text, this ) );
}


void K3bToolBox::addWidget( QWidget* w )
{
    w->reparent( this, QPoint() );
    insertWidget( -1, w->sizeHint().width(), w );
}


KToolBarButton* K3bToolBox::addToggleButton( KToggleAction* action )
{
    return addButton( action );
}


void K3bToolBox::addWidgetAction( KWidgetAction* action )
{
  addWidget( action->widget() );
}


void K3bToolBox::clear()
{
}


// copied mostly from ktoolbar.cpp
KPopupMenu* K3bToolBox::Private::contextMenu()
{
  if ( m_contextMenu )
    return m_contextMenu;

  // Construct our contextMenu popup menu. Name it qt_dockwidget_internal so it
  // won't be deleted by QToolBar::clear().
  m_contextMenu = new KPopupMenu( m_parent, "qt_dockwidget_internal" );
  m_contextMenu->insertTitle(i18n("Toolbox Menu"));

  KPopupMenu *mode = new KPopupMenu( m_contextMenu, "mode" );
  mode->insertItem( i18n("Icons Only"), CONTEXT_ICONS );
  mode->insertItem( i18n("Text Only"), CONTEXT_TEXT );
  mode->insertItem( i18n("Text Alongside Icons"), CONTEXT_TEXTRIGHT );
  mode->insertItem( i18n("Text Under Icons"), CONTEXT_TEXTUNDER );

  KPopupMenu *size = new KPopupMenu( m_contextMenu, "size" );
  size->insertItem( i18n("Default"), CONTEXT_ICONSIZES );
  // Query the current theme for available sizes
  KIconTheme *theme = KGlobal::instance()->iconLoader()->theme();
  Q3ValueList<int> avSizes;
  if (theme) {
      avSizes = theme->querySizes( KIcon::Toolbar);
  }

  iconSizes = avSizes;
  qSort(avSizes);

  Q3ValueList<int>::Iterator it;
  if (avSizes.count() < 10) {
      // Fixed or threshold type icons
	  Q3ValueList<int>::Iterator end(avSizes.end());
      for (it=avSizes.begin(); it!=end; ++it) {
          QString text;
          if ( *it < 19 )
              text = i18n("Small (%1x%2)").arg(*it).arg(*it);
          else if (*it < 25)
              text = i18n("Medium (%1x%2)").arg(*it).arg(*it);
          else if (*it < 35)
              text = i18n("Large (%1x%2)").arg(*it).arg(*it);
          else
              text = i18n("Huge (%1x%2)").arg(*it).arg(*it);
          //we use the size as an id, with an offset
          size->insertItem( text, CONTEXT_ICONSIZES + *it );
      }
  }
  else {
      // Scalable icons.
      const int progression[] = {16, 22, 32, 48, 64, 96, 128, 192, 256};

      it = avSizes.begin();
      for (uint i = 0; i < 9; i++) {
          while (it++ != avSizes.end()) {
              if (*it >= progression[i]) {
                  QString text;
                  if ( *it < 19 )
                      text = i18n("Small (%1x%2)").arg(*it).arg(*it);
                  else if (*it < 25)
                      text = i18n("Medium (%1x%2)").arg(*it).arg(*it);
                  else if (*it < 35)
                      text = i18n("Large (%1x%2)").arg(*it).arg(*it);
                  else
                      text = i18n("Huge (%1x%2)").arg(*it).arg(*it);
                  //we use the size as an id, with an offset
                  size->insertItem( text, CONTEXT_ICONSIZES + *it );
                  break;
              }
          }
      }
  }

  m_contextMenu->insertItem( i18n("Text Position"), mode );
  m_contextMenu->setItemChecked(CONTEXT_ICONS, true);
  m_contextMenu->insertItem( i18n("Icon Size"), size );

  connect( m_contextMenu, SIGNAL( aboutToShow() ), m_parent, SLOT( slotContextAboutToShow() ) );
  return m_contextMenu;
}


void K3bToolBox::slotContextAboutToShow()
{
    for(int i = CONTEXT_ICONS; i <= CONTEXT_TEXTUNDER; ++i)
        d->contextMenu()->setItemChecked(i, false);

    switch( iconText() )
    {
    case IconOnly:
    default:
        d->contextMenu()->setItemChecked(CONTEXT_ICONS, true);
        break;
    case IconTextRight:
        d->contextMenu()->setItemChecked(CONTEXT_TEXTRIGHT, true);
        break;
    case TextOnly:
        d->contextMenu()->setItemChecked(CONTEXT_TEXT, true);
        break;
    case IconTextBottom:
        d->contextMenu()->setItemChecked(CONTEXT_TEXTUNDER, true);
        break;
    }

    Q3ValueList<int>::ConstIterator iIt = d->iconSizes.begin();
    Q3ValueList<int>::ConstIterator iEnd = d->iconSizes.end();
    for (; iIt != iEnd; ++iIt )
        d->contextMenu()->setItemChecked( CONTEXT_ICONSIZES + *iIt, false );

    d->contextMenu()->setItemChecked( CONTEXT_ICONSIZES, false );

    d->contextMenu()->setItemChecked( CONTEXT_ICONSIZES + iconSize(), true );
}


void K3bToolBox::mousePressEvent ( QMouseEvent* m )
{
    if ( m->button() == RightButton ) {
        int i = d->contextMenu()->exec( m->globalPos(), 0 );
        switch ( i ) {
        case -1:
            return; // popup canceled
        case CONTEXT_ICONS:
            setIconText( IconOnly );
            break;
        case CONTEXT_TEXTRIGHT:
            setIconText( IconTextRight );
            break;
        case CONTEXT_TEXT:
            setIconText( TextOnly );
            break;
        case CONTEXT_TEXTUNDER:
            setIconText( IconTextBottom );
            break;
        default:
            if ( i >= CONTEXT_ICONSIZES )
                setIconSize( i - CONTEXT_ICONSIZES );
            else
                return;
        }
    }
}


void K3bToolBox::loadSettings()
{
    KConfigGroup config( k3bcore->config(), settingsGroup() );

    setIconSize( config.readNumEntry( "IconSize", 16 ) );
    QString iconText = config.readEntry( "IconText", "IconOnly" );
    IconText icon_text;
    if ( iconText == "IconTextRight" )
        icon_text = IconTextRight;
    else if ( iconText == "IconTextBottom" )
        icon_text = IconTextBottom;
    else if ( iconText == "TextOnly" )
        icon_text = TextOnly;
    else
        icon_text = IconOnly;
    setIconText(icon_text);
}


void K3bToolBox::saveSettings()
{
    KConfigGroup config( k3bcore->config(), settingsGroup() );

    QString icontext;
    switch ( iconText() ) {
    case KToolBar::IconTextRight:
        icontext = "IconTextRight";
        break;
    case KToolBar::IconTextBottom:
        icontext = "IconTextBottom";
        break;
    case KToolBar::TextOnly:
        icontext = "TextOnly";
        break;
    case KToolBar::IconOnly:
    default:
        icontext = "IconOnly";
        break;
    }

    config.writeEntry("IconText", icontext);
    config.writeEntry("IconSize", iconSize());
}

#include "k3btoolbox.moc"
