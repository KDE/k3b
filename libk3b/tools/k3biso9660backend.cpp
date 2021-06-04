/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3biso9660backend.h"
#include "k3blibdvdcss.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <QFile>

#include "k3bdevice.h"


//
// K3b::Iso9660DeviceBackend -----------------------------------
//

K3b::Iso9660DeviceBackend::Iso9660DeviceBackend( K3b::Device::Device* dev )
    : m_device( dev ),
      m_isOpen(false)
{
}


K3b::Iso9660DeviceBackend::~Iso9660DeviceBackend()
{
    close();
}


bool K3b::Iso9660DeviceBackend::open()
{
    if( m_isOpen )
        return true;
    else if( m_device->open() ) {
        // set optimal reading speed
        m_device->setSpeed( 0xffff, 0xffff );
        m_isOpen = true;
        return true;
    }
    else
        return false;
}


void K3b::Iso9660DeviceBackend::close()
{
    if( m_isOpen ) {
        m_isOpen = false;
        m_device->close();
    }
}


int K3b::Iso9660DeviceBackend::read( unsigned int sector, char* data, int len )
{
    if( isOpen() ) {
        //
        // split the number of sectors to be read
        // FIXME: use a "real" value, not some arbitrary one
        //
        static const int maxReadSectors = 20;
        int sectorsRead = 0;
        int retries = 10;  // TODO: no fixed value
        while( retries ) {
            int read = qMin(len-sectorsRead, maxReadSectors);
            if( !m_device->read10( (unsigned char*)(data+sectorsRead*2048),
                                   read*2048,
                                   sector+sectorsRead,
                                   read ) ) {
                retries--;
            }
            else {
                sectorsRead += read;
                retries = 10; // new retires for every read part
                if( sectorsRead == len )
                    return len;
            }
        }
    }

    return -1;
}


//
// K3b::Iso9660FileBackend -----------------------------------
//

K3b::Iso9660FileBackend::Iso9660FileBackend( const QString& filename )
    : m_filename( filename ),
      m_fd( -1 ),
      m_closeFd( true )
{
}


K3b::Iso9660FileBackend::Iso9660FileBackend( int fd )
    : m_fd( fd ),
      m_closeFd( false )
{
}


K3b::Iso9660FileBackend::~Iso9660FileBackend()
{
    close();
}


#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

bool K3b::Iso9660FileBackend::open()
{
    if( m_fd > 0 )
        return true;
    else {
        m_fd = ::open( QFile::encodeName( m_filename ), O_RDONLY|O_LARGEFILE|O_CLOEXEC );
        return ( m_fd > 0 );
    }
}


void K3b::Iso9660FileBackend::close()
{
    if( m_closeFd && m_fd > 0 ) {
        ::close( m_fd );
        m_fd = -1;
    }
}



bool K3b::Iso9660FileBackend::isOpen() const
{
    return ( m_fd > 0 );
}


int K3b::Iso9660FileBackend::read( unsigned int sector, char* data, int len )
{
    int read = 0;
    if( ::lseek( m_fd, static_cast<unsigned long long>(sector)*2048, SEEK_SET ) != -1 )
        if( (read = ::read( m_fd, data, len*2048 )) != -1 )
            return read / 2048;

    return -1;
}



//
// K3b::Iso9660LibDvdCssBackend -----------------------------------
//

K3b::Iso9660LibDvdCssBackend::Iso9660LibDvdCssBackend( K3b::Device::Device* dev )
    : m_device( dev ),
      m_libDvdCss( 0 )
{
}


K3b::Iso9660LibDvdCssBackend::~Iso9660LibDvdCssBackend()
{
    close();
}


bool K3b::Iso9660LibDvdCssBackend::open()
{
    if( !m_libDvdCss ) {
        // open the libdvdcss stuff
        m_libDvdCss = K3b::LibDvdCss::create();

        if( m_libDvdCss ) {

            if( !m_libDvdCss->open( m_device ) ||
                !m_libDvdCss->crackAllKeys() ) {
                qDebug() << "(K3b::Iso9660LibDvdCssBackend) Failed to retrieve all CSS keys.";
                close();
            }
        }
        else
            qDebug() << "(K3b::Iso9660LibDvdCssBackend) failed to open libdvdcss.";
    }

    return ( m_libDvdCss != 0 );
}


void K3b::Iso9660LibDvdCssBackend::close()
{
    delete m_libDvdCss;
    m_libDvdCss = 0;
}



bool K3b::Iso9660LibDvdCssBackend::isOpen() const
{
    return ( m_libDvdCss != 0 );
}


int K3b::Iso9660LibDvdCssBackend::read( unsigned int sector, char* data, int len )
{
    int read = -1;

    if( isOpen() ) {
        int retries = 10;  // TODO: no fixed value
        while( retries && !m_libDvdCss->readWrapped( reinterpret_cast<void*>(data),
                                                     sector,
                                                     len ) )
            retries--;

        if( retries > 0 )
            read = len;
    }

    return read;
}

