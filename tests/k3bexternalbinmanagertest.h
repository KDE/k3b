/*
 * Copyright (C) 2016 Leslie Zhai <xiangzhai83@gmail.com>
 *
 * This file is part of the K3b project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
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
