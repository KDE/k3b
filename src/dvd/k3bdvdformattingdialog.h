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

#ifndef _K3B_DVD_FORMATTING_DIALOG_H_
#define _K3B_DVD_FORMATTING_DIALOG_H_

#include <k3binteractiondialog.h>


class QCheckBox;
class K3bWritingModeWidget;
class K3bWriterSelectionWidget;
class K3bDvdFormattingJob;


class K3bDvdFormattingDialog : public K3bInteractionDialog
{
  Q_OBJECT

 public:
  K3bDvdFormattingDialog( QWidget* = 0, const char* = 0, bool modal = true );
  ~K3bDvdFormattingDialog();

 protected slots:
  void slotStartClicked();
  void slotWriterChanged();

  void slotLoadUserDefaults();
  void slotSaveUserDefaults();
  void slotLoadK3bDefaults();

 private:
  K3bWriterSelectionWidget* m_writerSelectionWidget;
  K3bWritingModeWidget* m_writingModeWidget;
  QCheckBox* m_checkForce;
  QCheckBox* m_checkQuickFormat;

  K3bDvdFormattingJob* m_job;
};

#endif
