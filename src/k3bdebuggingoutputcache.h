/*
 *
 * $Id$
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
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
