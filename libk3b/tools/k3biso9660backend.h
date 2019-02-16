/*
 *
 * Copyright (C) 2005-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_ISO9660_BACKEND_H_
#define _K3B_ISO9660_BACKEND_H_

#include "k3b_export.h"

#include <QString>

namespace K3b {
    namespace Device {
        class Device;
    }

    class LibDvdCss;

    class LIBK3B_EXPORT Iso9660Backend
    {
    public:
        Iso9660Backend() {}
        virtual ~Iso9660Backend() {}

        virtual bool open() = 0;
        virtual void close() = 0;
        virtual bool isOpen() const = 0;
        virtual int read( unsigned int sector, char* data, int len ) = 0;
    };

    class LIBK3B_EXPORT Iso9660DeviceBackend : public Iso9660Backend
    {
    public:
        explicit Iso9660DeviceBackend( Device::Device* dev );
        ~Iso9660DeviceBackend() override;

        bool open() override;
        void close() override;
        bool isOpen() const override { return m_isOpen; }
        int read( unsigned int sector, char* data, int len ) override;

    private:
        Device::Device* m_device;
        bool m_isOpen;
    };

    class LIBK3B_EXPORT Iso9660FileBackend : public Iso9660Backend
    {
    public:
        explicit Iso9660FileBackend( const QString& filename );
        explicit Iso9660FileBackend( int fd );
        ~Iso9660FileBackend() override;

        bool open() override;
        void close() override;
        bool isOpen() const override;
        int read( unsigned int sector, char* data, int len ) override;

    private:
        QString m_filename;
        int m_fd;
        bool m_closeFd;
    };

    class LIBK3B_EXPORT Iso9660LibDvdCssBackend : public Iso9660Backend
    {
    public:
        explicit Iso9660LibDvdCssBackend( Device::Device* );
        ~Iso9660LibDvdCssBackend() override;

        bool open() override;
        void close() override;
        bool isOpen() const override;
        int read( unsigned int sector, char* data, int len ) override;

    private:
        Device::Device* m_device;
        LibDvdCss* m_libDvdCss;
    };
}

#endif
