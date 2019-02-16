/*
 *
 * Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
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

#ifndef K3BVCDBURNDIALOG_H
#define K3BVCDBURNDIALOG_H

#include "k3bprojectburndialog.h"
#include "k3bvcdoptions.h"

class QCheckBox;
class QGroupBox;
class QSpinBox;
class QRadioButton;
class QLabel;
class QLineEdit;
class QTextEdit;
namespace K3b {
    class VcdDoc;
}
class QButtonGroup;

namespace K3b {
    class VcdBurnDialog : public ProjectBurnDialog
    {
        Q_OBJECT

    public:
        explicit VcdBurnDialog( VcdDoc* doc, QWidget *parent = 0 );
        ~VcdBurnDialog() override;

        VcdDoc* vcdDoc() const
        {
            return m_vcdDoc;
        }

    protected:
        void setupAdvancedTab();
        void setupVideoCdTab();
        void setupLabelTab();
        void saveSettingsToProject() override;
        void readSettingsFromProject() override;

        void loadSettings( const KConfigGroup& ) override;
        void saveSettings( KConfigGroup ) override;

        // -----------------------------------------------------------
        // the video-cd-tab
        // -----------------------------------------------------------

        QButtonGroup* m_buttonGroupVcdFormat;
        QGroupBox *m_groupVcdFormat;
        QRadioButton* m_radioVcd11;
        QRadioButton* m_radioVcd20;
        QRadioButton* m_radioSvcd10;
        QRadioButton* m_radioHqVcd10;

        QGroupBox* m_groupOptions;
        QCheckBox* m_checkAutoDetect;
        QCheckBox* m_checkNonCompliant;
        QCheckBox* m_checkVCD30interpretation;
        QCheckBox* m_check2336;

        // CD-i
        QGroupBox* m_groupCdi;
        QCheckBox* m_checkCdiSupport;
        QTextEdit* m_editCdiCfg;


        // -----------------------------------------------------------
        // the video-label-tab
        // -----------------------------------------------------------

        QLineEdit* m_editVolumeId;
        QLineEdit* m_editPublisher;
        QLineEdit* m_editAlbumId;

        QSpinBox* m_spinVolumeCount;
        QSpinBox* m_spinVolumeNumber;

        // -----------------------------------------------------------
        // the advanced-tab
        // -----------------------------------------------------------

        QGroupBox* m_groupGeneric;
        QGroupBox* m_groupGaps;
        QGroupBox* m_groupMisc;

        QCheckBox* m_checkPbc;
        QCheckBox* m_checkSegmentFolder;
        QCheckBox* m_checkRelaxedAps;
        QCheckBox* m_checkUpdateScanOffsets;
        QCheckBox* m_checkGaps;

        QSpinBox* m_spinRestriction;
        QSpinBox* m_spinPreGapLeadout;
        QSpinBox* m_spinPreGapTrack;
        QSpinBox* m_spinFrontMarginTrack;
        QSpinBox* m_spinRearMarginTrack;
        QSpinBox* m_spinFrontMarginTrackSVCD;
        QSpinBox* m_spinRearMarginTrackSVCD;

        QLabel* m_labelRestriction;
        QLabel* m_labelPreGapLeadout;
        QLabel* m_labelPreGapTrack;
        QLabel* m_labelFrontMarginTrack;
        QLabel* m_labelRearMarginTrack;

        // -----------------------------------------------------------

    private:
        VcdDoc* m_vcdDoc;
        void setVolumeID( );
        void MarginChecked( bool );
        void saveCdiConfig();
        void loadCdiConfig();
        void loadDefaultCdiConfig();
        void toggleAll() override;

    protected Q_SLOTS:
        void slotStartClicked() override;

        void slotGapsChecked( bool );
        void slotSpinVolumeCount();
        void slotVcdTypeClicked( K3b::VcdOptions::MPEGVersion );
        void slotCdiSupportChecked( bool );
        void slotAutoDetect( bool );
    };
}

#endif
