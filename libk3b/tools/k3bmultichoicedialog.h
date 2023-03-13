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
                           QMessageBox::Icon = QMessageBox::Information,
                           QWidget* parent = 0,
                           int buttonCount = 2,
                           const KGuiItem& b1 = KStandardGuiItem::ok(),
                           const KGuiItem& b2 = KStandardGuiItem::cancel(),
                           const KGuiItem& b3 = KStandardGuiItem::cancel(),
                           const KGuiItem& b4 = KStandardGuiItem::cancel(),
                           const KGuiItem& b5 = KStandardGuiItem::cancel(),
                           const KGuiItem& b6 = KStandardGuiItem::cancel() );

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
