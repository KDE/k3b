/*
    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3B_DATA_ADVANCED_IMAGE_SETTINGS_DIALOG_H
#define K3B_DATA_ADVANCED_IMAGE_SETTINGS_DIALOG_H

#include "ui_base_k3badvanceddataimagesettings.h"

namespace K3b {
    class IsoOptions;

    class DataAdvancedImageSettingsDialog : public QDialog, public Ui::base_K3bAdvancedDataImageSettings
    {
        Q_OBJECT

    public:
        explicit DataAdvancedImageSettingsDialog( QWidget* parent = nullptr );
        ~DataAdvancedImageSettingsDialog() override;

        void load( const IsoOptions& options );
        void save( IsoOptions& options );
    };
}


#endif
