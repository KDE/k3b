/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_DATA_MULTISESSION_IMPORT_DIALOG_H_
#define _K3B_DATA_MULTISESSION_IMPORT_DIALOG_H_

#include <QDialog>

namespace K3b {
    class DataDoc;
    class Medium;

    class DataMultisessionImportDialog : public QDialog
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

    private:
        explicit DataMultisessionImportDialog( QWidget* parent );
        ~DataMultisessionImportDialog() override;

        class Private;
        Private* const d;
    };
}

#endif
