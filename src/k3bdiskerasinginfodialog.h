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


#ifndef K3B_DISK_ERASING_INFO_DIALOG_H
#define K3B_DISK_ERASING_INFO_DIALOG_H

#include <kdialogbase.h>

class K3bBusyWidget;
class QLabel;
class KProgress;


class K3bErasingInfoDialog : public KDialogBase
{
  Q_OBJECT

 public:
  K3bErasingInfoDialog( bool progress = false, QWidget* parent = 0, const char* name = 0 );
  ~K3bErasingInfoDialog();

 public slots:
  void slotFinished( bool success );
  void setProgress( int p );

 private:
  QLabel* m_label;
  K3bBusyWidget* m_busyWidget;
  KProgress* m_progressBar;

  bool m_progress;
};


#endif
