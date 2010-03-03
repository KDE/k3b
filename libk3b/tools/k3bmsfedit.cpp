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

#include <QApplication>
#include <QFontMetrics>
#include <QLineEdit>
#include <QStringList>
#include <QStyle>
#include <QStyleOptionSpinBox>

#include <KDebug>


class K3b::MsfEdit::Private
{
public:
    void _k_editingFinished();

    Msf value;
    Msf minimum;
    Msf maximum;
    MsfEdit* q;
    
    QSize cachedSizeHint;
};


void K3b::MsfEdit::Private::_k_editingFinished()
{
    Msf newValue = Msf::fromString( q->lineEdit()->text() );
    if( newValue != value ) {
        value = newValue;
        emit q->valueChanged( value );
    }
}


K3b::MsfEdit::MsfEdit( QWidget* parent )
    : QAbstractSpinBox( parent ),
      d( new Private() )
{
    d->q = this;

    // some very high value (10000 minutes)
    setMaximum( 10000*60*75 );
    
    lineEdit()->setInputMask( "99:99:99" );
    lineEdit()->setText( d->value.toString() );
    //setSpecialValueText( QString() );
    //setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum, QSizePolicy::SpinBox ) );

    connect( this, SIGNAL(editingFinished()),
             this, SLOT(_k_editingFinished()) );
}


K3b::MsfEdit::~MsfEdit()
{
    delete d;
}


void K3b::MsfEdit::stepBy( int steps )
{
    // look if we are currently editing minutes or seconds
    QString text = lineEdit()->text();
    if( text.length() == 8 ) {
        const int pos = lineEdit()->cursorPosition();
        text = text.mid( pos );
        int num = text.count( ':' );
        if( num == 1 ) {
            d->value.addSeconds( steps );
        }
        else if( num == 2 ) {
            d->value.addMinutes( steps );
        }
        else {
            d->value.addFrames( steps );
        }
        lineEdit()->setText( d->value.toString() );
        lineEdit()->setCursorPosition( pos );
        emit valueChanged( d->value );
    }
}


QSize K3b::MsfEdit::sizeHint() const
{
    if (d->cachedSizeHint.isEmpty()) {
        ensurePolished();

        const QFontMetrics fm(fontMetrics());
        int h = lineEdit()->sizeHint().height();
        int w = qMax(w, fm.width( lineEdit()->inputMask() ));
        w += 2; // cursor blinking space

        QStyleOptionSpinBox opt;
        initStyleOption(&opt);
        QSize hint(w, h);
        QSize extra(35, 6);
        opt.rect.setSize(hint + extra);
        extra += hint - style()->subControlRect(QStyle::CC_SpinBox, &opt,
                                                QStyle::SC_SpinBoxEditField, this).size();
        // get closer to final result by repeating the calculation
        opt.rect.setSize(hint + extra);
        extra += hint - style()->subControlRect(QStyle::CC_SpinBox, &opt,
                                                QStyle::SC_SpinBoxEditField, this).size();
        hint += extra;

        opt.rect = rect();
        d->cachedSizeHint = style()->sizeFromContents(QStyle::CT_SpinBox, &opt, hint, this)
                            .expandedTo(QApplication::globalStrut());
    }
    return d->cachedSizeHint;
}


K3b::Msf K3b::MsfEdit::value() const
{
    return d->value;
}


K3b::Msf K3b::MsfEdit::maximum() const
{
    return d->maximum;
}


void K3b::MsfEdit::setMaximum( const Msf& max )
{
    d->maximum = max;
    if( d->value > d->maximum )
        d->value = d->maximum;
}


void K3b::MsfEdit::setValue( const K3b::Msf& value )
{
    if( d->value != value ) {
        d->value = value;
        lineEdit()->setText( d->value.toString() );
        emit valueChanged( d->value );
    }
}


QAbstractSpinBox::StepEnabled K3b::MsfEdit::stepEnabled () const
{
    StepEnabled stepEnabled;
    if( d->value > d->minimum )
        stepEnabled |= StepDownEnabled;
    if( d->value < d->maximum || d->maximum.totalFrames() == 0 )
        stepEnabled |= StepUpEnabled;
    return stepEnabled;
}

#include "k3bmsfedit.moc"
