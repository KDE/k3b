/* 
 *
 * $Id$
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



#ifndef K3BSETUPWIZARD_H
#define K3BSETUPWIZARD_H

#include <qvariant.h>
#include <kwizard.h>


class QCloseEvent;
class QKeyEvent;

class K3bSetup;
class KSimpleConfig;
class FstabEntriesTab;


class K3bSetupWizard : public KWizard
{ 
    Q_OBJECT

 public:
  K3bSetupWizard( K3bSetup*, QWidget* = 0, const char* = 0, bool = FALSE, WFlags = WType_TopLevel | WDestructiveClose );
  ~K3bSetupWizard();

  /**
   * reimplemented from QWizard
   */
  bool appropriate( QWidget* ) const;

  K3bSetup* setup() const { return m_setup; }

  void showPage( QWidget* );

 protected slots:
  void accept();
  void next();

 protected:
  void closeEvent( QCloseEvent* );
  void keyPressEvent( QKeyEvent* );

 private:
  K3bSetup* m_setup;
  FstabEntriesTab* m_fstabTab;
  bool m_accepted;
};

#endif // K3BSETUPWIZARD_H
