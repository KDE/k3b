/*

    SPDX-FileCopyrightText: 2007-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef _K3B_MEDIA_COPY_DIALOG_H_
#define _K3B_MEDIA_COPY_DIALOG_H_

#include "k3binteractiondialog.h"
#include <KIO/Global>

class QCheckBox;
class QSpinBox;
class QGroupBox;
class QComboBox;

namespace K3b {
    namespace Device {
        class Device;
    }

    class WriterSelectionWidget;
    class TempDirSelectionWidget;
    class MediaSelectionComboBox;
    class WritingModeWidget;

    class MediaCopyDialog : public InteractionDialog
    {
        Q_OBJECT

    public:
        explicit MediaCopyDialog( QWidget *parent = 0 );
        ~MediaCopyDialog() override;

        void setReadingDevice( Device::Device* );
        Device::Device* readingDevice() const;

    private Q_SLOTS:
        void slotStartClicked() override;
        void updateOverrideDevice();

    protected:
        void toggleAll() override;
        void init() override;

    private:
        void loadSettings( const KConfigGroup& ) override;
        void saveSettings( KConfigGroup ) override;

        KIO::filesize_t neededSize() const;

        WriterSelectionWidget* m_writerSelectionWidget;
        TempDirSelectionWidget* m_tempDirSelectionWidget;
        QCheckBox* m_checkSimulate;
        QCheckBox* m_checkCacheImage;
        QCheckBox* m_checkDeleteImages;
        QCheckBox* m_checkOnlyCreateImage;
        QCheckBox* m_checkReadCdText;
        QCheckBox* m_checkIgnoreDataReadErrors;
        QCheckBox* m_checkIgnoreAudioReadErrors;
        QCheckBox* m_checkNoCorrection;
        QCheckBox* m_checkVerifyData;
        MediaSelectionComboBox* m_comboSourceDevice;
        QComboBox* m_comboParanoiaMode;
        QSpinBox* m_spinCopies;
        QSpinBox* m_spinDataRetries;
        QSpinBox* m_spinAudioRetries;
        WritingModeWidget* m_writingModeWidget;
        QComboBox* m_comboCopyMode;

        QGroupBox* m_groupAdvancedDataOptions;
        QGroupBox* m_groupAdvancedAudioOptions;
    };
}

#endif
