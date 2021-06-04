/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bintmapcombobox.h"

#include <QDebug>
#include <QHash>
#include <QPair>


class K3b::IntMapComboBox::Private
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

    bool haveCustomWhatsThis() const {
        for ( int i = 0; i < values.count(); ++i ) {
            if ( !values[i].second.isEmpty() ) {
                return true;
            }
        }
        return false;
    }

    void updateWhatsThis() {
        if ( haveCustomWhatsThis() ) {
            QString ws( topWhatsThis );
            for( int i = 0; i < values.count(); ++i ) {
                ws += "<p><b>" + q->itemText( i ) + "</b><br>";
                ws += values[i].second;
            }
            ws += "<p>" + bottomWhatsThis;

            q->setWhatsThis( ws );
        }
    }

    K3b::IntMapComboBox* q;
};


K3b::IntMapComboBox::IntMapComboBox( QWidget* parent )
    : QComboBox( parent ),
      d( new Private() )
{
    d->q = this;

    connect( this, SIGNAL(highlighted(int)),
             this, SLOT(slotItemHighlighted(int)) );
    connect( this, SIGNAL(activated(int)),
             this, SLOT(slotItemActivated(int)) );

    setSizeAdjustPolicy(QComboBox::AdjustToContents);
}


K3b::IntMapComboBox::~IntMapComboBox()
{
    delete d;
}


int K3b::IntMapComboBox::selectedValue() const
{
    if( d->values.count() > QComboBox::currentIndex() &&
        QComboBox::currentIndex() >= 0 )
        return d->values[QComboBox::currentIndex()].first;
    else
        return 0;
}


void K3b::IntMapComboBox::setSelectedValue( int value )
{
    if( d->valueIndexMap.contains( value ) ) {
        QComboBox::setCurrentIndex( d->valueIndexMap[value] );
    }
}


bool K3b::IntMapComboBox::hasValue( int value ) const
{
    return d->valueIndexMap.contains( value );
}


void K3b::IntMapComboBox::clear()
{
    d->valueIndexMap.clear();
    d->values.clear();

    QComboBox::clear();
}


bool K3b::IntMapComboBox::insertItem( int value, const QString& text, const QString& description, int index )
{
    if( d->valueIndexMap.contains( value ) )
        return false;

    if ( index < 0 || index > QComboBox::count() ) {
        index = QComboBox::count();
    }

    d->values.insert( index, qMakePair<int, QString>( value, description ) );
    d->buildValueIndexMap();

    QComboBox::insertItem( index, text );

    d->updateWhatsThis();

    // select a default value. This is always wanted in K3b
    if ( QComboBox::currentIndex() < 0 ) {
        setSelectedValue( d->values[0].first );
    }

    return true;
}


void K3b::IntMapComboBox::slotItemHighlighted( int index )
{
    emit valueHighlighted( d->values[index].first );
}


void K3b::IntMapComboBox::slotItemActivated( int index )
{
    emit valueChanged( d->values[index].first );
}


void K3b::IntMapComboBox::addGlobalWhatsThisText( const QString& top, const QString& bottom )
{
    d->topWhatsThis = top;
    d->bottomWhatsThis = bottom;
    d->updateWhatsThis();
}


