/*
 *
 * Copyright (C) 2008-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_CDDB_H_
#define _K3B_CDDB_H_

#include <KDialog>
#include <KJob>

#include <libkcddb/kcddb.h>
#include <libkcddb/cdinfo.h>

#include "k3b_export.h"


namespace K3bDevice {
    class Toc;
}
class QListWidget;
class K3bMedium;


namespace K3bCDDB {
    class LIBK3B_EXPORT MultiEntriesDialog : public KDialog
    {
        Q_OBJECT

    public:
        ~MultiEntriesDialog();

        static int selectCddbEntry( const KCDDB::CDInfoList& entries, QWidget* parent = 0 );

    protected:
        MultiEntriesDialog( QWidget* parent = 0 );

    private:
        QListWidget* m_listBox;
    };
    

    class LIBK3B_EXPORT CDDBJob : public KJob
    {
        Q_OBJECT

    public:
        CDDBJob( QObject* parent = 0 );
        ~CDDBJob();

        /**
         * The medium specified in queryCddb. K3bMediaCache
         * uses it to remember the original medium.
         */
        K3bMedium medium() const;

        /**
         * Only valid after the job finished successfully.
         */
        KCDDB::CDInfo cddbResult() const;

        /**
         * Query cddb for the medium. The returned job is
         * already started.
         */
        static CDDBJob* queryCddb( const K3bMedium& medium );

        /**
         * Query cddb for the toc. The returned job is
         * already started.
         */
        static CDDBJob* queryCddb( const K3bDevice::Toc& toc );

    public Q_SLOTS:
        void start();

    private:
        class Private;
        Private* const d;

        Q_PRIVATE_SLOT( d, void _k_cddbQueryFinished( KCDDB::Result ) )
    };


    LIBK3B_EXPORT KCDDB::TrackOffsetList createTrackOffsetList( const K3bDevice::Toc& toc );
}

#endif
