/*
 * Copyright (C) 2016 - 2017 Leslie Zhai <lesliezhai@llvm.org.cn>
 *
 * This file is part of the K3b project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include <QDebug>
#include <QFile>
#include <QString>

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv) 
{
    qDebug() << "DEBUG:" << __PRETTY_FUNCTION__ << *argc;
    qDebug() << "DEBUG:" << __PRETTY_FUNCTION__ << *argv[0];
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) 
{
    // QTBUG-57553
    qDebug() << "DEBUG:" << __PRETTY_FUNCTION__
             << QFile::encodeName(QString::fromRawData((const QChar *)Data, Size));
    return 0;
}
