/* 
 *
 * $Id: $
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


#ifndef K3BMISCOPTIONTAB_H
#define K3BMISCOPTIONTAB_H

#include <qwidget.h>

class QCheckBox;
class QToolButton;
class QLineEdit;

/**
  *@author Sebastian Trueg
  */
class K3bMiscOptionTab : public QWidget
{
   Q_OBJECT

 public: 
  K3bMiscOptionTab(QWidget *parent=0, const char *name=0);
  ~K3bMiscOptionTab();

  void readSettings();
  bool saveSettings();

 private slots:
  void slotGetTempDir();

 private:
  QCheckBox* m_checkShowSplash;
  QCheckBox* m_checkShowSystemTrayProgress;

  QLineEdit*    m_editTempDir;
  QToolButton*  m_buttonTempDir;

};

#endif
