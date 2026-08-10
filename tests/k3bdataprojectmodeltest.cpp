/*
    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bdataprojectmodeltest.h"
#include "k3bdatadoc.h"
#include "k3bdataprojectmodel.h"
#include "k3bdataitem.h"
#include "k3bdiritem.h"
#include "k3bfileitem.h"
#include "k3biso9660.h"
#include "k3bsessionimportitem.h"
#include "k3bspecialdataitem.h"
#include "k3btestutils.h"

#include <QSignalSpy>
#include <QTest>

QTEST_GUILESS_MAIN( DataProjectModelTest )

Q_DECLARE_METATYPE( QModelIndex )


namespace {
    void verifyFile( const K3b::DataItem* item,
                     const char expectedFilename[],
                     const K3b::DataItem* expectedItemFromOldSession )
    {
        QCOMPARE( item->k3bName(), expectedFilename );
        QCOMPARE( static_cast<const K3b::FileItem*>(item)->replaceItemFromOldSession(),
                  expectedItemFromOldSession );
    }
} // namespace


DataProjectModelTest::DataProjectModelTest()
{
    qRegisterMetaType<QModelIndex>();

    // create test.iso:
    // touch file2 && genisoimage -no-pad -rock -o test.iso file2 && rm file2
    m_iso = new K3b::Iso9660( QFINDTESTDATA( "testdata/test.iso" ) );
    m_iso->open();
}


DataProjectModelTest::~DataProjectModelTest()
{
    delete m_iso;
}


void DataProjectModelTest::init()
{
    const K3b::Iso9660File* isoFile = static_cast<const K3b::Iso9660File*>
        ( m_iso->firstIsoDirEntry()->entry("file2") );

    m_doc = new K3b::DataDoc;
    m_doc->newDocument();
    m_doc->root()->addDataItem( new K3b::DirItem( "First directory" ) ); // index 0
    m_doc->root()->addDataItem( new K3b::SpecialDataItem( 1024, "file1" ) ); // index 1
    m_doc->root()->addDataItem( new K3b::SpecialDataItem( 2048, "file2" ) ); // index 2
    K3b::DirItem* secondDirectory = new K3b::DirItem( "Second directory" );
    m_doc->root()->addDataItem( secondDirectory ); // index 3
    secondDirectory->addDataItem( new K3b::SpecialDataItem( 1024, "file1" ) ); // index 3 -> 0
    secondDirectory->addDataItem( new K3b::SessionImportItem( isoFile ) ); // index 3 -> 1
    m_doc->root()->addDataItem( new K3b::SpecialDataItem( 300, "file3" ) ); // index 4
    m_doc->root()->addDataItem( new K3b::SpecialDataItem( 400, "file4" ) ); // index 5
}


void DataProjectModelTest::cleanup()
{
    m_doc->deleteLater();
}


void DataProjectModelTest::testCreate()
{
    K3b::DataProjectModel model( m_doc );
}


void DataProjectModelTest::testAdd()
{
    K3b::DataProjectModel model( m_doc );
    TestUtils::InsertRemoveModelSpy spy( &model,
                                         SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
                                         SIGNAL(rowsInserted(QModelIndex,int,int)) );

    m_doc->addEmptyDir( "Third directory", m_doc->root() );
    spy.check( model.indexForItem( m_doc->root() ), 6 );
}


void DataProjectModelTest::testRemove()
{
    QVariantList args;
    K3b::DataProjectModel model( m_doc );
    TestUtils::InsertRemoveModelSpy spy( &model,
                                         SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                                         SIGNAL(rowsRemoved(QModelIndex,int,int)) );

    m_doc->root()->removeDataItems( 4, 1 );
    spy.check( model.indexForItem( m_doc->root() ), 4 );

    K3b::DirItem* dir = dynamic_cast<K3b::DirItem*>( m_doc->root()->children().at( 3 ) );
    QVERIFY( dir != 0 );
    dir->removeDataItems( 0, 1 );
    spy.check( model.indexForItem( dir ), 0 );

    m_doc->root()->removeDataItems( 3, 1 );
    spy.check( model.indexForItem( m_doc->root() ), 3 );
}


void DataProjectModelTest::testReplace()
{
    K3b::DataProjectModel model( m_doc );
    K3b::DirItem* secondDirectory = m_doc->root()->find( "Second directory" )->getDirItem();
    const K3b::DataItem* itemFromOldSession = secondDirectory->find( "file2" );

    TestUtils::InsertRemoveModelSpy removeSpy( &model,
                                               SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                                               SIGNAL(rowsRemoved(QModelIndex,int,int)) );
    TestUtils::InsertRemoveModelSpy insertSpy( &model,
                                               SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
                                               SIGNAL(rowsInserted(QModelIndex,int,int)) );

    secondDirectory->addDataItem( new K3b::FileItem( nullptr, nullptr, "file2", *m_doc ) );

    const K3b::DirItem::Children& children = secondDirectory->children();
    QCOMPARE( children.size(), 2 );
    verifyFile( children.at( 0 ), "file1", nullptr );
    verifyFile( children.at( 1 ), "file2", itemFromOldSession );

    removeSpy.check( model.indexForItem( secondDirectory ), 1 );
    insertSpy.check( model.indexForItem( secondDirectory ), 1 );
}


void DataProjectModelTest::testAddAndReplace()
{
    K3b::DataProjectModel model( m_doc );
    K3b::DirItem* secondDirectory = m_doc->root()->find( "Second directory" )->getDirItem();
    const K3b::DataItem* itemFromOldSession = secondDirectory->find( "file2" );

    TestUtils::InsertRemoveModelSpy removeSpy( &model,
                                               SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                                               SIGNAL(rowsRemoved(QModelIndex,int,int)) );
    TestUtils::InsertRemoveModelSpy insertSpy( &model,
                                               SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
                                               SIGNAL(rowsInserted(QModelIndex,int,int)) );

    secondDirectory->addDataItems( {
        new K3b::FileItem( nullptr, nullptr, "file1", *m_doc ),
        new K3b::FileItem( nullptr, nullptr, "file2", *m_doc ),
        new K3b::FileItem( nullptr, nullptr, "file3", *m_doc ),
    } );

    const K3b::DirItem::Children& children = secondDirectory->children();
    QCOMPARE( children.size(), 4 );
    verifyFile( children.at( 0 ), "file1",  nullptr );
    verifyFile( children.at( 1 ), "file11", nullptr );
    verifyFile( children.at( 2 ), "file2",  itemFromOldSession );
    verifyFile( children.at( 3 ), "file3",  nullptr );

    removeSpy.check( model.indexForItem( secondDirectory ), 1, 1 );
    insertSpy.check( model.indexForItem( secondDirectory ), 1, 3 );
}

#include "moc_k3bdataprojectmodeltest.cpp"
