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


#ifndef _K3B_PROGRESS_DIALOG_H_
#define _K3B_PROGRESS_DIALOG_H_

#include <kdialogbase.h>


class K3bBusyWidget;
class QLabel;
class KProgress;
class QWidgetStack;


/**
 * A progressdialog which displays a line of text and a progress
 * bar or a moving dot for tasks that do not provide any progress
 * information.
 */
class K3bProgressDialog : public KDialogBase
{
  Q_OBJECT

 public:
  K3bProgressDialog( const QString& text = QString::null,
		     QWidget* parent = 0, const char* name = 0 );
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
  QWidgetStack* m_stack;
  K3bBusyWidget* m_busyWidget;
  KProgress* m_progressBar;
};


#endif
