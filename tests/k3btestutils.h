/*
 *
 * Copyright (C) 2011 Michal Malek <michalm@jabster.pl>
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

#ifndef K3B_TEST_UTILS_H
#define K3B_TEST_UTILS_H

#include <QtTest/QSignalSpy>

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
