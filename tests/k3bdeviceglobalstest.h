/*
 * Copyright (C) 2016 - 2017 Leslie Zhai <lesliezhai@llvm.org.cn>
 *
 * This file is part of the K3b project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
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
