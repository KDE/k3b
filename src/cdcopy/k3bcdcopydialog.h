/***************************************************************************
                          k3bcdcopydialog.h  -  description
                             -------------------
    begin                : Sun Mar 17 2002
    copyright            : (C) 2002 by Sebastian Trueg
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

#ifndef K3BCDCOPYDIALOG_H
#define K3BCDCOPYDIALOG_H


#include <kdialogbase.h>

class K3bWriterSelectionWidget;
class K3bTempDirSelectionWidget;
class QCheckBox;
class QSpinBox;
class QComboBox;
class K3bDevice;


/**
  *@author Sebastian Trueg
  */
class K3bCdCopyDialog : public KDialogBase  
{
  Q_OBJECT

 public: 
  K3bCdCopyDialog(QWidget *parent = 0, const char *name = 0, bool modal = true );
  ~K3bCdCopyDialog();

  K3bDevice* readingDevice() const;

 private slots:
  void slotSourceSelected();
  void slotOnlyCreateImageChecked(bool);
  void slotUser1();
  void slotUser2();

 private:
  K3bWriterSelectionWidget* m_writerSelectionWidget;
  K3bTempDirSelectionWidget* m_tempDirSelectionWidget;
  QCheckBox* m_checkSimulate;
  QCheckBox* m_checkOnTheFly;
  QCheckBox* m_checkDeleteImages;
  QCheckBox* m_checkFastToc;
  QCheckBox* m_checkRawCopy;
  QCheckBox* m_checkTaoSource;
  QCheckBox* m_checkForce;
  QSpinBox*  m_spinTaoSourceAdjust;

  QCheckBox* m_checkOnlyCreateImage;
  QComboBox* m_comboSourceDevice;
  QComboBox* m_comboParanoiaMode;
  QComboBox* m_comboSubchanMode;
  QSpinBox* m_spinCopies;
};

#endif
