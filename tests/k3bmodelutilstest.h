/*

    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3B_MODEL_UTILS_TEST_H
#define K3B_MODEL_UTILS_TEST_H

#include <QObject>

class ModelUtilsTest : public QObject
{
    Q_OBJECT
public:
    ModelUtilsTest();
private slots:
    void testCommonCheckState();
    void testToggleCommonCheckState();
    void testCommonText();
    void testSetCommonText();
};

#endif // K3B_GLOBALS_TEST_H
