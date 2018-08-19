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
