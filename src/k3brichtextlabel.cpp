/* 
 *
 * $Id$
 * Copyright (C) 2005 Waldo Bastian <bastian@kde.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3brichtextlabel.h"

#include <qtooltip.h>
#include <qstylesheet.h>
#include <qsimplerichtext.h>

#include <kglobalsettings.h>

static QString qrichtextify( const QString& text )
{
  if ( text.isEmpty() || text[0] == '<' )
    return text;

  QStringList lines = QStringList::split('\n', text);
  for(QStringList::Iterator it = lines.begin(); it != lines.end(); ++it)
  {
    *it = QStyleSheet::convertFromPlainText( *it, QStyleSheetItem::WhiteSpaceNormal );
  }

  return lines.join(QString::null);
}

K3bRichTextLabel::K3bRichTextLabel( const QString &text , QWidget *parent, const char *name )
 : QLabel ( parent, name ) {
  m_defaultWidth = QMIN(400, KGlobalSettings::desktopGeometry(this).width()*2/5);
  setAlignment( Qt::WordBreak );
  setText(text);
}

K3bRichTextLabel::K3bRichTextLabel( QWidget *parent, const char *name )
 : QLabel ( parent, name ) {
  m_defaultWidth = QMIN(400, KGlobalSettings::desktopGeometry(this).width()*2/5);
  setAlignment( Qt::WordBreak );
}

void K3bRichTextLabel::setDefaultWidth(int defaultWidth)
{
  m_defaultWidth = defaultWidth;
  updateGeometry();
}

QSizePolicy K3bRichTextLabel::sizePolicy() const
{
  return QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum, false);
}

QSize K3bRichTextLabel::minimumSizeHint() const
{
  QString qt_text = qrichtextify( text() );
  int pref_width = 0;
  int pref_height = 0;
  QSimpleRichText rt(qt_text, font());
  pref_width = m_defaultWidth;
  rt.setWidth(pref_width);
  int used_width = rt.widthUsed();
  if (used_width <= pref_width)
  {
    while(true)
    {
      int new_width = (used_width * 9) / 10;
      rt.setWidth(new_width);
      int new_height = rt.height();
      if (new_height > pref_height)
        break;
      used_width = rt.widthUsed();
      if (used_width > new_width)
        break;
    }
    pref_width = used_width;
  }
  else
  {
    if (used_width > (pref_width *2))
      pref_width = pref_width *2;
    else
      pref_width = used_width;
  }

  return QSize(pref_width, rt.height());
}

QSize K3bRichTextLabel::sizeHint() const
{
  return minimumSizeHint();
}

void K3bRichTextLabel::setText( const QString &text ) {
  QLabel::setText(text);
}

void K3bRichTextLabel::virtual_hook( int, void* )
{ /*BASE::virtual_hook( id, data );*/ }

#include "k3brichtextlabel.moc"
