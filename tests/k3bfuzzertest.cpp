/*
    SPDX-FileCopyrightText: 2016-2017 Leslie Zhai <lesliezhai@llvm.org.cn>

    This file is part of the K3b project.

    SPDX-License-Identifier: GPL-2.0-or-later
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
    // QTBUG-57553, KDEBUG-391610
    qDebug() << "DEBUG:" << __PRETTY_FUNCTION__
             << QFile::encodeName(QString::fromRawData((const QChar *)Data, Size));
    // KDEBUG-384750
    QStringList lines("xorriso : UPDATE :  0,52\% done, estimate finish Mon Sep 25 11:04:34 2017");
    lines << "xorriso : UPDATE :  0.52\% done, estimate finish Mon Sep 25 11:04:34 2017";
    lines << "0.52\% done, estimate finish Mon Sep 25 11:04:34 2017";
    for (QString line : lines) {
        QString perStr = line;
        perStr.truncate(perStr.indexOf('%'));
        QRegExp rx("(\\d+.|,+\\d)");
        QStringList list;
        int pos = 0;
        while ((pos = rx.indexIn(perStr, pos)) != -1) {
            list << rx.cap(1);
            pos += rx.matchedLength();
        }
        if (list.size() > 1)
            qDebug() << "DEBUG:" << __PRETTY_FUNCTION__ << list[0].replace(',', '.') + list[1];
    }
    return 0;
}
