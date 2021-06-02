/*

    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef K3B_META_ITEM_MODEL_TEST_H
#define K3B_META_ITEM_MODEL_TEST_H

#include <QObject>
#include <QPointer>

class QStringListModel;
class QStandardItemModel;

class MetaItemModelTest : public QObject
{
    Q_OBJECT
    
public:
    MetaItemModelTest();
    
private slots:
    void init(); // executed before each test function
    void testCreate();
    void testAddSubModel();
    void testAddFlatSubModel();
    void testRemoveSubModel();
    void testDynamicChanges();
    void testDynamicChangesInFlatModel();
    void testDataChanges();
    
private:
    QPointer<QStandardItemModel> m_listModel;
    QPointer<QStandardItemModel> m_treeModel;
};

#endif // K3B_META_ITEM_MODEL_TEST_H
