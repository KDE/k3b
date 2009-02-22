/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_GROWISOFS_HANDLER_H_
#define _K3B_GROWISOFS_HANDLER_H_

#include <qobject.h>

namespace K3b {
    namespace Device {
        class Device;
        class DeviceHandler;
    }


    /**
     * This class handles the output parsing for growisofs
     * We put it in an extra class since we have two classes
     * using growisofs: the writer and the imager.
     */
    class GrowisofsHandler : public QObject
    {
        Q_OBJECT

    public:
        GrowisofsHandler( QObject* parent = 0 );
        ~GrowisofsHandler();

        enum ErrorType {
            ERROR_UNKNOWN,
            ERROR_MEDIA,
            ERROR_OVERSIZE,
            ERROR_SPEED_SET_FAILED,
            ERROR_OPC,
            ERROR_MEMLOCK,
            ERROR_WRITE_FAILED
        };

        int error() const { return m_error; }

    public Q_SLOTS:
        /**
         * This will basically reset the error type
         * @param dao was growisofs called with DAO?
         */
        void reset( K3b::Device::Device* = 0, bool dao = false );

        void handleStart();
        void handleLine( const QString& );
        void handleExit( int exitCode );

    Q_SIGNALS:
        void infoMessage( const QString&, int );
        void newSubTask( const QString& );
        void buffer( int );
        void deviceBuffer( int );

        /**
         * We need this to know when the writing finished to update the progress
         */
        void flushingCache();

    private Q_SLOTS:
        void slotCheckBufferStatus();
        void slotCheckBufferStatusDone( Device::DeviceHandler* );

    private:
        class Private;
        Private* d;

        int m_error;
        bool m_dao;
        Device::Device* m_device;
    };
}

#endif
