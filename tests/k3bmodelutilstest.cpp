/*

    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#include "k3bmodelutilstest.h"
#include "k3bmodelutils.h"

#include <QStandardItemModel>
#include <QTest>

QTEST_GUILESS_MAIN( ModelUtilsTest )

ModelUtilsTest::ModelUtilsTest()
{
}

void ModelUtilsTest::testCommonCheckState()
{
    QStandardItemModel* listModel = new QStandardItemModel( this );
    listModel->appendRow( new QStandardItem );
    listModel->appendRow( new QStandardItem );
    listModel->appendRow( new QStandardItem );
    listModel->appendRow( new QStandardItem );
    
    QModelIndexList indexes;
    for( int i = 0; i < listModel->rowCount(); ++i )
        indexes.push_back( listModel->index( i, 0 ) );
    
    QCOMPARE( K3b::ModelUtils::commonCheckState( indexes ), Qt::Unchecked );
    
    listModel->item( 2 )->setCheckState( Qt::Checked );
    QCOMPARE( K3b::ModelUtils::commonCheckState( indexes ), Qt::PartiallyChecked );
    
    listModel->item( 0 )->setCheckState( Qt::Checked );
    listModel->item( 1 )->setCheckState( Qt::Checked );
    listModel->item( 3 )->setCheckState( Qt::Checked );
    QCOMPARE( K3b::ModelUtils::commonCheckState( indexes ), Qt::Checked );
}

void ModelUtilsTest::testToggleCommonCheckState()
{
    QStandardItemModel* listModel = new QStandardItemModel( this );
    listModel->appendRow( new QStandardItem );
    listModel->appendRow( new QStandardItem );
    listModel->appendRow( new QStandardItem );
    listModel->appendRow( new QStandardItem );
    
    for( int i = 0; i < listModel->rowCount(); ++i )
        QCOMPARE( listModel->item( i )->checkState(), Qt::Unchecked );
    
    QModelIndexList indexes;
    for( int i = 0; i < listModel->rowCount(); ++i )
        indexes.push_back( listModel->index( i, 0 ) );
    
    K3b::ModelUtils::toggleCommonCheckState( listModel, indexes );
    for( int i = 0; i < listModel->rowCount(); ++i )
        QCOMPARE( listModel->item( i )->checkState(), Qt::Checked );
    
    K3b::ModelUtils::toggleCommonCheckState( listModel, indexes );
    for( int i = 0; i < listModel->rowCount(); ++i )
        QCOMPARE( listModel->item( i )->checkState(), Qt::Unchecked );
    
    listModel->item( 2 )->setCheckState( Qt::Checked );
    K3b::ModelUtils::toggleCommonCheckState( listModel, indexes );
    for( int i = 0; i < listModel->rowCount(); ++i )
        QCOMPARE( listModel->item( i )->checkState(), Qt::Checked );
    
    K3b::ModelUtils::toggleCommonCheckState( listModel, indexes );
    for( int i = 0; i < listModel->rowCount(); ++i )
        QCOMPARE( listModel->item( i )->checkState(), Qt::Unchecked );
    
    listModel->item( 2 )->setCheckState( Qt::PartiallyChecked );
    K3b::ModelUtils::toggleCommonCheckState( listModel, indexes );
    for( int i = 0; i < listModel->rowCount(); ++i )
        QCOMPARE( listModel->item( i )->checkState(), Qt::Checked );
}

void ModelUtilsTest::testCommonText()
{
    QStandardItemModel* listModel = new QStandardItemModel( this );
    listModel->appendRow( new QStandardItem );
    listModel->appendRow( new QStandardItem );
    listModel->appendRow( new QStandardItem );
    listModel->appendRow( new QStandardItem );
    
    QModelIndexList indexes;
    for( int i = 0; i < listModel->rowCount(); ++i )
        indexes.push_back( listModel->index( i, 0 ) );
    
    QCOMPARE( K3b::ModelUtils::commonText( indexes ), QString() );
    
    listModel->item( 2 )->setText( "a" );
    QCOMPARE( K3b::ModelUtils::commonText( indexes ), QString() );
    
    listModel->item( 0 )->setText( "a" );
    listModel->item( 1 )->setText( "a" );
    listModel->item( 3 )->setText( "a" );
    QCOMPARE( K3b::ModelUtils::commonText( indexes ), QString( "a" ) );
    
    listModel->item( 1 )->setText( "b" );
    QCOMPARE( K3b::ModelUtils::commonText( indexes ), QString() );
}

void ModelUtilsTest::testSetCommonText()
{
    QStandardItemModel* listModel = new QStandardItemModel( this );
    listModel->appendRow( new QStandardItem );
    listModel->appendRow( new QStandardItem );
    listModel->appendRow( new QStandardItem );
    listModel->appendRow( new QStandardItem );
    
    for( int i = 0; i < listModel->rowCount(); ++i )
        QCOMPARE( listModel->item( i )->text(), QString() );
    
    QModelIndexList indexes;
    for( int i = 0; i < listModel->rowCount(); ++i )
        indexes.push_back( listModel->index( i, 0 ) );
    
    K3b::ModelUtils::setCommonText( listModel, indexes, "a" );
    for( int i = 0; i < listModel->rowCount(); ++i )
        QCOMPARE( listModel->item( i )->text(), QString( "a" ) );
    
    K3b::ModelUtils::setCommonText( listModel, indexes, QString() );
    for( int i = 0; i < listModel->rowCount(); ++i )
        QCOMPARE( listModel->item( i )->text(), QString( "a" ) );
    
    listModel->item( 2 )->setText( "b" );
    K3b::ModelUtils::setCommonText( listModel, indexes, QString() );
    QCOMPARE( listModel->item( 2 )->text(), QString( "b" ) );
}


