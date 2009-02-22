/*
 *
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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


#ifndef _K3B_VIDEODVD_IMAGER_H_
#define _K3B_VIDEODVD_IMAGER_H_


#include <k3bisoimager.h>

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
        virtual ~VideoDvdImager();

    public Q_SLOTS:
        virtual void start();
        virtual void init();
        virtual void calculateSize();

    protected:
        bool addMkisofsParameters( bool printSize = false );
        int writePathSpec();
        void cleanup();
        int writePathSpecForDir( DirItem* dirItem, QTextStream& stream );

    protected Q_SLOTS:
        virtual void slotReceivedStderr( const QString& );

    private:
        void fixVideoDVDSettings();

        class Private;
        Private* d;
    };
}

#endif
