/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3B_BOOTIMAGEDIALOG_H
#define K3B_BOOTIMAGEDIALOG_H

#include "ui_base_k3bbootimagedialog.h"

class QModelIndex;

namespace K3b {

    class DataDoc;
    class BootItem;
    class BootImageModel;

    class BootImageDialog : public QDialog, public Ui::base_K3bBootImageDialog
    {
        Q_OBJECT

    public:
        explicit BootImageDialog( DataDoc* doc, QWidget* parent = 0 );
        ~BootImageDialog() override;

    private Q_SLOTS:
        void slotNewBootImage();
        void slotDeleteBootImage();
        void slotToggleOptions();
        void slotCurrentChanged( const QModelIndex& current, const QModelIndex& previous );

        /* reimplemeted from base_...*/
        void slotOptionsChanged();

        void slotNoEmulationToggled( bool );

    private:
        void showAdvancedOptions( bool );
        void loadBootItemSettings( BootItem* );

        DataDoc* m_doc;
        BootImageModel* m_bootImageModel;

        bool m_loadingItem;
    };
}

#endif
