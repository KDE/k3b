/* 
 *
 * $Id$
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


#ifndef _K3B_WRITING_MODE_WIDGET_H_
#define _K3B_WRITING_MODE_WIDGET_H_

#include <kcombobox.h>


class KConfig;


/**
 * Allows selection of K3b::WritingMode
 */
class K3bWritingModeWidget : public KComboBox
{
  Q_OBJECT

 public:
  K3bWritingModeWidget( QWidget* parent = 0, const char* name = 0 );
  K3bWritingModeWidget( int modes, QWidget* parent = 0, const char* name = 0 );
  ~K3bWritingModeWidget();

  int writingMode() const;

  void saveConfig( KConfig* );
  /**
   * This will not emit the writingModeChanged signal
   */
  void loadConfig( KConfig* );

 public slots:
  /**
   * This will not emit the writingModeChanged signal
   */
  void setWritingMode( int m );
  void setSupportedModes( int );

 signals:
  void writingModeChanged( int );

 private slots:
  void slotActivated( int );

 private:
  void init();

  class Private;
  Private* d;
};

#endif
