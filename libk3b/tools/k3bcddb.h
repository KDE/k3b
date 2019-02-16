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

#include <KJob>

#include <KCddb/Kcddb>
#include <KCddb/Cdinfo>

#include "k3b_export.h"

class QWidget;

namespace K3b {
    namespace Device {
        class Toc;
    }

    class Medium;

    namespace CDDB {
        namespace MultiEntriesDialog {
             LIBK3B_EXPORT int selectCddbEntry( const KCDDB::CDInfoList& entries, QWidget* parent = nullptr );
        }


        class LIBK3B_EXPORT CDDBJob : public KJob
        {
            Q_OBJECT

        public:
            explicit CDDBJob( QObject* parent = 0 );
            ~CDDBJob() override;

            /**
             * The medium specified in queryCddb. MediaCache
             * uses it to remember the original medium.
             */
            K3b::Medium medium() const;

            /**
             * Only valid after the job finished successfully.
             */
            KCDDB::CDInfo cddbResult() const;

            /**
             * Query cddb for the medium. The returned job is
             * already started.
             */
            static CDDBJob* queryCddb( const Medium& medium );

            /**
             * Query cddb for the toc. The returned job is
             * already started.
             */
            static CDDBJob* queryCddb( const Device::Toc& toc );

        public Q_SLOTS:
            void start() override;

        private:
            class Private;
            Private* const d;

            Q_PRIVATE_SLOT( d, void _k_cddbQueryFinished( KCDDB::Result ) )
        };

        LIBK3B_EXPORT KCDDB::TrackOffsetList createTrackOffsetList( const K3b::Device::Toc& toc );
    }
}

#endif
