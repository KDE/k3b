/*
    SPDX-FileCopyrightText: 2003 Christian Kvasny <chris@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef K3BVIDEOCDINFO_H
#define K3BVIDEOCDINFO_H

#include <QList>
#include <QObject>
#include <QProcess>
#include <QString>

namespace K3b {

class Process;

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

class VideoCdInfo : public QObject
{
        Q_OBJECT

    public:
        explicit VideoCdInfo( QObject* parent = 0 );
        ~VideoCdInfo() override;

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

} // namespace K3b

#endif
