/* 
 *
 * $Id$
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


#ifndef K3BDATAPROPERTIESDIALOG_H
#define K3BDATAPROPERTIESDIALOG_H

#include <kdialogbase.h>

class K3bDataItem;

class KLineEdit;
class QPushButton;
class QLabel;
class QCheckBox;


/**
  *@author Sebastian Trueg
  */
class K3bDataPropertiesDialog : public KDialogBase  
{
Q_OBJECT

 public: 
  K3bDataPropertiesDialog( K3bDataItem*, QWidget* parent = 0, const char* name = 0 );
  ~K3bDataPropertiesDialog();

 protected slots:
  void slotOk();

 private:
  KLineEdit* m_editName;
  QLabel* m_labelType;
  QLabel* m_labelLocation;
  QLabel* m_labelSize;

  QLabel* m_labelLocalName;
  QLabel* m_labelLocalLocation;

  QCheckBox* m_checkHideOnRockRidge;
  QCheckBox* m_checkHideOnJoliet;
  KLineEdit* m_editSortWeight;

  K3bDataItem* m_dataItem;
};

#endif
