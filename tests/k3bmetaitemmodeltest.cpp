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
#include <QtGui/QStringListModel>
#include <QtTest/QTest>

QTEST_KDEMAIN_CORE( MetaItemModelTest )

void MetaItemModelTest::init()
{
    m_stringListModel = new QStringListModel( this );
    m_standardItemModel = new QStandardItemModel( this );
    
    QStringList stringList;
    stringList.push_back( "a" );
    stringList.push_back( "b" );
    stringList.push_back( "c" );
    stringList.push_back( "d" );
    m_stringListModel->setStringList( stringList );
    
    m_standardItemModel->appendRow( new QStandardItem( KIcon( "edit-copy" ), "copy" ) );
    m_standardItemModel->appendRow( new QStandardItem( KIcon( "edit-paste" ), "paste" ) );
    m_standardItemModel->item( 0 )->appendRow( new QStandardItem( KIcon( "edit-cut" ), "cut" ) );
}


void MetaItemModelTest::testCreate()
{
    K3b::MetaItemModel model;
}


void MetaItemModelTest::testAddSubModel()
{
    K3b::MetaItemModel model;
    model.addSubModel( "stringList", KIcon( "configure" ), m_stringListModel );
    model.addSubModel( "standard", KIcon( "go-previous" ), m_standardItemModel );
    
    QCOMPARE( model.rowCount(), 2 );
    
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
    model.addSubModel( "stringList", KIcon( "configure" ), m_stringListModel, true );
    model.addSubModel( "standard", KIcon( "go-previous" ), m_standardItemModel, true );
    
    QCOMPARE( model.rowCount(), 6 );
    
    QModelIndex fifthIndex = model.index( 4, 0 );
    QVERIFY( fifthIndex.isValid() );
    QCOMPARE( model.rowCount( fifthIndex ), 1 );
}


void MetaItemModelTest::testRemoveSubModel()
{
    K3b::MetaItemModel model;
    model.addSubModel( "stringList", KIcon( "configure" ), m_stringListModel );
    model.addSubModel( "standard", KIcon( "go-previous" ), m_standardItemModel, true );
    QCOMPARE( model.rowCount(), 3 );
    
    model.removeSubModel( m_stringListModel );
    QCOMPARE( model.rowCount(), 2 );
    
    model.removeSubModel( m_standardItemModel );
    QCOMPARE( model.rowCount(), 0 );
}


void MetaItemModelTest::testDynamicChanges()
{
    K3b::MetaItemModel model;
    model.addSubModel( "stringList", KIcon( "configure" ), m_stringListModel );
    QModelIndex firstIndex = model.index( 0, 0 );
    
    QCOMPARE( model.rowCount(), 1 );
    QCOMPARE( model.rowCount( firstIndex ), 4 ); 
    
    m_stringListModel->removeRow( 2 );
    QCOMPARE( model.rowCount( firstIndex ), 3 );
    
    model.addSubModel( "standard", KIcon( "go-previous" ), m_standardItemModel );
    QModelIndex secondIndex = model.index( 1, 0 );
    
    QCOMPARE( model.rowCount(), 2 );
    QCOMPARE( model.rowCount( secondIndex ), 2 );
    
    m_standardItemModel->removeRow( 1 );
    QCOMPARE( model.rowCount( secondIndex ), 1 );
    
    model.removeSubModel( m_stringListModel );
    QCOMPARE( model.rowCount(), 1 );
}


void MetaItemModelTest::testDynamicChangesInFlatModel()
{
    K3b::MetaItemModel model;
    model.addSubModel( "stringList", KIcon( "configure" ), m_stringListModel, true );
    QCOMPARE( model.rowCount(), 4 );
    
    m_stringListModel->removeRow( 2 );
    QCOMPARE( model.rowCount(), 3 );
    
    model.addSubModel( "standard", KIcon( "go-previous" ), m_standardItemModel, true );
    QCOMPARE( model.rowCount(), 5 );
    
    m_standardItemModel->removeRow( 1 );
    QCOMPARE( model.rowCount(), 4 );
    
    model.removeSubModel( m_stringListModel );
    QCOMPARE( model.rowCount(), 1 );
}


#include "k3bmetaitemmodeltest.moc"
