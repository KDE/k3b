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

#ifndef koDirectoryStore_h
#define koDirectoryStore_h

#include "koStore.h"

class QFile;

class KoDirectoryStore : public KoStore
{
public:
    KoDirectoryStore( const QString& path, Mode _mode );
    ~KoDirectoryStore();
protected:
    virtual bool init( Mode _mode );
    virtual bool openWrite( const QString& name ) { return openReadOrWrite( name, IO_WriteOnly ); }
    virtual bool openRead( const QString& name ) { return openReadOrWrite( name, IO_ReadOnly ); }
    virtual bool closeRead() { return true; }
    virtual bool closeWrite() { return true; }
    virtual bool enterRelativeDirectory( const QString& dirName );
    virtual bool enterAbsoluteDirectory( const QString& path );
    virtual bool fileExists( const QString& absPath );

    bool openReadOrWrite( const QString& name, int iomode );
private:
    // Path to base directory (== the ctor argument)
    QString m_basePath;

    // Path to current directory
    QString m_currentPath;

    // Current File
    QFile* m_file;
};

#endif
