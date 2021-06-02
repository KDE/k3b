//
/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004 Nicolas GOUTTE <goutte@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOSTORE_BASE_H
#define KOSTORE_BASE_H

#include <QUrl>

#include "KoStore.h"

/**
 * Helper class for KoStore (mainly for remote file support)
 */
class KoStoreBase : public KoStore
{
public:
    KoStoreBase();
    ~KoStoreBase(void) override;
public:
    enum FileMode { /*Bad=0,*/ Local=1, RemoteRead, RemoteWrite };

protected:
    /**
     * original URL of the remote file
     * (undefined for a local file)
     */
    QUrl m_url;
    FileMode m_fileMode;
    QString m_localFileName;
    QWidget* m_window;
};

#endif //KOSTORE_BASE_H
