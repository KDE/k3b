/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_DVD_COPY_DIALOG_H_
#define _K3B_DVD_COPY_DIALOG_H_

#include <k3binteractiondialog.h>

class K3bTempDirSelectionWidget;
class K3bWriterSelectionWidget;
class K3bDeviceComboBox;
class QCheckBox;
class QSpinBox;
class K3bDvdCopyJob;
class K3bWritingModeWidget;


class K3bDvdCopyDialog : public K3bInteractionDialog
{
  Q_OBJECT

 public:
  K3bDvdCopyDialog( QWidget* parent = 0, const char* name = 0, bool modal = true );
  ~K3bDvdCopyDialog();

 private slots:
  void slotStartClicked();

  void slotLoadUserDefaults();
  void slotSaveUserDefaults();
  void slotLoadK3bDefaults();

  void slotToggleAll();

 private:
  void init();

  K3bWriterSelectionWidget* m_writerSelectionWidget;
  K3bTempDirSelectionWidget* m_tempDirSelectionWidget;
  K3bDeviceComboBox* m_comboSourceDevice;
  QCheckBox* m_checkSimulate;
  QCheckBox* m_checkDeleteImages;
  QCheckBox* m_checkOnlyCreateImage;
  QCheckBox* m_checkOnTheFly;
  QSpinBox* m_spinCopies;

  K3bWritingModeWidget* m_writingModeWidget;

  K3bDvdCopyJob* m_job;
};

#endif
