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

#include "k3bmetaitemmodeltest.h"
#include "k3bmetaitemmodel.h"

#include <KIcon>
#include <qtest_kde.h>

#include <QtCore/QStringList>
#include <QtGui/QStandardItemModel>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

QTEST_KDEMAIN_CORE( MetaItemModelTest )

Q_DECLARE_METATYPE( QModelIndex )

namespace
{
    
    void checkForIdenticalData(
        QAbstractItemModel const& model1, QAbstractItemModel const& model2,
        QModelIndex root1 = QModelIndex(), QModelIndex root2 = QModelIndex() )
    {
        QCOMPARE( model1.rowCount( root1 ), model2.rowCount( root2 ) );
        
        for( int row = 0; row < model1.rowCount( root1 ); ++row )
        {
            QCOMPARE( model1.columnCount( root1 ), model2.columnCount( root2 ) );
            
            for( int col = 0; col < model1.columnCount( root1 ); ++col )
            {
                const QModelIndex index1 = model1.index( row, col, root1 );
                const QModelIndex index2 = model2.index( row, col, root2 );
                QCOMPARE( model1.data( index1 ).toString(), model2.data( index2 ).toString() );
            }
            
            const QModelIndex newRoot1 = model1.index( row, 0, root1 );
            const QModelIndex newRoot2 = model2.index( row, 0, root2 );
            checkForIdenticalData( model1, model2, newRoot1, newRoot2 );
        }
    }
    
} // namespace


MetaItemModelTest::MetaItemModelTest()
{
    qRegisterMetaType<QModelIndex>();
}


void MetaItemModelTest::init()
{
    m_listModel = new QStandardItemModel( this );
    m_listModel->appendRow( new QStandardItem( "a" ) );
    m_listModel->appendRow( new QStandardItem( "b" ) );
    m_listModel->appendRow( new QStandardItem( "c" ) );
    m_listModel->appendRow( new QStandardItem( "d" ) );
    
    m_treeModel = new QStandardItemModel( this );
    m_treeModel->appendRow( new QStandardItem( KIcon( "edit-copy" ), "copy" ) );
    m_treeModel->appendRow( new QStandardItem( KIcon( "edit-paste" ), "paste" ) );
    m_treeModel->item( 0 )->appendRow( new QStandardItem( KIcon( "edit-cut" ), "cut" ) );
}


void MetaItemModelTest::testCreate()
{
    K3b::MetaItemModel model;
}


void MetaItemModelTest::testAddSubModel()
{
    K3b::MetaItemModel model;
    model.addSubModel( "stringList", KIcon( "configure" ), m_listModel );
    model.addSubModel( "standard", KIcon( "go-previous" ), m_treeModel );
    
    QCOMPARE( model.rowCount(), 2 );
    
    checkForIdenticalData( model, *m_listModel, model.index( 0, 0 ) );
    checkForIdenticalData( model, *m_treeModel, model.index( 1, 0 ) );
    
    QModelIndex firstIndex = model.index( 0, 0 );
    QVERIFY( firstIndex.isValid() );
    QCOMPARE( model.rowCount( firstIndex ), 4 );
    
    QModelIndex secondIndex = model.index( 1, 0 );
    QVERIFY( secondIndex.isValid() );
    QCOMPARE( model.rowCount( secondIndex ), 2 );
}


void MetaItemModelTest::testAddFlatSubModel()
{
    K3b::MetaItemModel model;
    model.addSubModel( "stringList", KIcon( "configure" ), m_listModel, true );
    model.addSubModel( "standard", KIcon( "go-previous" ), m_treeModel, true );
    
    QCOMPARE( model.rowCount(), 6 );
    
    QModelIndex fifthIndex = model.index( 4, 0 );
    QVERIFY( fifthIndex.isValid() );
    QCOMPARE( model.rowCount( fifthIndex ), 1 );
}


