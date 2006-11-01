/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef KCUTLABEL_H
#define KCUTLABEL_H

#include <qlabel.h>
#include "k3b_export.h"


/*
 * @ref QLabel
 */
class LIBK3B_EXPORT KCutLabel : public QLabel 
{
  Q_OBJECT

 public:
  /**
   * Default constructor.
   */
  KCutLabel( QWidget *parent = 0, const char *name = 0);
  KCutLabel( const QString &text, QWidget *parent = 0, const char *name = 0 );

  virtual QSize minimumSizeHint() const;

  /**
   * \return the full text while text() returns the cut text
   */
  const QString& fullText() const;

 public slots:
  void setText( const QString & );

  /**
   * \param i the number of characters that have to be visible. Default is 1.
   */
  void setMinimumVisibleText( int i );

 protected:
  /**
   * used when widget is resized
   */
  void resizeEvent( QResizeEvent * );
  /**
   * does the dirty work
   */
  void cutTextToLabel();

 private:
  QString m_fullText;
  int m_minChars;
};

#endif // KCUTLABEL_H
