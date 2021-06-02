/*

    SPDX-FileCopyrightText: 2004 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/


#ifndef _K3B_VIDEODVD_IMAGER_H_
#define _K3B_VIDEODVD_IMAGER_H_


#include "k3bisoimager.h"

#include <QScopedPointer>

namespace K3b {
    class VideoDvdDoc;

    /**
     * Create VideoDVD images with mkisofs. The difference
     * to the IsoImager is the -dvd-video option and the fact
     * that all VIDEO_TS files need to be in one local folder since
     * otherwise mkisofs is not able to find the dvd structures.
     */
    class VideoDvdImager : public IsoImager
    {
        Q_OBJECT

    public:
        VideoDvdImager( VideoDvdDoc* doc, JobHandler*, QObject* parent = 0 );
        ~VideoDvdImager() override;

    public Q_SLOTS:
        void start() override;
        void init() override;
        void calculateSize() override;

    protected:
        bool addMkisofsParameters( bool printSize = false ) override;
        int writePathSpec() override;
        void cleanup() override;
        int writePathSpecForDir( DirItem* dirItem, QTextStream& stream ) override;

    protected Q_SLOTS:
        void slotReceivedStderr( const QString& ) override;

    private:
        void fixVideoDVDSettings();

        class Private;
        QScopedPointer<Private> d;
    };
}

#endif
