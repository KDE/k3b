/*

    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3B_DATA_PROJECT_MODEL_TEST_H
#define K3B_DATA_PROJECT_MODEL_TEST_H

#include <QObject>
#include <QPointer>

namespace K3b { class DataDoc; }

class DataProjectModelTest : public QObject
{
    Q_OBJECT

public:
    DataProjectModelTest();

private slots:
    void init(); // executed before each test function
    void cleanp(); // executed after each test function
    void testCreate();
    void testAdd();
    void testRemove();

private:
    QPointer<K3b::DataDoc> m_doc;
};

#endif // K3B_DATA_PROJECT_MODEL_TEST_H
