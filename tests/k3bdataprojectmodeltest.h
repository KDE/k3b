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

#ifndef K3B_DATA_PROJECT_MODEL_TEST_H
#define K3B_DATA_PROJECT_MODEL_TEST_H

#include <QtCore/QObject>
#include <QtCore/QPointer>

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
