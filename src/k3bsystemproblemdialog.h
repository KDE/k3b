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


#ifndef _K3B_SYSTEM_DIALOG_H_
#define _K3B_SYSTEM_DIALOG_H_

#include <qstring.h>
#include <qvaluelist.h>

#include <kdialog.h>

class QPushButton;
class QCheckBox;
class QCloseEvent;


class K3bSystemProblem
{
 public:
  K3bSystemProblem( int type = NON_CRITICAL,
		    const QString& problem = QString::null,
		    const QString& details = QString::null,
		    const QString& solution = QString::null,
		    bool k3bsetup = false );

  enum {
    CRITICAL,
    NON_CRITICAL,
    WARNING
  };

  int type;
  QString problem;
  QString details;
  QString solution;
  bool solvableByK3bSetup;
};


class K3bSystemProblemDialog : public KDialog
{
  Q_OBJECT

 public:
  K3bSystemProblemDialog( const QValueList<K3bSystemProblem>&,
			  QWidget* parent = 0, 
			  const char* name = 0 );

  static void checkSystem( QWidget* parent = 0, 
			   const char* name = 0 );

 protected:
  void closeEvent( QCloseEvent* );

 private slots:
  void slotK3bSetup();

 private:
  QPushButton* m_closeButton;
  QPushButton* m_k3bsetupButton;
  QCheckBox* m_checkDontShowAgain;
};

#endif
