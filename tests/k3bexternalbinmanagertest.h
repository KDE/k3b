/*
    SPDX-FileCopyrightText: 2016-2017 Leslie Zhai <lesliezhai@llvm.org.cn>

    This file is part of the K3b project.

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef K3B_EXTERNAL_BINMANAGER_TEST_H
#define K3B_EXTERNAL_BINMANAGER_TEST_H

#include <QObject>

#include "k3bapplication.h"

class ExternalBinManagerTest : public QObject
{
    Q_OBJECT

public:
    ExternalBinManagerTest();

private Q_SLOTS:
    void testBinObject();
    void testMyBurnJob();

private:
    K3b::Application::Core *m_core;
};

#endif // K3B_EXTERNAL_BINMANAGER_TEST_H
