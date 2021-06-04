/*
    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3B_TEST_UTILS_H
#define K3B_TEST_UTILS_H

#include <QSignalSpy>

class QModelIndex;

namespace TestUtils
{

    class InsertRemoveModelSpy
    {
    public:
        InsertRemoveModelSpy( QObject* object, const char* beginSignal, const char* doneSignal );

        void check( QModelIndex const& index, int pos ) { check( index, pos, pos ); }
        void check( QModelIndex const& index, int first, int last );

    private:
        QSignalSpy beginSpy;
        QSignalSpy doneSpy;
    };

} // namespace TestUtils

#endif // K3B_TEST_UTILS_H
