/***************************************************************************
                          k3bmiscoptiontab.h  -  description
                             -------------------
    begin                : Tue Dec 18 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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
  void saveSettings();

 private slots:
  void slotGetTempDir();

 private:
  QCheckBox* m_checkShowSplash;

  QLineEdit*    m_editTempDir;
  QToolButton*  m_buttonTempDir;

};

#endif
