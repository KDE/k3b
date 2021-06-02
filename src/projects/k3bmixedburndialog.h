/*

    SPDX-FileCopyrightText: 2003 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef K3BMIXEDBURNDIALOG_H
#define K3BMIXEDBURNDIALOG_H

#include "k3bprojectburndialog.h"

class QCheckBox;
class QRadioButton;

namespace K3b {
    class MixedDoc;
    class DataImageSettingsWidget;
    class AudioCdTextWidget;
    class DataModeWidget;
    class IntMapComboBox;

    /**
     *@author Sebastian Trueg
     */
    class MixedBurnDialog : public ProjectBurnDialog
    {
        Q_OBJECT

    public:
        explicit MixedBurnDialog( MixedDoc*, QWidget *parent=0 );

    protected:
        void loadSettings( const KConfigGroup& ) override;
        void saveSettings( KConfigGroup ) override;
        void toggleAll() override;

        DataImageSettingsWidget* m_imageSettingsWidget;
        AudioCdTextWidget* m_cdtextWidget;

    protected Q_SLOTS:
        /**
         * Reimplemented for internal reasons (shut down the audio player)
         */
        void slotStartClicked() override;
        void saveSettingsToProject() override;
        void readSettingsFromProject() override;

        void slotCacheImageToggled( bool on );
        void slotNormalizeToggled( bool on );

    private:
        void setupSettingsPage();
        MixedDoc* m_doc;

        IntMapComboBox* m_comboMixedModeType;
        QRadioButton* m_radioMixedTypeFirstTrack;
        QRadioButton* m_radioMixedTypeLastTrack;
        QRadioButton* m_radioMixedTypeSessions;

        QCheckBox* m_checkNormalize;

        DataModeWidget* m_dataModeWidget;
    };
}

#endif
