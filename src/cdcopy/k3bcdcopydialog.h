/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *                    Klaus-Dieter Krannich <kd@k3b.org>
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


#ifndef K3BCDCOPYDIALOG_H
#define K3BCDCOPYDIALOG_H


#include <k3binteractiondialog.h>
#include <device/k3bdevice.h>

class K3bWriterSelectionWidget;
class K3bTempDirSelectionWidget;
class QCheckBox;
class QSpinBox;
class QComboBox;


/**
  *@author Sebastian Trueg
  */
class K3bCdCopyDialog : public K3bInteractionDialog
{
  Q_OBJECT

 public: 
  K3bCdCopyDialog(QWidget *parent = 0, const char *name = 0, bool modal = true );
  ~K3bCdCopyDialog();

  K3bDevice* readingDevice() const;

 private slots:
  void slotSourceSelected();
  void slotOnlyCreateImageChecked(bool);
  void slotStartClicked();

  void slotLoadUserDefaults();
  void slotSaveUserDefaults();
  void slotLoadK3bDefaults();

 private:
  void initReadingDevices();

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
