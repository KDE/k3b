/***************************************************************************
                          k3bcdinfodialog.h  -  description
                             -------------------
    begin                : Mon Oct 29 2001
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

#ifndef K3BCDINFODIALOG_H
#define K3BCDINFODIALOG_H

#include <kdialogbase.h>

class QComboBox;
class K3bCdInfo;
class QPushButton;
class QCloseEvent;


/**
  *@author Sebastian Trueg
  */
class K3bCdInfoDialog : public KDialogBase
{
 Q_OBJECT

 public: 
  K3bCdInfoDialog( QWidget* parent = 0, const char* name = 0, bool modal = true );
  ~K3bCdInfoDialog();

 private slots:
  void slotDeviceChanged();

 private:
  QComboBox* m_comboDevice;
  QPushButton* m_buttonRefresh;

  K3bCdInfo* m_cdInfo;
};

#endif