void MetaItemModelTest::testRemoveSubModel()
{
    K3b::MetaItemModel model;
    model.addSubModel( "stringList", KIcon( "configure" ), m_listModel );
    model.addSubModel( "standard", KIcon( "go-previous" ), m_treeModel, true );
    
    checkForIdenticalData( model, *m_listModel, model.index( 0, 0 ) );
    
    QCOMPARE( model.rowCount(), 3 );
    
    model.removeSubModel( m_listModel );
    QCOMPARE( model.rowCount(), 2 );
    
    model.removeSubModel( m_treeModel );
    QCOMPARE( model.rowCount(), 0 );
}


void MetaItemModelTest::testDynamicChanges()
{
    K3b::MetaItemModel model;
    model.addSubModel( "stringList", KIcon( "configure" ), m_listModel );
    
    QSignalSpy rowsAboutToBeRemovedSpy( &model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)) );
    QSignalSpy rowsRemovedSpy( &model, SIGNAL(rowsRemoved(QModelIndex,int,int)) );
    QList<QVariant> arguments;
    
    QModelIndex firstIndex = model.index( 0, 0 );
    
    QCOMPARE( model.rowCount(), 1 );
    QCOMPARE( model.rowCount( firstIndex ), 4 ); 
    checkForIdenticalData( model, *m_listModel, firstIndex );
    
    m_listModel->removeRow( 2 );
    QCOMPARE( rowsAboutToBeRemovedSpy.count(), 1 );
    arguments = rowsAboutToBeRemovedSpy.takeFirst();
    QCOMPARE( arguments.count(), 3 );
    QCOMPARE( arguments.at( 0 ).value<QModelIndex>(), firstIndex );
    QCOMPARE( arguments.at( 1 ).value<int>(), 2 );
    QCOMPARE( arguments.at( 2 ).value<int>(), 2 );
    QCOMPARE( rowsRemovedSpy.count(), 1 );
    arguments = rowsRemovedSpy.takeFirst();
    QCOMPARE( arguments.count(), 3 );
    QCOMPARE( arguments.at( 0 ).value<QModelIndex>(), firstIndex );
    QCOMPARE( arguments.at( 1 ).value<int>(), 2 );
    QCOMPARE( arguments.at( 2 ).value<int>(), 2 );
    QCOMPARE( model.rowCount( firstIndex ), 3 );
    checkForIdenticalData( model, *m_listModel, firstIndex );
    
    model.addSubModel( "standard", KIcon( "go-previous" ), m_treeModel );
    QModelIndex secondIndex = model.index( 1, 0 );
    
    QCOMPARE( model.rowCount(), 2 );
    QCOMPARE( model.rowCount( secondIndex ), 2 );
    checkForIdenticalData( model, *m_listModel, firstIndex );
    
    m_treeModel->removeRow( 1 );
    QCOMPARE( rowsAboutToBeRemovedSpy.count(), 1 );
    arguments = rowsAboutToBeRemovedSpy.takeFirst();
    QCOMPARE( arguments.count(), 3 );
    QCOMPARE( arguments.at( 0 ).value<QModelIndex>(), secondIndex );
    QCOMPARE( arguments.at( 1 ).value<int>(), 1 );
    QCOMPARE( arguments.at( 2 ).value<int>(), 1 );
    QCOMPARE( rowsRemovedSpy.count(), 1 );
    arguments = rowsRemovedSpy.takeFirst();
    QCOMPARE( arguments.count(), 3 );
    QCOMPARE( arguments.at( 0 ).value<QModelIndex>(), secondIndex );
    QCOMPARE( arguments.at( 1 ).value<int>(), 1 );
    QCOMPARE( arguments.at( 2 ).value<int>(), 1 );
    QCOMPARE( model.rowCount( secondIndex ), 1 );
    
    model.removeSubModel( m_listModel );
    QCOMPARE( model.rowCount(), 1 );
}


