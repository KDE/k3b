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

#ifndef _K3B_JOB_PROGRESS_SYSTEMTRAY_H_
#define _K3B_JOB_PROGRESS_SYSTEMTRAY_H_

#include <qwidget.h>

class QPaintEvent;
class K3bJob;


class K3bJobProgressSystemTray : public QWidget
{
  Q_OBJECT

 public:
  K3bJobProgressSystemTray( QWidget* parent, const char* name = 0 );
  ~K3bJobProgressSystemTray();

 public slots:
  void setJob( K3bJob* );

 private slots:
  void slotProgress( int );
  void slotFinished( bool );

 protected:
  void paintEvent( QPaintEvent* );

 private:
  class Private;
  Private* d;
};

#endif
