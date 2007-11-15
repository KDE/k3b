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

#include "k3bcutcombobox.h"

#include <k3bstringutils.h>

#include <qfontmetrics.h>
#include <qevent.h>
#include <qstringlist.h>
#include <qrect.h>
#include <qsize.h>
#include <qpixmap.h>
#include <qstyle.h>
#include <qsizepolicy.h>
//Added by qt3to4:
#include <Q3StrList>
#include <QResizeEvent>


class K3bCutComboBox::Private
{
public:
  Private() {
    method = CUT;
  }

  QStringList originalItems;

  int method;
  int width;
};


#ifdef __GNUC__
#warning Use user data to store the full strings or a custom item delegate to paint the items
#endif
K3bCutComboBox::K3bCutComboBox( QWidget* parent )
  : KComboBox( parent )
{
  d = new Private();
  //  setSizePolicy( QSizePolicy::Maximum, sizePolicy().horData(), sizePolicy().hasHeightForWidth() );
}


K3bCutComboBox::K3bCutComboBox( int method, QWidget* parent )
  : KComboBox( parent )
{
  d = new Private();
  d->method = method;
}


K3bCutComboBox::~K3bCutComboBox()
{
  delete d;
}


void K3bCutComboBox::setMethod( int m )
{
  d->method = m;
  cutText();
}


QSize K3bCutComboBox::sizeHint() const
{
//   QSize s(KComboBox::sizeHint());

//   for( int i = 0; i < count(); i++ ) {
//     int w = fontMetrics().width(d->originalItems[i]) +
//       ( d->pixmaps[i].isNull() ? 0 : d->pixmaps[i].width() + 4);
//     if( w > s.width() )
//       s.setWidth( w );
//   }

  return KComboBox::sizeHint();
}

QSize K3bCutComboBox::minimumSizeHint() const
{
  return KComboBox::minimumSizeHint();
}


void K3bCutComboBox::setCurrentText( const QString& s )
{
  int i;
  for( i = 0; i < count(); i++ )
    if ( d->originalItems[i] == s )
      break;
  if ( i < count() ) {
    setCurrentIndex(i);
  }
  else if( !d->originalItems.isEmpty() ) {
    d->originalItems[currentIndex()] = s;
    cutText();
  }
}


void K3bCutComboBox::insertStringList( const QStringList&, int )
{
  // FIXME
}


void K3bCutComboBox::insertStrList( const Q3StrList&, int )
{
  // FIXME
}

void K3bCutComboBox::insertStrList( const Q3StrList*, int )
{
  // FIXME
}

void K3bCutComboBox::insertStrList( const char**, int, int)
{
  // FIXME
}

void K3bCutComboBox::insertItem( const QString& text, int index )
{
  insertItem( index, QPixmap(), text );
}

void K3bCutComboBox::insertItem( const QPixmap& pix, int i )
{
  insertItem( i, pix, "" );
}

void K3bCutComboBox::insertItem( const QPixmap& pixmap, const QString& text, int index )
{
  if( index != -1 )
    d->originalItems.insert( d->originalItems.at(index), text );
  else
    d->originalItems.append( text );

  if( !pixmap.isNull() )
    KComboBox::insertItem( index, pixmap, "xx" );
  else
    KComboBox::insertItem( index, "xx" );

  cutText();
}

void K3bCutComboBox::removeItem( int i )
{
  d->originalItems.erase( d->originalItems.at(i) );
  KComboBox::removeItem( i );
}

void K3bCutComboBox::changeItem( const QString& s, int i )
{
  d->originalItems[i] = s;
  cutText();
}

void K3bCutComboBox::changeItem( const QPixmap& pix, const QString& s, int i )
{
  KComboBox::changeItem( pix, i );
  changeItem( s, i );
}


QString K3bCutComboBox::text( int i ) const
{
  if( i < (int)d->originalItems.count() )
    return d->originalItems[i];
  else
    return QString::null;
}


QString K3bCutComboBox::currentText() const
{
  if( currentItem() < (int)d->originalItems.count() )
    return d->originalItems[currentItem()];
  else
    return QString::null;
}


void K3bCutComboBox::clear()
{
  KComboBox::clear();
  d->originalItems.clear();
}

void K3bCutComboBox::resizeEvent( QResizeEvent* e )
{
  cutText();

  KComboBox::resizeEvent(e);
}


void K3bCutComboBox::cutText()
{
  d->width = QStyle::visualRect( style().querySubControlMetrics(QStyle::CC_ComboBox, this,
								QStyle::SC_ComboBoxEditField), this ).width();

  for( int i = 0; i < (int)d->originalItems.count(); ++i ) {
    int w = d->width;
    if ( pixmap(i) && !pixmap(i)->isNull() )
      w -= ( pixmap(i)->width() + 4 );

    QString text;
    if( d->method == SQUEEZE )
      text = K3b::squeezeTextToWidth( fontMetrics(), d->originalItems[i], w );
    else
      text = K3b::cutToWidth( fontMetrics(), d->originalItems[i], w );

    // now insert the cut text
    if( pixmap(i) )
      KComboBox::changeItem( *pixmap(i), text, i );
    else
      KComboBox::changeItem( text, i );
  }
}

#include "k3bcutcombobox.moc"
