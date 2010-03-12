/*
*
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
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


#ifndef K3BVIDEOCDINFO_H
#define K3BVIDEOCDINFO_H

#include <qstring.h>
#include <qstringlist.h>
#include <qobject.h>
#include <QList>
#include <QProcess>

#include "k3btoc.h"
#include "k3bcore.h"

namespace K3b {
    class Process;
}

namespace K3b {
class VideoCdInfoResultEntry
{
    public:
        VideoCdInfoResultEntry() : name(), id()
        {}

        VideoCdInfoResultEntry( const QString& name, const QString& id )
                : name( name ), id( id )
        {}

        QString name;
        QString id;

        long size;
};
}

namespace K3b {
class VideoCdInfoResult
{
    public:
        VideoCdInfoResult()
        {}

        enum type {NONE = 0, FILE, SEGMENT, SEQUENCE};

        void addEntry( const VideoCdInfoResultEntry& = VideoCdInfoResultEntry(), int type = VideoCdInfoResult::SEQUENCE );
        const VideoCdInfoResultEntry& entry( int number = 0 , int type = VideoCdInfoResult::SEQUENCE ) const;
        int foundEntries( int type = VideoCdInfoResult::SEQUENCE ) const;

        QString volumeId;
        QString type;
        QString version;

        QString xmlData;

    private:
        QList<VideoCdInfoResultEntry> m_fileEntry;
        QList<VideoCdInfoResultEntry> m_segmentEntry;
        QList<VideoCdInfoResultEntry> m_sequenceEntry;

        VideoCdInfoResultEntry m_emptyEntry;
};
}

namespace K3b {
class VideoCdInfo : public QObject
{
        Q_OBJECT

    public:
        VideoCdInfo( QObject* parent = 0 );
        ~VideoCdInfo();

        /**
         * Do NOT call this before queryResult has
         * been emitted
         */
        const VideoCdInfoResult& result() const;

        void info( const QString& );

     Q_SIGNALS:
        void infoFinished( bool success );

    private Q_SLOTS:
        void slotInfoFinished( int exitCode, QProcess::ExitStatus exitStatus );
        void slotParseOutput( const QString& inp );

    private:
        void cancelAll();

        VideoCdInfoResult m_Result;
        void parseXmlData();

        Process* m_process;

        QString m_xmlData;
        bool m_isXml;

};
}

#endif
