/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bcutcombobox.h"

#include <tools/k3bstringutils.h>

#include <qfontmetrics.h>
#include <qevent.h>
#include <qvaluevector.h>
#include <qrect.h>
#include <qsize.h>
#include <qpixmap.h>
#include <qstyle.h>
#include <qsizepolicy.h>


class K3bCutComboBox::Private
{
public:
  Private() {
    method = CUT;
  }

  QValueVector<QString> originalItems;
  QValueVector<QPixmap> pixmaps;

  int method;
};


K3bCutComboBox::K3bCutComboBox( QWidget* parent, const char* name )
  : KComboBox( parent, name )
{
  d = new Private();
  //  setSizePolicy( QSizePolicy::Maximum, sizePolicy().horData(), sizePolicy().hasHeightForWidth() );
}


K3bCutComboBox::K3bCutComboBox( int method, QWidget* parent, const char* name )
  : KComboBox( parent, name )
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
    setCurrentItem(i);
  }
  else if( !d->originalItems.isEmpty() ) {
    d->originalItems[currentItem()] = s;
    cutText();
  }
}


void K3bCutComboBox::insertStringList( const QStringList&, int )
{
  // FIXME
}


void K3bCutComboBox::insertStrList( const QStrList&, int )
{
  // FIXME
}

void K3bCutComboBox::insertStrList( const QStrList*, int )
{
  // FIXME
}

void K3bCutComboBox::insertStrList( const char**, int, int)
{
  // FIXME
}

void K3bCutComboBox::insertItem( const QString& text, int index )
{
  insertItem( QPixmap(), text, index );
}

void K3bCutComboBox::insertItem( const QPixmap&, int )
{
  // FIXME
}

void K3bCutComboBox::insertItem( const QPixmap& pixmap, const QString& text, int index )
{
  if( index == -1 ) {
    d->originalItems.append( text );
    d->pixmaps.append( pixmap );
  }

  cutText();
}

void K3bCutComboBox::removeItem( int )
{
  // FIXME
}

void K3bCutComboBox::changeItem( const QString&, int )
{
  // FIXME
}

void K3bCutComboBox::changeItem( const QPixmap&, const QString&, int )
{
  // FIXME
}


QString K3bCutComboBox::text( int i ) const
{
  return d->originalItems.at(i);
}


QString K3bCutComboBox::currentText() const
{
  return d->originalItems.at(currentItem());
}


void K3bCutComboBox::clear()
{
  KComboBox::clear();
  d->originalItems.clear();
  d->pixmaps.clear();
}

void K3bCutComboBox::resizeEvent( QResizeEvent* e )
{
  cutText();

  KComboBox::resizeEvent(e);
}


void K3bCutComboBox::cutText()
{
  KComboBox::clear();

  // the following size code is from QComboBox's updateLinedGeometry  
  QRect r = QStyle::visualRect( style().querySubControlMetrics(QStyle::CC_ComboBox, this,
							       QStyle::SC_ComboBoxEditField), this );


  for( int i = 0; i < (int)d->originalItems.count(); ++i ) {
    int size = r.width();
    if ( !d->pixmaps[i].isNull() )
      size -= ( d->pixmaps[i].width() + 4 );

    QString text;
    if( d->method == SQUEEZE )
      text = K3b::squeezeTextToWidth( fontMetrics(), d->originalItems[i], size );
    else
      text = K3b::cutToWidth( fontMetrics(), d->originalItems[i], size );

    // now insert the cut text
    if( d->pixmaps[i].isNull() )
      KComboBox::insertItem( text );
    else
      KComboBox::insertItem( d->pixmaps[i], text );
  }
}

#include "k3bcutcombobox.moc"
