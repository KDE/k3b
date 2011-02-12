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

#ifndef K3B_META_ITEM_MODEL_TEST_H
#define K3B_META_ITEM_MODEL_TEST_H

#include <QtCore/QObject>
#include <QtCore/QPointer>

class QStringListModel;
class QStandardItemModel;

class MetaItemModelTest : public QObject
{
    Q_OBJECT
    
private slots:
    void init(); // executed before each test function
    void testCreate();
    void testAddSubModel();
    void testAddFlatSubModel();
    void testRemoveSubModel();
    void testDynamicChanges();
    void testDynamicChangesInFlatModel();
    
private:
    QPointer<QStringListModel> m_stringListModel;
    QPointer<QStandardItemModel> m_standardItemModel;
};

#endif // K3B_META_ITEM_MODEL_TEST_H
