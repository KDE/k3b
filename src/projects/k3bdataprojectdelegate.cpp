/*
 *
 *  Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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
#include <QFocusEvent>
#include <QLineEdit>

namespace K3b {

DataProjectDelegate::DataProjectDelegate( QObject* parent )
:
    QStyledItemDelegate( parent )
{
}

void DataProjectDelegate::setEditorData( QWidget* editor, const QModelIndex& index ) const
{
    if( QLineEdit* lineEdit = qobject_cast<QLineEdit*>( editor ) ) {
        const QString text = index.data().toString();
        lineEdit->setText( text );
        int dotPosition = text.lastIndexOf( QChar( '.') );
        lineEdit->setSelection( 0, 1 );
    }
    else {
        QStyledItemDelegate::setEditorData( editor, index );
    }
}

} // namespace K3b

#include "k3bdataprojectdelegate.moc"
