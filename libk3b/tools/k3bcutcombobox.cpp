/*

    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#include "k3bcutcombobox.h"

#include <QEvent>
#include <QStringList>
#include <QRect>
#include <QSize>
#include <QFontMetrics>
#include <QPixmap>
#include <QSizePolicy>
#include <QStyle>


class K3b::CutComboBox::Private
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
K3b::CutComboBox::CutComboBox( QWidget* parent )
    : KComboBox( parent )
{
    d = new Private();
    //  setSizePolicy( QSizePolicy::Maximum, sizePolicy().horData(), sizePolicy().hasHeightForWidth() );
}


K3b::CutComboBox::CutComboBox( int method, QWidget* parent )
    : KComboBox( parent )
{
    d = new Private();
    d->method = method;
}


K3b::CutComboBox::CutComboBox()
{
    delete d;
}


void K3b::CutComboBox::setMethod( int m )
{
    d->method = m;
    cutText();
}


QSize K3b::CutComboBox::sizeHint() const
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

QSize K3b::CutComboBox::minimumSizeHint() const
{
    return KComboBox::minimumSizeHint();
}


void K3b::CutComboBox::setCurrentText( const QString& s )
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


void K3b::CutComboBox::insertStringList( const QStringList&, int )
{
    // FIXME
}

void K3b::CutComboBox::insertStrList( const char**, int, int)
{
    // FIXME
}

void K3b::CutComboBox::insertItem( const QString& text, int index )
{
    insertItem( index, QPixmap(), text );
}

void K3b::CutComboBox::insertItem( const QPixmap& pix, int i )
{
    insertItem( i, pix, "" );
}

void K3b::CutComboBox::insertItem( const QPixmap& pixmap, const QString& text, int index )
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

void K3b::CutComboBox::removeItem( int i )
{
    d->originalItems.erase( d->originalItems.at(i) );
    KComboBox::removeItem( i );
}

void K3b::CutComboBox::changeItem( const QString& s, int i )
{
    d->originalItems[i] = s;
    cutText();
}

void K3b::CutComboBox::changeItem( const QPixmap& pix, const QString& s, int i )
{
    KComboBox::changeItem( pix, i );
    changeItem( s, i );
}


QString K3b::CutComboBox::text( int i ) const
{
    if( i < (int)d->originalItems.count() )
        return d->originalItems[i];
    else
        return QString();
}


QString K3b::CutComboBox::currentText() const
{
    if( currentItem() < (int)d->originalItems.count() )
        return d->originalItems[currentItem()];
    else
        return QString();
}


void K3b::CutComboBox::clear()
{
    KComboBox::clear();
    d->originalItems.clear();
}

void K3b::CutComboBox::resizeEvent( QResizeEvent* e )
{
    cutText();

    KComboBox::resizeEvent(e);
}


void K3b::CutComboBox::cutText()
{
    d->width = QStyle::visualRect( style().querySubControlMetrics(QStyle::CC_ComboBox, this,
                                                                  QStyle::SC_ComboBoxEditField), this ).width();

    for( int i = 0; i < (int)d->originalItems.count(); ++i ) {
        int w = d->width;
        if ( pixmap(i) && !pixmap(i)->isNull() )
            w -= ( pixmap(i)->width() + 4 );

        QString text = fontMetrics().elidedText( d->originalItems[i],
                                                 d->method == SQUEEZE ? Qt::ElideMiddle : Qt::ElideRight,
                                                 w );

        // now insert the cut text
        if( pixmap(i) )
            KComboBox::changeItem( *pixmap(i), text, i );
        else
            KComboBox::changeItem( text, i );
    }
}


