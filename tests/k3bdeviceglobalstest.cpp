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

#include "k3bdeviceglobalstest.h"
#include "k3bdeviceglobals.h"

#include <QtTest/QTest>

QTEST_GUILESS_MAIN(DeviceGlobalsTest)

DeviceGlobalsTest::DeviceGlobalsTest()
{
}

void DeviceGlobalsTest::testFrom2Byte()
{
    unsigned char* d = NULL;
    QCOMPARE(K3b::Device::from2Byte(d), (quint16)0);
    unsigned char buf[1] = { '\0' };
    QCOMPARE(K3b::Device::from2Byte(buf), (quint16)0);
}

void DeviceGlobalsTest::testFrom4Byte() 
{
    unsigned char d[1] = { '\0' };
    QCOMPARE(K3b::Device::from4Byte(d), (quint32)0);
}
