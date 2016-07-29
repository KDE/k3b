/*
 *
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_DATA_MULTISESSION_IMPORT_DIALOG_H_
#define _K3B_DATA_MULTISESSION_IMPORT_DIALOG_H_

#include <kdialog.h>

namespace K3b {
    class DataDoc;
    class Medium;

    namespace Device {
        class Device;
    }

    class DataMultisessionImportDialog : public KDialog
    {
        Q_OBJECT

    public:
        /**
         * Import a session into the project.
         * If the project is a DVD data project only DVD media are
         * presented for selection.
         *
         * \param doc if 0 a new project will be created.
         *
         * \return the project
         */
        static DataDoc* importSession( DataDoc* doc, QWidget* parent );

    private Q_SLOTS:
        void slotOk();
        void slotCancel();

        void importSession( K3b::DataDoc* doc );
        void slotSelectionChanged();
        void updateMedia();
        void addMedium( const K3b::Medium& medium );
        void showSessionInfo( K3b::Device::Device* dev, int session );

    private:
        DataMultisessionImportDialog( QWidget* parent );
        ~DataMultisessionImportDialog();

        class Private;
        Private* const d;
    };
}

#endif
