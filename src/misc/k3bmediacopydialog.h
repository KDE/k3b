/* 
 *
 * $Id: k3bcdcopydialog.h 619556 2007-01-03 17:38:12Z trueg $
 * Copyright (C) 2007 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_MEDIA_COPY_DIALOG_H_
#define _K3B_MEDIA_COPY_DIALOG_H_

#include "k3binteractiondialog.h"
#include <kio/global.h>

namespace K3bDevice {
  class Device;
  class DeviceManager;
}

class K3bWriterSelectionWidget;
class K3bTempDirSelectionWidget;
class K3bMediaSelectionComboBox;
class QCheckBox;
class QSpinBox;
class QComboBox;
class K3bWritingModeWidget;
class Q3ButtonGroup;
class Q3GroupBox;

class K3bMediaCopyDialog : public K3bInteractionDialog
{
  Q_OBJECT

 public: 
  K3bMediaCopyDialog( QWidget *parent = 0 );
  ~K3bMediaCopyDialog();

  void setReadingDevice( K3bDevice::Device* );
  K3bDevice::Device* readingDevice() const;

 private slots:
  void slotStartClicked();
  void updateOverrideDevice();

 protected:
  void toggleAll();
  void init();

 private:
  void loadUserDefaults( const KConfigGroup& );
  void saveUserDefaults( KConfigGroup& );
  void loadK3bDefaults();

  KIO::filesize_t neededSize() const;

  K3bWriterSelectionWidget* m_writerSelectionWidget;
  K3bTempDirSelectionWidget* m_tempDirSelectionWidget;
  QCheckBox* m_checkSimulate;
  QCheckBox* m_checkCacheImage;
  QCheckBox* m_checkDeleteImages;
  QCheckBox* m_checkOnlyCreateImage;
  QCheckBox* m_checkReadCdText;
  QCheckBox* m_checkIgnoreDataReadErrors;
  QCheckBox* m_checkIgnoreAudioReadErrors;
  QCheckBox* m_checkNoCorrection;
  QCheckBox* m_checkVerifyData;
  K3bMediaSelectionComboBox* m_comboSourceDevice;
  QComboBox* m_comboParanoiaMode;
  QSpinBox* m_spinCopies;
  QSpinBox* m_spinDataRetries;
  QSpinBox* m_spinAudioRetries;
  K3bWritingModeWidget* m_writingModeWidget;
  QComboBox* m_comboCopyMode;

  Q3GroupBox* m_groupAdvancedDataOptions;
  Q3GroupBox* m_groupAdvancedAudioOptions;
};

#endif
