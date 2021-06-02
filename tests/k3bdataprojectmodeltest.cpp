/*

    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bdataprojectmodeltest.h"
#include "k3bdatadoc.h"
#include "k3bdataprojectmodel.h"
#include "k3bdataitem.h"
#include "k3bdiritem.h"
#include "k3bspecialdataitem.h"
#include "k3btestutils.h"

#include <QSignalSpy>
#include <QTest>

QTEST_GUILESS_MAIN( DataProjectModelTest )

Q_DECLARE_METATYPE( QModelIndex )

DataProjectModelTest::DataProjectModelTest()
{
    qRegisterMetaType<QModelIndex>();
}


void DataProjectModelTest::init()
{
    m_doc = new K3b::DataDoc;
    m_doc->newDocument();
    m_doc->root()->addDataItem( new K3b::DirItem( "First directory" ) ); // index 0
    m_doc->root()->addDataItem( new K3b::SpecialDataItem( 1024, "file1" ) ); // index 1
    m_doc->root()->addDataItem( new K3b::SpecialDataItem( 2048, "file2" ) ); // index 2
    K3b::DirItem* secondDirectory = new K3b::DirItem( "Second directory" );
    m_doc->root()->addDataItem( secondDirectory ); // index 3
    secondDirectory->addDataItem( new K3b::SpecialDataItem( 1024, "file1" ) ); // index 3 -> 0
    secondDirectory->addDataItem( new K3b::SpecialDataItem( 512, "file2" ) ); // index 3 -> 1
    m_doc->root()->addDataItem( new K3b::SpecialDataItem( 300, "file3" ) ); // index 4
    m_doc->root()->addDataItem( new K3b::SpecialDataItem( 400, "file4" ) ); // index 5
}


void DataProjectModelTest::cleanp()
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