void MetaItemModelTest::testDynamicChangesInFlatModel()
{
    K3b::MetaItemModel model;
    model.addSubModel( "stringList", KIcon( "configure" ), m_listModel, true );
    QCOMPARE( model.rowCount(), 4 );
    checkForIdenticalData( model, *m_listModel );
    
    QSignalSpy rowsAboutToBeRemovedSpy( &model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)) );
    QSignalSpy rowsRemovedSpy( &model, SIGNAL(rowsRemoved(QModelIndex,int,int)) );
    QList<QVariant> arguments;
    
    m_listModel->removeRow( 2 );
    QCOMPARE( rowsAboutToBeRemovedSpy.count(), 1 );
    arguments = rowsAboutToBeRemovedSpy.takeFirst();
    QCOMPARE( arguments.count(), 3 );
    QCOMPARE( arguments.at( 0 ).value<QModelIndex>(), QModelIndex() );
    QCOMPARE( arguments.at( 1 ).value<int>(), 2 );
    QCOMPARE( arguments.at( 2 ).value<int>(), 2 );
    QCOMPARE( rowsRemovedSpy.count(), 1 );
    arguments = rowsRemovedSpy.takeFirst();
    QCOMPARE( arguments.count(), 3 );
    QCOMPARE( arguments.at( 0 ).value<QModelIndex>(), QModelIndex() );
    QCOMPARE( arguments.at( 1 ).value<int>(), 2 );
    QCOMPARE( arguments.at( 2 ).value<int>(), 2 );
    QCOMPARE( model.rowCount(), 3 );
    checkForIdenticalData( model, *m_listModel );
    
    model.addSubModel( "standard", KIcon( "go-previous" ), m_treeModel, true );
    QCOMPARE( model.rowCount(), 5 );
    
    m_treeModel->removeRow( 1 );
    QCOMPARE( rowsAboutToBeRemovedSpy.count(), 1 );
    arguments = rowsAboutToBeRemovedSpy.takeFirst();
    QCOMPARE( arguments.count(), 3 );
    QCOMPARE( arguments.at( 0 ).value<QModelIndex>(), QModelIndex() );
    QCOMPARE( arguments.at( 1 ).value<int>(), 4 );
    QCOMPARE( arguments.at( 2 ).value<int>(), 4 );
    QCOMPARE( rowsRemovedSpy.count(), 1 );
    arguments = rowsRemovedSpy.takeFirst();
    QCOMPARE( arguments.count(), 3 );
    QCOMPARE( arguments.at( 0 ).value<QModelIndex>(), QModelIndex() );
    QCOMPARE( arguments.at( 1 ).value<int>(), 4 );
    QCOMPARE( arguments.at( 2 ).value<int>(), 4 );
    QCOMPARE( model.rowCount(), 4 );
    
    model.removeSubModel( m_listModel );
    QCOMPARE( model.rowCount(), 1 );
}


void MetaItemModelTest::testDataChanges()
{
    K3b::MetaItemModel model;
    model.addSubModel( "standard", KIcon( "go-previous" ), m_treeModel, true );
    checkForIdenticalData( model, *m_treeModel );
    
    QSignalSpy dataChangedSpy( &model, SIGNAL(dataChanged(QModelIndex,QModelIndex)) );
    m_treeModel->setData( m_treeModel->index( 1, 0 ), "somethingElse" );
    checkForIdenticalData( model, *m_treeModel );
    
    // Check if dataChanged() signal has been emitted
    QCOMPARE( dataChangedSpy.count(), 1 );
    QList<QVariant> arguments = dataChangedSpy.takeFirst();
    QCOMPARE( arguments.count(), 2 );
    
    QCOMPARE( arguments.at( 0 ).value<QModelIndex>(), model.index( 1, 0 ) );
    QCOMPARE( arguments.at( 1 ).value<QModelIndex>(), model.index( 1, 0 ) );
}


#include "k3bmetaitemmodeltest.moc"
