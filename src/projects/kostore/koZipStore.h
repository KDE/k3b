/* This file is part of the KDE project
   Copyright (C) 2002 David Faure <david@mandrakesoft.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef koZipStore_h
#define koZipStore_h

#include "koStore.h"

class KoZip;
class KArchiveDirectory;

class KoZipStore : public KoStore
{
public:
    KoZipStore( const QString & _filename, Mode _mode, const QCString & appIdentification );
    KoZipStore( QIODevice *dev, Mode mode, const QCString & appIdentification );
    ~KoZipStore();

    virtual Q_LONG write( const char* _data, Q_ULONG _len );
protected:
    virtual bool init( Mode _mode, const QCString& appIdentification );
    virtual bool openWrite( const QString& name );
    virtual bool openRead( const QString& name );
    virtual bool closeWrite();
    virtual bool closeRead() { return true; }
    virtual bool enterRelativeDirectory( const QString& dirName );
    virtual bool enterAbsoluteDirectory( const QString& path );
    virtual bool fileExists( const QString& absPath );

    // The archive
    KoZip * m_pZip;

    // In "Read" mode this pointer is pointing to the
    // current directory in the archive to speed up the verification process
    const KArchiveDirectory* m_currentDir;
};

#endif
