/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_MULTI_CHOICE_DIALOG_H_
#define _K3B_MULTI_CHOICE_DIALOG_H_

#include "k3b_export.h"

#include <KStandardGuiItem>
#include <QDialog>
#include <QMessageBox>

class QCloseEvent;

namespace K3b {
    class LIBK3B_EXPORT MultiChoiceDialog : public QDialog
    {
        Q_OBJECT

    public:
        MultiChoiceDialog( const QString& caption,
                           const QString& text,
                           QMessageBox::Icon = QMessageBox::Information,
                           QWidget* parent = 0 );
        ~MultiChoiceDialog() override;

        /**
         * Adds a new button. returns it's number starting at 1.
         */
        int addButton( const KGuiItem& );

        static int choose( const QString& caption,
                           const QString& text,
                           QMessageBox::Icon,
                           QWidget* parent,
                           int buttonCount,
                           const KGuiItem& b1,
                           const KGuiItem& b2,
                           const KGuiItem& b3,
                           const KGuiItem& b4,
                           const KGuiItem& b5,
                           const KGuiItem& b6 );

    public Q_SLOTS:
        /**
         * returns the number of the clicked button starting at 1.
         */
        int exec() override;

    private Q_SLOTS:
        void slotButtonClicked( int );

    private:
        void closeEvent( QCloseEvent* ) override;

        class Private;
        Private* d;
    };
}

#endif
