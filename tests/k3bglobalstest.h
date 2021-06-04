/*
    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3B_GLOBALS_TEST_H
#define K3B_GLOBALS_TEST_H

#include <QObject>

class GlobalsTest : public QObject
{
    Q_OBJECT
public:
    GlobalsTest();
private slots:
    void testCutFilename();
    void testRemoveFilenameExtension();
};

#endif // K3B_GLOBALS_TEST_H
