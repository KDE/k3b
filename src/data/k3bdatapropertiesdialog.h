/***************************************************************************
                          k3bdatapropertiesdialog.h  -  description
                             -------------------
    begin                : Mon Dec 17 2001
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

#ifndef K3BDATAPROPERTIESDIALOG_H
#define K3BDATAPROPERTIESDIALOG_H

#include <kdialogbase.h>

class K3bDataItem;

class KLineEdit;
class QPushButton;
class QLabel;


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

  K3bDataItem* m_dataItem;
};

#endif
