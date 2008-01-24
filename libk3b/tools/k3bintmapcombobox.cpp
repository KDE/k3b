/*
 *
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bintmapcombobox.h"

#include <KDebug>

#include <QtCore/QHash>
#include <QtCore/QPair>


class K3bIntMapComboBox::Private
{
public:
    QHash<int, int> valueIndexMap;
    QList<QPair<int, QString> > values;

    QString topWhatsThis;
    QString bottomWhatsThis;

    void buildValueIndexMap() {
        valueIndexMap.clear();
        for ( int i = 0; i < values.count(); ++i ) {
            valueIndexMap.insert( values[i].first, i );
        }
    }
};


K3bIntMapComboBox::K3bIntMapComboBox( QWidget* parent )
    : KComboBox( parent )
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
    if( d->values.count() > KComboBox::currentIndex() &&
        KComboBox::currentIndex() >= 0 )
        return d->values[KComboBox::currentIndex()].first;
    else
        return 0;
}


void K3bIntMapComboBox::setSelectedValue( int value )
{
    if( d->valueIndexMap.contains( value ) ) {
        KComboBox::setCurrentIndex( d->valueIndexMap[value] );
    }
}


bool K3bIntMapComboBox::hasValue( int value ) const
{
    return d->valueIndexMap.contains( value );
}


void K3bIntMapComboBox::clear()
{
    d->valueIndexMap.clear();
    d->values.clear();

    KComboBox::clear();
}


bool K3bIntMapComboBox::insertItem( int value, const QString& text, const QString& description, int index )
{
    if( d->valueIndexMap.contains( value ) )
        return false;

    if ( index < 0 || index > KComboBox::count() ) {
        index = KComboBox::count();
    }

    d->values.insert( index, qMakePair<int, QString>( value, description ) );
    d->buildValueIndexMap();

    KComboBox::insertItem( index, text );

    updateWhatsThis();

    // select a default value. This is always wanted in K3b
    if ( KComboBox::currentIndex() < 0 ) {
        setSelectedValue( d->values[0].first );
    }

    return true;
}


void K3bIntMapComboBox::updateWhatsThis()
{
    QString ws( d->topWhatsThis );
    for( int i = 0; i < d->values.count(); ++i ) {
        ws += "<p><b>" + KComboBox::text( i ) + "</b><br>";
        ws += d->values[i].second;
    }
    ws += "<p>" + d->bottomWhatsThis;

    setWhatsThis( ws );
}


void K3bIntMapComboBox::slotItemHighlighted( int index )
{
    emit valueHighlighted( d->values[index].first );
}


void K3bIntMapComboBox::slotItemActivated( int index )
{
    emit valueChanged( d->values[index].first );
}


void K3bIntMapComboBox::addGlobalWhatsThisText( const QString& top, const QString& bottom )
{
    d->topWhatsThis = top;
    d->bottomWhatsThis = bottom;
    updateWhatsThis();
}

#include "k3bintmapcombobox.moc"
