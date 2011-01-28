
#include "k3bglobalstest.h"
#include "k3bglobals.h"

#include <QtTest/QTest>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( GlobalsTest )

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
