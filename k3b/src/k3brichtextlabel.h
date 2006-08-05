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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef K3BRICHTEXTLABEL_H
#define K3BRICHTEXTLABEL_H

#include <qlabel.h>

/**
 * @short A replacement for QLabel that supports richtext and proper layout management
 *
 * @author Waldo Bastian <bastian@kde.org>
 */

/*
 * QLabel
 */
class K3bRichTextLabel : public QLabel {
  Q_OBJECT

public:
  /**
   * Default constructor.
   */
  K3bRichTextLabel( QWidget *parent, const char *name = 0 );
  K3bRichTextLabel( const QString &text, QWidget *parent, const char *name = 0 );

  int defaultWidth() const { return m_defaultWidth; }
  void setDefaultWidth(int defaultWidth);

  virtual QSize minimumSizeHint() const;
  virtual QSize sizeHint() const;
  QSizePolicy sizePolicy() const;

public slots:
  void setText( const QString & );

protected:
  int m_defaultWidth;

protected:
  virtual void virtual_hook( int id, void* data );
private:
  class K3bRichTextLabelPrivate;
  K3bRichTextLabelPrivate *d;
};

#endif // K3BRICHTEXTLABEL_H
