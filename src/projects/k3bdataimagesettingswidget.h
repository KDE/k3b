/*

    SPDX-FileCopyrightText: 2003 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef K3B_DATAIMAGE_SETTINGS_WIDGET_H
#define K3B_DATAIMAGE_SETTINGS_WIDGET_H

#include "ui_base_k3bdataimagesettings.h"

#include <QWidget>

namespace K3b {

    class IsoOptions;
    class DataVolumeDescDialog;
    class DataAdvancedImageSettingsDialog;

    class DataImageSettingsWidget : public QWidget, public Ui::base_K3bDataImageSettings
    {
        Q_OBJECT

    public:
        explicit DataImageSettingsWidget( QWidget* parent = 0 );
        ~DataImageSettingsWidget() override;

        void load( const IsoOptions& );
        void save( IsoOptions& );

        void showFileSystemOptions( bool );

    private Q_SLOTS:
        void slotSpaceHandlingChanged( int i );
        void slotCustomFilesystems();
        void slotMoreVolDescFields();
        void slotFilesystemsChanged();

    private:
        DataAdvancedImageSettingsDialog* m_customFsDlg;
        DataVolumeDescDialog* m_volDescDlg;

        bool m_fileSystemOptionsShown;
    };
}


#endif
