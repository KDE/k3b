/***************************************************************************
 *   Copyright (C) 2002 by Sebastian Trueg                                 *
 *   trueg@k3b.org                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/


#ifndef _K3B_MOVIX_BURN_DIALOG_H_
#define _K3B_MOVIX_BURN_DIALOG_H_

#include <k3bprojectburndialog.h>

class K3bMovixDoc;
class K3bMovixOptionsWidget;

class K3bMovixBurnDialog : public K3bProjectBurnDialog
{
  Q_OBJECT

 public:
  K3bMovixBurnDialog( K3bMovixDoc* doc, QWidget* parent = 0, const char* name = 0, bool modal = true );
  ~K3bMovixBurnDialog();

 protected slots:
  void loadDefaults();
  void loadUserDefaults();
  void saveUserDefaults();

 protected:
  void saveSettings();
  void readSettings();

 private:
  K3bMovixDoc* m_doc;
  K3bMovixOptionsWidget* m_movixOptionsWidget;
};


#endif

