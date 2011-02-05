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

#include <QtCore/QPointer>
#include <QtCore/QStringList>
#include <QtGui/QStandardItemModel>
#include <QtGui/QStringListModel>
#include <QtTest/QTest>

QTEST_KDEMAIN_CORE( MetaItemModelTest )

MetaItemModelTest::MetaItemModelTest()
:
    stringListModel_( new QStringListModel( this ) ),
    standardItemModel_( new QStandardItemModel( this ) )
{
    QStringList stringList;
    stringList.push_back( "a" );
    stringList.push_back( "b" );
    stringList.push_back( "c" );
    stringList.push_back( "d" );
    stringListModel_->setStringList( stringList );
    
    standardItemModel_->appendRow( new QStandardItem( KIcon( "edit-copy" ), "copy" ) );
    standardItemModel_->appendRow( new QStandardItem( KIcon( "edit-paste" ), "paste" ) );
    standardItemModel_->item( 0 )->appendRow( new QStandardItem( KIcon( "edit-cut" ), "cut" ) );
}

void MetaItemModelTest::testCreate()
{
    QPointer<K3b::MetaItemModel>( new K3b::MetaItemModel( this ) );
}

void MetaItemModelTest::testAddSubModel()
{
    QPointer<K3b::MetaItemModel> model( new K3b::MetaItemModel( this ) );
    model->addSubModel( "stringList", KIcon( "configure" ), stringListModel_ );
    model->addSubModel( "standard", KIcon( "go-previous" ), standardItemModel_ );
    
    QCOMPARE( 2, model->rowCount() );
    
    QModelIndex firstIndex = model->index( 0, 0 );
    QVERIFY( firstIndex.isValid() );
    QCOMPARE( 4, model->rowCount( firstIndex ) );
    
    QModelIndex secondIndex = model->index( 1, 0 );
    QVERIFY( secondIndex.isValid() );
    QCOMPARE( 2, model->rowCount( secondIndex ) );
}

void MetaItemModelTest::testAddFlatSubModel()
{
    QPointer<K3b::MetaItemModel> model( new K3b::MetaItemModel( this ) );
    model->addSubModel( "stringList", KIcon( "configure" ), stringListModel_, true );
    model->addSubModel( "standard", KIcon( "go-previous" ), standardItemModel_, true );
    
    QCOMPARE( 6, model->rowCount() );
    
    QModelIndex fifthIndex = model->index( 4, 0 );
    QVERIFY( fifthIndex.isValid() );
    QCOMPARE( 1, model->rowCount( fifthIndex ) );
}
