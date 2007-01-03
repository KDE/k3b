/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
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

#include <kio/global.h>


namespace K3bDevice {
  class Device;
  class DeviceManager;
}

class K3bTempDirSelectionWidget;
class K3bWriterSelectionWidget;
class K3bMediaSelectionComboBox;
class QCheckBox;
class QSpinBox;
class K3bWritingModeWidget;


class K3bDvdCopyDialog : public K3bInteractionDialog
{
  Q_OBJECT

 public:
  K3bDvdCopyDialog( QWidget* parent = 0, const char* name = 0, bool modal = true );
  ~K3bDvdCopyDialog();

  void setReadingDevice( K3bDevice::Device* );
  K3bDevice::Device* readingDevice() const;

 private slots:
  void slotStartClicked();
  void slotSourceMediumChanged( K3bDevice::Device* );
  void updateOverrideDevice();

 protected:
  void init();
  void toggleAll();

 private:
  void loadUserDefaults( KConfigBase* );
  void saveUserDefaults( KConfigBase* );
  void loadK3bDefaults();

  KIO::filesize_t neededSize() const;

  K3bWriterSelectionWidget* m_writerSelectionWidget;
  K3bTempDirSelectionWidget* m_tempDirSelectionWidget;
  K3bMediaSelectionComboBox* m_comboSourceDevice;
  QCheckBox* m_checkSimulate;
  QCheckBox* m_checkDeleteImages;
  QCheckBox* m_checkOnlyCreateImage;
  QCheckBox* m_checkCacheImage;
  QCheckBox* m_checkVerifyData;
  QSpinBox* m_spinCopies;
  QSpinBox* m_spinRetries;
  QCheckBox* m_checkIgnoreReadErrors;
  K3bWritingModeWidget* m_writingModeWidget;
};

#endif
