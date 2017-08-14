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
    QCOMPARE(K3b::Device::from2Byte(d), (quint16)0);   // Invalid Byte!
    const unsigned char buf0[] = { '\0' };
    QCOMPARE(K3b::Device::from2Byte(buf0), (quint16)0);
    unsigned const char buf1[] = { 0x00, 0x00 };
    QCOMPARE(K3b::Device::from2Byte(buf1), (quint16)0x0000);
    unsigned const char buf2[] = { 0x00, 0x70 };
    QCOMPARE(K3b::Device::from2Byte(buf2), (quint16)0x0070);
    unsigned const char buf3[] = { 0x05, 0x00 };
    QCOMPARE(K3b::Device::from2Byte(buf3), (quint16)0x0500);
    unsigned const char buf4[] = { 0xF0, 0x03 };
    QCOMPARE(K3b::Device::from2Byte(buf4), (quint16)0xF003);
}

void DeviceGlobalsTest::testFrom4Byte() 
{
    const unsigned char buf0[] = { '\0' };
    QCOMPARE(K3b::Device::from4Byte(buf0), (quint32)0);
    unsigned const char buf1[] = { 0x00, 0x00, 0x00, 0x00 };
    QCOMPARE(K3b::Device::from4Byte(buf1), (quint32)0x00000000);
    unsigned const char buf2[] = { 0x00, 0x00, 0x00, 0x01 };
    QCOMPARE(K3b::Device::from4Byte(buf2), (quint32)0x00000001);
    unsigned const char buf3[] = { 0x12, 0x34, 0x56, 0x78 };
    QCOMPARE(K3b::Device::from4Byte(buf3), (quint32)0x12345678);
}
