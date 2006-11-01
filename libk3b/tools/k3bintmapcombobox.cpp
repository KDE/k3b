/* 
 *
 * $Id: k3bwritingmodewidget.cpp 554512 2006-06-24 07:25:39Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bintmapcombobox.h"

#include <qwhatsthis.h>
#include <qmap.h>
#include <qvaluevector.h>


class K3bIntMapComboBox::Private
{
public:
  QMap<int, int> valueIndexMap;
  QValueVector<QPair<int, QString> > indexValueDescriptionMap;
};


K3bIntMapComboBox::K3bIntMapComboBox( QWidget* parent, const char* name )
  : KComboBox( parent, name )
{
  d = new Private;
  connect( this, SIGNAL(highlighted(int)),
	   this, SLOT(slotItemHighlighted(int)) );
  connect( this, SIGNAL(activated(int)),
	   this, SLOT(slotItemActivated(int)) );
}


K3bIntMapComboBox::~K3bIntMapComboBox()
{
  delete d;
}


int K3bIntMapComboBox::selectedValue() const
{
  return d->indexValueDescriptionMap[KComboBox::currentItem()].first;
}


void K3bIntMapComboBox::setSelectedValue( int value )
{
  KComboBox::setCurrentItem( d->valueIndexMap[value] );
}


void K3bIntMapComboBox::clear()
{
  d->valueIndexMap.clear();
  d->indexValueDescriptionMap.clear();

  KComboBox::clear();
}


bool K3bIntMapComboBox::insertItem( int value, const QString& text, const QString& description, int index )
{
  if( d->valueIndexMap.contains( value ) )
    return false;

  d->valueIndexMap[value] = KComboBox::count();
  d->indexValueDescriptionMap.append( qMakePair<int, QString>( value, description ) );

  KComboBox::insertItem( text, index );

  updateWhatsThis();

  return true;
}


void K3bIntMapComboBox::updateWhatsThis()
{
  QString ws;
  for( unsigned int i = 0; i < d->indexValueDescriptionMap.count(); ++i ) {
    ws += "<p><b>" + KComboBox::text( i ) + "</b><br>";
    ws += d->indexValueDescriptionMap[i].second;
  }

  QWhatsThis::add( this, ws );
}


void K3bIntMapComboBox::slotItemHighlighted( int index )
{
  emit valueHighlighted( d->indexValueDescriptionMap[index].first );
}


void K3bIntMapComboBox::slotItemActivated( int index )
{
  emit valueChanged( d->indexValueDescriptionMap[index].first );
}

#include "k3bintmapcombobox.moc"
