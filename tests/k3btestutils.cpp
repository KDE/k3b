
#include "k3btestutils.h"

#include <QModelIndex>
#include <QtTest/QTest>

Q_DECLARE_METATYPE( QModelIndex )

namespace TestUtils
{

InsertRemoveModelSpy::InsertRemoveModelSpy( QObject* object, const char* beginSignal, const char* doneSignal )
:
    beginSpy( object, beginSignal ),
    doneSpy( object, doneSignal )
{
    qRegisterMetaType<QModelIndex>();
}

void InsertRemoveModelSpy::check( QModelIndex const& index, int first, int last )
{
    QVariantList args;

    QCOMPARE( beginSpy.size(), 1 );
    args = beginSpy.takeFirst();
    QCOMPARE( args.at( 0 ).typeName(), "QModelIndex" );
    QCOMPARE( args.at( 0 ).value<QModelIndex>(), index );
    QCOMPARE( args.at( 1 ).type(), QVariant::Int );
    QCOMPARE( args.at( 1 ).toInt(), first );
    QCOMPARE( args.at( 2 ).type(), QVariant::Int );
    QCOMPARE( args.at( 2 ).toInt(), last );

    QCOMPARE( doneSpy.size(), 1 );
    args = doneSpy.takeFirst();
    QCOMPARE( args.at( 0 ).typeName(), "QModelIndex" );
    QCOMPARE( args.at( 0 ).value<QModelIndex>(), index );
    QCOMPARE( args.at( 1 ).type(), QVariant::Int );
    QCOMPARE( args.at( 1 ).toInt(), first );
    QCOMPARE( args.at( 2 ).type(), QVariant::Int );
    QCOMPARE( args.at( 2 ).toInt(), last );
}

} // namespace TestUtils
