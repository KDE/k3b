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
//Added by qt3to4:
#include <QList>

#include <k3btoc.h>
#include <k3bcore.h>

class K3bProcess;
class K3Process;

class K3bVideoCdInfoResultEntry
{
    public:
        K3bVideoCdInfoResultEntry() : name( 0 ), id( 0 )
        {}

        K3bVideoCdInfoResultEntry( const QString& name, const QString& id )
                : name( name ), id( id )
        {}

        QString name;
        QString id;

        long size;
};

class K3bVideoCdInfoResult
{
    public:
        K3bVideoCdInfoResult()
        {}

        enum type {NONE = 0, FILE, SEGMENT, SEQUENCE};

        void addEntry( const K3bVideoCdInfoResultEntry& = K3bVideoCdInfoResultEntry(), int type = K3bVideoCdInfoResult::SEQUENCE );
        const K3bVideoCdInfoResultEntry& entry( int number = 0 , int type = K3bVideoCdInfoResult::SEQUENCE ) const;
        int foundEntries( int type = K3bVideoCdInfoResult::SEQUENCE ) const;

        QString volumeId;
        QString type;
        QString version;

        QString xmlData;

    private:
        QList<K3bVideoCdInfoResultEntry> m_fileEntry;
        QList<K3bVideoCdInfoResultEntry> m_segmentEntry;
        QList<K3bVideoCdInfoResultEntry> m_sequenceEntry;

        K3bVideoCdInfoResultEntry m_emptyEntry;
};

class K3bVideoCdInfo : public QObject
{
        Q_OBJECT

    public:
        K3bVideoCdInfo( QObject* parent = 0 );
        ~K3bVideoCdInfo();

        /**
         * Do NOT call this before queryResult has
         * been emitted
         */
        const K3bVideoCdInfoResult& result() const;

        void info( const QString& );

     Q_SIGNALS:
        void infoFinished( bool success );

    private Q_SLOTS:
        void slotInfoFinished();
        void slotParseOutput( K3Process*, char* output, int len );

    private:
        void cancelAll();

        K3bVideoCdInfoResult m_Result;
        void parseXmlData();

        K3bProcess* m_process;

        QString m_xmlData;
        bool m_isXml;

};

#endif
