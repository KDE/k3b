/* 
 *
 * $Id$
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_JOB_PROGRESS_OSD_H_
#define _K3B_JOB_PROGRESS_OSD_H_

#include <qwidget.h>
#include <qpixmap.h>

class QPaintEvent;
class QMouseEvent;
class KConfig;

/**
 * An OSD displaying a text and a progress bar.
 *
 * Insprired by Amarok's OSD (I also took a bit of their code. :)
 */
class K3bJobProgressOSD : public QWidget
{
  Q_OBJECT

 public:
  K3bJobProgressOSD( QWidget* parent = 0, const char* name = 0 );
  ~K3bJobProgressOSD();

  enum Alignment {
    TOP,
    LEFT,
    RIGHT,
    BOTTOM
  };

  int screen() const { return m_screen; }
  Alignment alignment() const { return m_alignment; }
  int offset() const { return m_offset; }

  void readSettings( KConfig* );
  void saveSettings( KConfig* );

 public slots:
  void setScreen( int );
  void setText( const QString& );
  void setProgress( int );
  /**
   * The offset's meaning depends on the alignment
   */
  void setOffset( int );
  void setAlignment( Alignment );

  void show();

 protected:
  void paintEvent( QPaintEvent* );
  void mousePressEvent( QMouseEvent* );
  void mouseReleaseEvent( QMouseEvent* );
  void mouseMoveEvent( QMouseEvent* );
  void renderOSD();
  void refresh();
  void reposition( QSize size = QSize() );

 private:
  static const int s_outerMargin = 15;

  QPixmap m_osdBuffer;
  bool m_dirty;
  QString m_text;
  int m_progress;
  bool m_dragging;
  QPoint m_dragOffset;
  int m_screen;
  int m_offset;
  Alignment m_alignment;
};

#endif
