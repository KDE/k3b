/*
    This file is part of the KDE libraries

    SPDX-FileCopyrightText: 2007 Oswald Buddenhagen <ossi@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPROCESS_P_H
#define KPROCESS_P_H

#include "k3bkprocess.h"

class K3bKProcessPrivate {
    Q_DECLARE_PUBLIC(K3bKProcess)
protected:
    K3bKProcessPrivate() :
        openMode(QIODevice::ReadWrite)
    {
    }
    void writeAll(const QByteArray &buf, int fd);
    void forwardStd(::QProcess::ProcessChannel good, int fd);
    void _k_forwardStdout();
    void _k_forwardStderr();

    QString prog;
    QStringList args;
    KProcess::OutputChannelMode outputChannelMode;
    QIODevice::OpenMode openMode;

    K3bKProcess *q_ptr;
};

#endif
