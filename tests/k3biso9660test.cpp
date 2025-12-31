#include "k3biso9660test.h"
#include "k3biso9660.h"

#include <QTest>

QTEST_APPLESS_MAIN( Iso9660Test )

namespace {
    void verifyDirectoryEntry( const K3b::Iso9660Directory* dirEntry, const QString& expected )
    {
        if ( expected.isEmpty() ) {
            QVERIFY( !dirEntry );
        } else {
            QVERIFY( dirEntry );
            QVERIFY( dirEntry->entry(expected) );
        }
    }
} // namespace

void Iso9660Test::testEntries_data()
{
    // rockridge_joliet.iso
    // touch ¹²³.txt && genisoimage -graft-points -no-pad -rock -joliet -o rockridge_joliet.iso äöüß/=¹²³.txt && rm ¹²³.txt

    // rockridge.iso
    // touch ¹²³.txt && genisoimage -graft-points -no-pad -rock -o rockridge.iso äöüß/=¹²³.txt && rm ¹²³.txt

    // joliet.iso
    // touch ¹²³.txt && genisoimage -graft-points -no-pad -joliet -o joliet.iso äöüß/=¹²³.txt && rm ¹²³.txt

    // iso.iso
    // touch ¹²³.txt && genisoimage -graft-points -no-pad -o iso.iso äöüß/=¹²³.txt && rm ¹²³.txt

    QTest::addColumn<QString>( "filename" );
    QTest::addColumn<bool   >( "plainIso" );
    QTest::addColumn<QString>( "rr" );
    QTest::addColumn<QString>( "joliet" );
    QTest::addColumn<QString>( "iso" );

    QTest::newRow( "Rockridge/Joliet, !plainIso" )
        << "rockridge_joliet.iso" << false
        << "äöüß/¹²³.txt"         << "äöüß/¹²³.txt" << "äöüß/¹²³.txt";
    QTest::newRow( "Rockridge, !plainIso" )
        << "rockridge.iso"        << false
        << "äöüß/¹²³.txt"         << ""             << "äöüß/¹²³.txt";
    QTest::newRow( "Joliet, !plainIso" )
        << "joliet.iso"           << false
        << ""                     << "äöüß/¹²³.txt" << "________/______.TXT";
    QTest::newRow( "Iso, !plainIso" )
        << "iso.iso"              << false
        << ""                     << ""             << "________/______.TXT";

    QTest::newRow( "Rockridge/Joliet, plainIso" )
        << "rockridge_joliet.iso" << true
        << ""                     << ""             << "________/______.TXT";
    QTest::newRow( "Rockridge, plainIso" )
        << "rockridge.iso"        << true
        << ""                     << ""             << "________/______.TXT";
    QTest::newRow( "Joliet, plainIso" )
        << "joliet.iso"           << true
        << ""                     << ""             << "________/______.TXT";
    QTest::newRow( "Iso, plainIso" )
        << "iso.iso"              << true
        << ""                     << ""             << "________/______.TXT";
}

void Iso9660Test::testEntries()
{
    QFETCH( QString, filename );
    QFETCH( bool,    plainIso );
    QFETCH( QString, rr );
    QFETCH( QString, joliet );
    QFETCH( QString, iso );

    K3b::Iso9660 isoImage( QFINDTESTDATA( "testdata/" + filename ) );
    isoImage.setPlainIso9660( plainIso );
    QVERIFY( isoImage.open() );

    verifyDirectoryEntry( isoImage.firstRRDirEntry(),     rr );
    verifyDirectoryEntry( isoImage.firstJolietDirEntry(), joliet );
    verifyDirectoryEntry( isoImage.firstIsoDirEntry(),    iso );
}

#include "moc_k3biso9660test.cpp"
