/*
 *
 * $Id$
 *
 * Implementation of KRestrictedLine
 *
 * Copyright (C) 1997 Michael Wiedmann, <mw@miwie.in-berlin.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <qkeycode.h>

#include "krestrictedline.h"

KRestrictedLine::KRestrictedLine( QWidget *parent, 
				  const char *name,
				  const QString& valid )
  : KLineEdit( parent, name )
{
    qsValidChars = valid;
    bCheckForInvalidChars = false;
}

KRestrictedLine::~KRestrictedLine()
{
  ;
}


void KRestrictedLine::keyPressEvent( QKeyEvent *e )
{
  // let QLineEdit process "special" keys and return/enter
  // so that we still can use the default key binding
  if (e->key() == Key_Enter || e->key() == Key_Return ||  e->key() == Key_Delete || e->ascii() < 32)
    {
      KLineEdit::keyPressEvent(e);
      return;
    }

  // do we have a list of valid chars (or invalid chars) &&
  // is the pressed key in the list of valid chars (or NOT in the list of invalid chars)?
  if ( ( !bCheckForInvalidChars && !qsValidChars.isEmpty() && !qsValidChars.contains(e->ascii()) ) ||
       ( bCheckForInvalidChars && !qsInvalidChars.isEmpty() && qsInvalidChars.contains(e->ascii() ) ) )
    {
      // invalid char, emit signal and return
      emit (invalidChar(e->key()));
      return;
    }
  else
    // valid char: let QLineEdit process this key as usual
    KLineEdit::keyPressEvent(e);

  return;
}


void KRestrictedLine::setValidChars( const QString& valid)
{
  qsValidChars = valid;
  bCheckForInvalidChars = false;
}


void KRestrictedLine::setInvalidChars( const QString& invalid)
{
  qsInvalidChars = invalid;
  bCheckForInvalidChars = true;
}


QString KRestrictedLine::validChars() const
{
  return qsValidChars;
}


QString KRestrictedLine::invalidChars() const
{
  return qsInvalidChars;
}

#include "krestrictedline.moc"
