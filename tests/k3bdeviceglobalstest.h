/*
    SPDX-FileCopyrightText: 2016-2017 Leslie Zhai <lesliezhai@llvm.org.cn>

    This file is part of the K3b project.

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef K3B_DEVICE_GLOBALS_TEST_H
#define K3B_DEVICE_GLOBALS_TEST_H

#include <QObject>

class DeviceGlobalsTest : public QObject
{
    Q_OBJECT
public:
    DeviceGlobalsTest();
private slots:
    void testFrom2Byte();
    void testFrom4Byte();
};

#endif // K3B_DEVICE_GLOBALS_TEST_H
