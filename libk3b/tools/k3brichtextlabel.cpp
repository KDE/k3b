/* 
 *
 * Copyright (C) 2005 Waldo Bastian <bastian@kde.org>
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

#include "k3brichtextlabel.h"

#include <qtooltip.h>
#include <q3stylesheet.h>
#include <q3simplerichtext.h>
//Added by qt3to4:
#include <QLabel>

#include <kglobalsettings.h>
#include <QTextDocument>

static QString qrichtextify( const QString& text )
{
  if ( text.isEmpty() || text[0] == '<' )
    return text;

  QStringList lines = text.split('\n');
  for(QStringList::Iterator it = lines.begin(); it != lines.end(); ++it)
  {
    *it = Qt::convertFromPlainText( *it, Qt::WhiteSpaceNormal );
  }

  return lines.join(QString());
}

K3bRichTextLabel::K3bRichTextLabel( const QString &text , QWidget *parent )
 : QLabel ( parent ) {
  m_defaultWidth = qMin(400, KGlobalSettings::desktopGeometry(this).width()*2/5);
  setWordWrap( true );
  setText(text);
}

K3bRichTextLabel::K3bRichTextLabel( QWidget *parent )
 : QLabel ( parent ) {
  m_defaultWidth = qMin(400, KGlobalSettings::desktopGeometry(this).width()*2/5);
  setWordWrap( true );
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
  Q3SimpleRichText rt(qt_text, font());
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
