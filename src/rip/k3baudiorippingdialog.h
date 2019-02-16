/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_AUDIO_RIPPING_DIALOG_H_
#define _K3B_AUDIO_RIPPING_DIALOG_H_

#include "k3binteractiondialog.h"
#include "k3btoc.h"
#include "k3bmedium.h"

#include <KCddb/Cdinfo>

#include <QList>
#include <QStringList>

class QCheckBox;
class QSpinBox;
class QComboBox;

namespace K3b {

    namespace Device {
        class Device;
    }

    class CddbPatternWidget;
    class AudioConvertingOptionWidget;

    class AudioRippingDialog : public InteractionDialog
    {
        Q_OBJECT

    public:
        AudioRippingDialog( const Medium&,
                            const KCDDB::CDInfo&,
                            const QList<int>&,
                            QWidget *parent = 0 );
        ~AudioRippingDialog() override;

        void setStaticDir( const QString& path );

    public Q_SLOTS:
        void refresh();
        void init() override;

    private Q_SLOTS:
        void slotStartClicked() override;

    private:
        Medium m_medium;
        KCDDB::CDInfo m_cddbEntry;
        QList<int> m_trackNumbers;

        QComboBox* m_comboParanoiaMode;
        QSpinBox* m_spinRetries;
        QCheckBox* m_checkIgnoreReadErrors;
        QCheckBox* m_checkUseIndex0;

        CddbPatternWidget* m_patternWidget;
        AudioConvertingOptionWidget* m_optionWidget;

        void setupGui();
        void setupContextHelp();

        void loadSettings( const KConfigGroup& ) override;
        void saveSettings( KConfigGroup ) override;

        class Private;
        Private* d;
    };
}

#endif
