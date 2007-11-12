/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#ifndef _K3B_PROGRESS_DIALOG_H_
#define _K3B_PROGRESS_DIALOG_H_

#include <kdialogbase.h>
#include "k3b_export.h"
//Added by qt3to4:
#include <QLabel>

class K3bBusyWidget;
class QLabel;
class KProgress;
class Q3WidgetStack;


/**
 * A progressdialog which displays a line of text and a progress
 * bar or a moving dot for tasks that do not provide any progress
 * information.
 */
class LIBK3B_EXPORT  K3bProgressDialog : public KDialogBase
{
  Q_OBJECT

 public:
  K3bProgressDialog( const QString& text = QString::null,
		     QWidget* parent = 0, 
		     const QString& caption = QString::null,
		     const char* name = 0 );
  ~K3bProgressDialog();

  int exec( bool showProgress );

 public slots:
  void setText( const QString& );
  void slotFinished( bool success );
  void setProgress( int p );

 private slots:
  void slotCancel();

 private:
  QLabel* m_label;
  Q3WidgetStack* m_stack;
  K3bBusyWidget* m_busyWidget;
  KProgress* m_progressBar;
};


#endif
