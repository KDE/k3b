/*
 *
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bradioaction.h"


K3b::RadioAction::RadioAction( const QString& text, QObject* parent )
    : KToggleAction( text, parent ),
      m_alwaysEmit(false)
{
}

K3b::RadioAction::RadioAction( const KIcon& icon, const QString& text, QObject* parent )
    : KToggleAction( icon, text, parent ),
      m_alwaysEmit(false)
{
}

K3b::RadioAction::RadioAction( QObject* parent )
    : KToggleAction( parent ),
      m_alwaysEmit(false)
{
}


// void K3b::RadioAction::slotActivated()
// {
//   if( isChecked() ) {
//     if( m_alwaysEmit )
//       emit activated();

//     const QObject *senderObj = sender();

//     if ( !senderObj || !::qt_cast<const KToolBarButton *>( senderObj ) )
//       return;

//     const_cast<KToolBarButton *>( static_cast<const KToolBarButton *>( senderObj ) )->on( true );

//     return;
//   }

//   KToggleAction::slotActivated();
// }

#include "k3bradioaction.moc"
