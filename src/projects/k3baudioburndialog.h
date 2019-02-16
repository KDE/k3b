/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BAUDIOBURNDIALOG_H
#define K3BAUDIOBURNDIALOG_H


#include "k3bprojectburndialog.h"

#include <QVariant>
#include <QShowEvent>
#include <QLabel>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QSpinBox;
class QShowEvent;

namespace K3b {
    class AudioDoc;
    class AudioCdTextWidget;

    /**
     *@author Sebastian Trueg
     */
    class AudioBurnDialog : public ProjectBurnDialog
    {
        Q_OBJECT

    public:
        explicit AudioBurnDialog(AudioDoc* doc, QWidget *parent=0 );
        ~AudioBurnDialog() override;

    protected:
        void saveSettingsToProject() override;
        void readSettingsFromProject() override;

        void loadSettings( const KConfigGroup& ) override;
        void saveSettings( KConfigGroup ) override;
        void showEvent( QShowEvent* ) override;
        void toggleAll() override;

    protected Q_SLOTS:
        /**
         * Reimplemented for internal reasons (shut down the audio player)
         */
        void slotStartClicked() override;
        void slotCacheImageToggled( bool on );
        void slotNormalizeToggled( bool on );

    private:
        /**
         * We need this here to be able to hide/show the group
         */
        QGroupBox* m_audioRippingGroup;
        QCheckBox* m_checkHideFirstTrack;
        QCheckBox* m_checkNormalize;
        QCheckBox* m_checkAudioRippingIgnoreReadErrors;
        QSpinBox* m_spinAudioRippingReadRetries;
        QComboBox* m_comboParanoiaMode;
        AudioCdTextWidget* m_cdtextWidget;
        AudioDoc* m_doc;
    };
}

#endif
