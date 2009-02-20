/* 
 *
 * Copyright (C) 2007-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
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
}

class K3bWriterSelectionWidget;
class K3bTempDirSelectionWidget;
class K3bMediaSelectionComboBox;
class QCheckBox;
class QSpinBox;
class QComboBox;
class K3bWritingModeWidget;
class QGroupBox;

class K3bMediaCopyDialog : public K3bInteractionDialog
{
    Q_OBJECT

public: 
    K3bMediaCopyDialog( QWidget *parent = 0 );
    ~K3bMediaCopyDialog();

    void setReadingDevice( K3bDevice::Device* );
    K3bDevice::Device* readingDevice() const;

private Q_SLOTS:
    void slotStartClicked();
    void updateOverrideDevice();

protected:
    void toggleAll();
    void init();

private:
    void loadUserDefaults( const KConfigGroup& );
    void saveUserDefaults( KConfigGroup );
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

    QGroupBox* m_groupAdvancedDataOptions;
    QGroupBox* m_groupAdvancedAudioOptions;
};

#endif
