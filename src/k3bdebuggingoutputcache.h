/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_DEBUGGING_OUTPUT_CACHE_H_
#define _K3B_DEBUGGING_OUTPUT_CACHE_H_

#include <QMap>
#include <QString>


namespace K3b {
    /**
     * Class to cache the debug output and make sure we do not eat all the
     * memory by restricting the memory used and ignoring multiple identical
     * messages.
     */
    class DebuggingOutputCache
    {
    public:
        DebuggingOutputCache();
        ~DebuggingOutputCache();

        void addOutput( const QString& group, const QString& line );

        DebuggingOutputCache& operator<<( const QString& line );

        void clear();

        QString toString() const;
        QMap<QString, QString> toGroups() const;

        bool stderrEnabled() const;
        void enableStderr( bool b );

        static QString defaultGroup();

    private:
        class Private;
        Private* const d;
    };
}

#endif
