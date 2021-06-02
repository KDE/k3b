/*

    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/



#include "k3bmsfedit.h"
#include "k3bglobals.h"

#include <QStringList>
#include <QFontMetrics>
#include <QApplication>
#include <QLineEdit>
#include <QStyle>
#include <QStyleOptionSpinBox>

#include <cmath>

class K3b::MsfEdit::Private
{
public:
    void _k_editingFinished();
    QString stringValue() const;

    Msf value;
    Msf minimum;
    Msf maximum;
    MsfEdit* q;
    
    QSize cachedSizeHint;
    int minutesWidth;
};


void K3b::MsfEdit::Private::_k_editingFinished()
{
    q->setValue( Msf::fromString( q->lineEdit()->text() ) );
}


QString K3b::MsfEdit::Private::stringValue() const
{
    return QString( "%1:%2:%3" )
        .arg( QString::number( value.minutes() ).rightJustified( minutesWidth, QLatin1Char( '0' ) ) )
        .arg( QString::number( value.seconds() ).rightJustified( 2, QLatin1Char( '0' ) ) )
        .arg( QString::number( value.frames() ).rightJustified( 2, QLatin1Char( '0' ) ) );
}


K3b::MsfEdit::MsfEdit( QWidget* parent )
    : QAbstractSpinBox( parent ),
      d( new Private() )
{
    d->q = this;

    // some very high value (10000 minutes)
    setMaximum( 10000*60*75 );

    lineEdit()->setText( d->stringValue() );

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
    const int pos = lineEdit()->cursorPosition();
    text = text.mid( pos );
    int num = text.count( ':' );
    
    Msf newValue = d->value;
    if( num == 1 ) {
        newValue.addSeconds( steps );
    }
    else if( num == 2 ) {
        newValue.addMinutes( steps );
    }
    else {
        newValue.addFrames( steps );
    }
    
    setValue( newValue );
    lineEdit()->setCursorPosition( pos );
}


QSize K3b::MsfEdit::sizeHint() const
{
    if (d->cachedSizeHint.isEmpty()) {
        ensurePolished();

        const QFontMetrics fm(fontMetrics());
        int h = lineEdit()->sizeHint().height();
        int w = fm.boundingRect( lineEdit()->inputMask() ).width();
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


K3b::Msf K3b::MsfEdit::minimum() const
{
    return d->minimum;
}


K3b::Msf K3b::MsfEdit::maximum() const
{
    return d->maximum;
}


void K3b::MsfEdit::setMinimum( const Msf& min )
{
    d->minimum = min;
    if( d->value < d->minimum )
        d->value = d->minimum;
    if( d->maximum < d->minimum )
        d->maximum = d->minimum;
}


void K3b::MsfEdit::setMaximum( const Msf& max )
{
    d->maximum = max;
    if( d->value > d->maximum )
        d->value = d->maximum;
    if( d->minimum > d->maximum )
        d->minimum = d->maximum;

    // Compute number of allowed positions for given maximum
    d->minutesWidth = static_cast<int>( std::log10( static_cast<float>( d->maximum.minutes() ) ) ) + 1;
    QString inputMask( ":99:99" );
    for( int i = 0; i < d->minutesWidth; ++i )
        inputMask.prepend( '9' );
    lineEdit()->setInputMask( inputMask );
}


void K3b::MsfEdit::setValue( const K3b::Msf& value )
{
    if( d->value != value ) {
        d->value = value;
        if( d->value < d->minimum )
            d->value = d->minimum;
        else if( d->value > d->maximum )
            d->value = d->maximum;
        
        lineEdit()->setText( d->stringValue() );
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

#include "moc_k3bmsfedit.cpp"
