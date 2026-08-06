#include "k3bdefaultexternalprogramstest.h"
#include "k3bdefaultexternalprograms.h"

#include <QTest>

QTEST_APPLESS_MAIN( DefaultExternalProgramsTest )

void DefaultExternalProgramsTest::testContainsWord_data()
{
    QTest::addColumn<QString>( "haystack" );
    QTest::addColumn<QString>( "needle" );
    QTest::addColumn<bool>( "isContained" );

    QTest::newRow("empty")      << "abc"       << ""      << true;
    QTest::newRow("left")       << "abc def"   << "abc"   << true;
    QTest::newRow("right")      << "ghi\tjkl"  << "jkl"   << true;
    QTest::newRow("middle")     << "x\ny\rz"   << "y"     << true;
    QTest::newRow("match")      << "ab a c"    << "a"     << true;
    QTest::newRow("no match 1") << "x"         << "a"     << false;
    QTest::newRow("no match 2") << ""          << "a"     << false;
    QTest::newRow("partial 1")  << "appletree" << "apple" << false;
    QTest::newRow("partial 2")  << "appletree" << "tree"  << false;
}

void DefaultExternalProgramsTest::testContainsWord()
{
    QFETCH( QString, haystack );
    QFETCH( QString, needle );
    QFETCH( bool,    isContained );

    QCOMPARE( K3b::containsWord( haystack, needle ), isContained );
}

#include "moc_k3bdefaultexternalprogramstest.cpp"
