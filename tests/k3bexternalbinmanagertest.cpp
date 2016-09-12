/*
 * Copyright (C) 2016 Leslie Zhai <xiangzhai83@gmail.com>
 *
 * This file is part of the K3b project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bexternalbinmanagertest.h"
#include "k3bexternalbinmanager.h"

#include <QtTest/QTest>

QTEST_GUILESS_MAIN(ExternalBinManagerTest)

ExternalBinManagerTest::ExternalBinManagerTest()
{
}

void ExternalBinManagerTest::testBinObject()
{
    K3b::ExternalBinManager* binManager = new K3b::ExternalBinManager;
    if (binManager->binObject("ooo") && binManager->binObject("ooo")->hasFeature("fff")) {
        qDebug() << __PRETTY_FUNCTION__ << "it *NEVER* happened!";
    }
    // ooo binObject directly return 0
    // then hasFeature will segfault!
    // there are a lot of unchecking binObject is nullptr issue in k3b-2.0.3!!!
    //if (binManager->binObject("ooo")->hasFeature("fff")) {
    //}
}
