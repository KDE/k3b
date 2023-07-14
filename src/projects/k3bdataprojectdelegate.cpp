/*
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bdataprojectdelegate.h"
#include <QKeyEvent>
#include <QFocusEvent>
#include <QLineEdit>

namespace K3b {

DataProjectDelegate::DataProjectDelegate( QObject* parent )
:
    QStyledItemDelegate( parent )
{
}


QWidget* DataProjectDelegate::createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    m_current = index;
    QLineEdit* lineEdit = new QLineEdit( parent );
    lineEdit->setFrame( false );
    lineEdit->setAlignment( option.displayAlignment );
    lineEdit->installEventFilter( const_cast<DataProjectDelegate*>( this ) );
    return lineEdit;
}


bool DataProjectDelegate::eventFilter( QObject* object, QEvent* event )
{
    if( event->type() == QEvent::FocusIn ) {
        if( QLineEdit* editor = qobject_cast<QLineEdit*>( object ) ) {
            const QString extension = m_mimeDatabase.suffixForFileName( editor->text() );
            // Select only filename without extension
            if( !extension.isEmpty() ) {
                const int selectionLength = editor->text().length() - extension.length() - 1;
                editor->setSelection( 0, selectionLength );
            }
            event->accept();
            return true;
        }
    }
    else if( event->type() == QEvent::KeyPress ) {
        QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>( event );
        if( keyEvent->key() == Qt::Key_Return ) {
            QWidget* editor = dynamic_cast<QWidget*>( object );
            Q_EMIT commitData( editor );
            if( m_current.row() < m_current.model()->rowCount( m_current.parent() ) - 1 )
                Q_EMIT closeEditor( editor, EditNextItem );
            else
                Q_EMIT closeEditor( editor, NoHint );
            event->accept();
            return true;
        }
        else if( keyEvent->key() == Qt::Key_Up ) {
            Q_EMIT closeEditor( dynamic_cast<QWidget*>( object ), EditPreviousItem );
            event->accept();
            return true;
        }
        else if( keyEvent->key() == Qt::Key_Down ) {
            Q_EMIT closeEditor( dynamic_cast<QWidget*>( object ), EditNextItem );
            event->accept();
            return true;
        }
    }
    return QStyledItemDelegate::eventFilter( object, event );
}

} // namespace K3b

#include "moc_k3bdataprojectdelegate.cpp"
