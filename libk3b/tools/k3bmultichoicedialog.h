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

#ifndef _K3B_MULTI_CHOICE_DIALOG_H_
#define _K3B_MULTI_CHOICE_DIALOG_H_

#include <kdialog.h>
#include <kstdguiitem.h>
#include "k3b_export.h"

#include <qmessagebox.h>


class QCloseEvent;

class LIBK3B_EXPORT K3bMultiChoiceDialog : public KDialog
{
  Q_OBJECT

 public:
  K3bMultiChoiceDialog( const QString& caption,
			const QString& text,
			QMessageBox::Icon = QMessageBox::Information,
			QWidget* parent = 0, const char* name = 0 );
  ~K3bMultiChoiceDialog();

  /**
   * Adds a new button. returns it's number starting at 1.
   */
  int addButton( const KGuiItem& );

  static int choose( const QString& caption,
		     const QString& text,
		     QMessageBox::Icon = QMessageBox::Information,
		     QWidget* parent = 0, 
		     const char* name = 0,
		     int buttonCount = 2,
		     const KGuiItem& b1 = KStdGuiItem::yes(),
		     const KGuiItem& b2 = KStdGuiItem::no(),
		     const KGuiItem& b3 = KStdGuiItem::no(),
		     const KGuiItem& b4 = KStdGuiItem::no(),
		     const KGuiItem& b5 = KStdGuiItem::no(),
		     const KGuiItem& b6 = KStdGuiItem::no() );
		     
 public slots:
  /**
   * returnes the number of the clicked button starting at 1.
   */
  int exec();

 private slots:
  void slotButtonClicked( int );

 private:
  void closeEvent( QCloseEvent* );

  class Private;
  Private* d;
};

#endif
