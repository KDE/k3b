/*
    SPDX-FileCopyrightText: 2003-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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

        /* reimplemented from base_...*/
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
