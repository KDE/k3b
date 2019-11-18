/*
 *
 * Copyright (C) 2006-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_VIDEODVD_H_
#define _K3B_VIDEODVD_H_

#include "k3bvideodvdtitle.h"

#include "k3b_export.h"

#include <QString>
#include <QVector>


namespace K3b {
    namespace Device {
        class Device;
    }

    /**
     * The VideoDVD classes do not provide a complete playback frontend to
     * libdvdread but are merely intended for Video DVD analysis.
     *
     * They are title based and thus treat a Video DVD to be a set of titles.
     * Additional Video DVD constructs such as title sets, parts of titles (chapters),
     * program chains, or cells are not handled explicitly.
     *
     * The usage is very simple. One creates a VideoDVD instance and calls the open()
     * method with a device containing a Video DVD. If the method returns true the
     * analysis was successful and the structures are filled.
     *
     * After open() has returned the device has already been closed.
     */
    namespace VideoDVD
    {
        /**
         * libdvdread wrapper class
         */
        class LIBK3B_EXPORT VideoDVD
        {
        public:
            VideoDVD();
            ~VideoDVD();

            /**
             * \return true if a Video DVD was successfully opened via open()
             */
            bool valid() const;

            /**
             * Open a video dvd and parse it's contents. The device will be closed after this
             * method returns, regardless of it's success.
             */
            bool open( Device::Device* dev );

            Device::Device* device() const { return m_device; }
            const QString& volumeIdentifier() const { return m_volumeIdentifier; }
            unsigned int numTitles() const { return m_titles.count(); }

            /**
             * Get a title from the Video DVD. Index starts at 0.
             */
            const Title& title( unsigned int num ) const;
            const Title& operator[]( unsigned int num ) const;

            void debug() const;

        private:
            Device::Device* m_device;
            QVector<Title> m_titles;
            QString m_volumeIdentifier;
        };

        LIBK3B_EXPORT QString audioFormatString( int format );
        LIBK3B_EXPORT QString audioCodeExtensionString( int ext );
        LIBK3B_EXPORT QString subPictureCodeModeString( int mode );
        LIBK3B_EXPORT QString subPictureCodeExtensionString( int ext );
    }
}

#endif
