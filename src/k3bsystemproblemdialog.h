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
namespace K3bDevice {
  class Device;
}


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


/**
 * The K3bSystemProblem checks for problems with the system setup
 * that could prevent K3b from funcioning properly. Examples are
 * missing external appplications like cdrecord or versions of 
 * external applications that are too old.
 *
 * Usage:
 * <pre>
 * if( K3bSystemProblemDialog::readCheckSystemConfig() )
 *    K3bSystemProblemDialog::checkSystem( this );
 * </pre>
 */
class K3bSystemProblemDialog : public KDialog
{
  Q_OBJECT

 public:
  /**
   * Determines if the system problem dialog should be shown or not.
   * It basicly reads a config entry. But in addition it
   * always forces the system check if a new version has been installed
   * or K3b is started for the first time.
   */
  static bool readCheckSystemConfig();
  static void checkSystem( QWidget* parent = 0, 
			   const char* name = 0 );

 protected:
  void closeEvent( QCloseEvent* );

 private slots:
  void slotK3bSetup();

 private:
  K3bSystemProblemDialog( const QValueList<K3bSystemProblem>&,
			  QWidget* parent = 0, 
			  const char* name = 0 );
  static int dmaActivated( K3bDevice::Device* );
  static QPtrList<K3bDevice::Device> checkForAutomounting();

  QPushButton* m_closeButton;
  QPushButton* m_k3bsetupButton;
  QCheckBox* m_checkDontShowAgain;
};

#endif
