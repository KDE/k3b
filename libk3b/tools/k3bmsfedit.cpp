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



#include "k3bmsfedit.h"
#include "k3bglobals.h"

#include <qstringlist.h>
#include <qlineedit.h>
#include <qstyle.h>
#include <qfontmetrics.h>
#include <qapplication.h>


K3bMsfValidator::K3bMsfValidator( QObject* parent )
    : QRegExpValidator( K3b::Msf::regExp(), parent )
{
}


class K3bMsfEdit::Private
{
public:
    void _k_valueChanged( int );
    int currentStepValue() const;

    K3bMsfEdit* q;
};


void K3bMsfEdit::Private::_k_valueChanged( int val )
{
    emit q->valueChanged( K3b::Msf( val ) );
}


int K3bMsfEdit::Private::currentStepValue() const
{
    int val = 1;

    // look if we are currently editing minutes or seconds
    QString text = q->lineEdit()->text();
    if( text.length() == 8 ) {
        text = text.mid( q->lineEdit()->cursorPosition() );
        int num = text.count( ':' );
        if( num == 1 )
            val = 75;
        else if( num == 2 )
            val = 60*75;
    }

    return val;
}


K3bMsfEdit::K3bMsfEdit( QWidget* parent )
    : QSpinBox( parent ),
      d( new Private() )
{
    d->q = this;

    // some very high value (10000 minutes)
    setRange( 0, 10000*60*75 );

    connect( this, SIGNAL(valueChanged(int)),
             this, SLOT(_k_valueChanged(int)) );
}


K3bMsfEdit::~K3bMsfEdit()
{
    delete d;
}


void K3bMsfEdit::stepBy( int steps )
{
    QSpinBox::stepBy( steps * d->currentStepValue() );
}


K3b::Msf K3bMsfEdit::msfValue() const
{
    return value();
}


void K3bMsfEdit::setMsfValue( const K3b::Msf& msf )
{
    setValue( msf.totalFrames() );
}


QString K3bMsfEdit::textFromValue( int value ) const
{
    return K3b::Msf( value ).toString();
}


int K3bMsfEdit::valueFromText( const QString& text ) const
{
    return K3b::Msf::fromString( text ).lba();
}

#include "k3bmsfedit.moc"
