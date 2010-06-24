/*
 *
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bdataprojectdelegate.h"
#include <KLineEdit>
#include <KMimeType>
#include <QFocusEvent>

namespace K3b {

DataProjectDelegate::DataProjectDelegate( QObject* parent )
:
    QStyledItemDelegate( parent )
{
}


QWidget* DataProjectDelegate::createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& /*index*/ ) const
{
    KLineEdit* lineEdit = new KLineEdit( parent );
    lineEdit->setFrame( false );
    lineEdit->setAlignment( option.displayAlignment );
    lineEdit->installEventFilter( const_cast<DataProjectDelegate*>( this ) );
    return lineEdit;
}


bool DataProjectDelegate::eventFilter( QObject* object, QEvent* event )
{
    if( event->type() == QEvent::FocusIn ) {
        if( KLineEdit* lineEdit = qobject_cast<KLineEdit*>( object ) ) {
            const QString extension = KMimeType::extractKnownExtension( lineEdit->text() );
            // Select only filename without extension
            if( !extension.isEmpty() ) {
                const int selectionLength = lineEdit->text().length() - extension.length() - 1;
                lineEdit->setSelection( 0, selectionLength );
            }
            event->accept();
            return true;
        }
    }
    return QStyledItemDelegate::eventFilter( object, event );
}

} // namespace K3b

#include "k3bdataprojectdelegate.moc"
