/*
    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3B_DATA_PROJECT_MODEL_TEST_H
#define K3B_DATA_PROJECT_MODEL_TEST_H

#include <QObject>
#include <QPointer>

namespace K3b {
    class DataDoc;
    class Iso9660;
} // namespace K3b

class DataProjectModelTest : public QObject
{
    Q_OBJECT

public:
    DataProjectModelTest();
    ~DataProjectModelTest() override;

private slots:
    void init(); // executed before each test function
    void cleanup(); // executed after each test function
    void testCreate();
    void testAdd();
    void testRemove();
    void testReplace();
    void testAddAndReplace();

private:
    QPointer<K3b::DataDoc> m_doc;
    K3b::Iso9660* m_iso;
};

#endif // K3B_DATA_PROJECT_MODEL_TEST_H
