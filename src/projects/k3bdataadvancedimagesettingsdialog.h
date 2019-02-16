/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
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
        explicit DataAdvancedImageSettingsDialog( QWidget* parent = 0 );
        ~DataAdvancedImageSettingsDialog() override;

        void load( const IsoOptions& options );
        void save( IsoOptions& options );
    };
}


#endif
