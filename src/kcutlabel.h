/* 
 *
 * $Id: $
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
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



/*
 * @ref QLabel
 */
class KCutLabel : public QLabel 
{
  Q_OBJECT

 public:
  /**
   * Default constructor.
   */
  KCutLabel( QWidget *parent = 0, const char *name = 0);
  KCutLabel( const QString &text, QWidget *parent = 0, const char *name = 0 );

  virtual QSize minimumSizeHint() const;

 public slots:
  void setText( const QString & );

 protected:
  /**
   * used when widget is resized
   */
  void resizeEvent( QResizeEvent * );
  /**
   * does the dirty work
   */
  void cutTextToLabel();
  QString cutToWidth( const QString&, int );
  QString m_fullText;
};

#endif // KCUTLABEL_H
