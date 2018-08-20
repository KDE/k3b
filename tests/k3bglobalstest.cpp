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

#include "k3bglobalstest.h"
#include "k3bglobals.h"

#include <QTest>

QTEST_GUILESS_MAIN( GlobalsTest )

GlobalsTest::GlobalsTest()
{
}

void GlobalsTest::testCutFilename()
{
    QCOMPARE( K3b::cutFilename( "abcd.txt", 2 ), QString( ".txt" ) );
    QCOMPARE( K3b::cutFilename( "abcd.txt", 3 ), QString( ".txt" ) );
    QCOMPARE( K3b::cutFilename( "abcd.txt", 8 ), QString( "abcd.txt" ) );
}

void GlobalsTest::testRemoveFilenameExtension()
{
    QCOMPARE( K3b::removeFilenameExtension( "abcd.txt" ), QString( "abcd" ) );
}


