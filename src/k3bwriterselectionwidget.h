/***************************************************************************
                          k3bwriterselectionwidget.h  -  description
                             -------------------
    begin                : Mon Dec 3 2001
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

#ifndef K3BWRITERSELECTIONWIDGET_H
#define K3BWRITERSELECTIONWIDGET_H

#include <qgroupbox.h>


class QComboBox;
class K3bDevice;

/**
  *@author Sebastian Trueg
  */

class K3bWriterSelectionWidget : public QGroupBox
{
   Q_OBJECT

 public: 
  K3bWriterSelectionWidget(QWidget *parent=0, const char *name=0);
  ~K3bWriterSelectionWidget();

  int writerSpeed() const;
  K3bDevice* writerDevice() const;

 signals:
  void writerChanged();

 private slots:
  void slotRefreshWriterSpeeds();

 private:
  QComboBox* m_comboSpeed;
  QComboBox* m_comboWriter;
};

#endif
