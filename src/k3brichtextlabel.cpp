/* This file is part of the KDE libraries
   Copyright (C) 2005 Waldo Bastian <bastian@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
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

K3BRichTextLabel::K3BRichTextLabel( const QString &text , QWidget *parent, const char *name )
 : QLabel ( parent, name ) {
  m_defaultWidth = QMIN(400, KGlobalSettings::desktopGeometry(this).width()*2/5);
  setAlignment( Qt::WordBreak );
  setText(text);
}

K3BRichTextLabel::K3BRichTextLabel( QWidget *parent, const char *name )
 : QLabel ( parent, name ) {
  m_defaultWidth = QMIN(400, KGlobalSettings::desktopGeometry(this).width()*2/5);
  setAlignment( Qt::WordBreak );
}

void K3BRichTextLabel::setDefaultWidth(int defaultWidth)
{
  m_defaultWidth = defaultWidth;
  updateGeometry();
}

QSizePolicy K3BRichTextLabel::sizePolicy() const
{
  return QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum, false);
}

QSize K3BRichTextLabel::minimumSizeHint() const
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

QSize K3BRichTextLabel::sizeHint() const
{
  return minimumSizeHint();
}

void K3BRichTextLabel::setText( const QString &text ) {
  QLabel::setText(text);
}

void K3BRichTextLabel::virtual_hook( int, void* )
{ /*BASE::virtual_hook( id, data );*/ }

#include "k3brichtextlabel.moc"
