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

#ifndef K3B_DATA_VOLUMEDESC_DIALOG_H
#define K3B_DATA_VOLUMEDESC_DIALOG_H

#include "ui_base_k3bdatavolumedescdialog.h"

namespace K3b {
    class IsoOptions;

    class DataVolumeDescDialog : public QDialog, public Ui::base_K3bDataVolumeDescDialog
    {
        Q_OBJECT

    public:
        explicit DataVolumeDescDialog( QWidget* parent = 0 );
        ~DataVolumeDescDialog() override;

        void load( const IsoOptions& );
        void save( IsoOptions& );

    private Q_SLOTS:
        void slotVolumeSetSizeChanged( int );
    };
}

#endif
