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


#ifndef _K3B_ISO9660_H_
#define _K3B_ISO9660_H_

#include "k3b_export.h"

#include <QDateTime>
#include <QHash>
#include <QString>
#include <QStringList>

#include <sys/stat.h>
#include <sys/types.h>


struct iso_directory_record;
struct el_torito_boot_descriptor;
struct iso_primary_descriptor;
typedef long sector_t;

namespace K3b {
    namespace Device {
        class Device;
    }

    class Iso9660;
    class Iso9660Backend;


    /**
     * Simplified primary descriptor which just contains the fields
     * used by K3b.
     */
    class LIBK3B_EXPORT Iso9660SimplePrimaryDescriptor
    {
    public:
        /**
         * Creates an empty descriptor
         */
        Iso9660SimplePrimaryDescriptor();

        QString volumeId;
        QString systemId;
        QString volumeSetId;
        QString publisherId;
        QString preparerId;
        QString applicationId;
        int volumeSetSize;
        int volumeSetNumber;
        long logicalBlockSize;
        long long volumeSpaceSize;
    };


    LIBK3B_EXPORT bool operator==( const Iso9660SimplePrimaryDescriptor& d1,
                                   const Iso9660SimplePrimaryDescriptor& d2 );
    LIBK3B_EXPORT bool operator!=( const Iso9660SimplePrimaryDescriptor& d1,
                                   const Iso9660SimplePrimaryDescriptor& d2 );


    /**
     * Base class for all entries in a Iso9660 archive. A lot has been copied
     * from KArchive.
     */
    class LIBK3B_EXPORT Iso9660Entry
    {
    public:
        Iso9660Entry( Iso9660* archive,
                      const QString& isoName,
                      const QString& name,
                      int access,
                      int date,
                      int adate,
                      int cdate,
                      const QString& user,
                      const QString& group,
                      const QString& symlink );
        virtual ~Iso9660Entry();

        int adate() const { return m_adate; }
        int cdate() const { return m_cdate; }

        /**
         * Creation date of the file.
         * @return the creation date
         */
        QDateTime datetime() const;

        /**
         * Creation date of the file.
         * @return the creation date in seconds since 1970
         */
        int date() const { return m_date; }

        /**
         * Name of the file without path.
         * @return The file name without path.
         */
        QString name() const { return m_name; }

        /**
         * \return The raw name as saved in the ISO9660 tree
         */
        QString isoName() const { return m_isoName; }

        /**
         * The permissions and mode flags as returned by the stat() function
         * in st_mode.
         * @return the permissions
         */
        mode_t permissions() const { return m_access; }

        /**
         * User who created the file.
         * @return the owner of the file
         */
        QString user() const { return m_user; }

        /**
         * Group of the user who created the file.
         * @return the group of the file
         */
        QString group() const { return m_group; }

        /**
         * Symlink if there is one.
         * @return the symlink, or QString()
         */
        QString symlink() const { return m_symlink; }

        /**
         * Checks whether the entry is a file.
         * @return true if this entry is a file
         */
        virtual bool isFile() const { return false; }

        /**
         * Checks whether the entry is a directory.
         * @return true if this entry is a directory
         */
        virtual bool isDirectory() const { return false; }

        Iso9660* archive() const { return m_archive; }

    private:
        int m_adate;
        int m_cdate;
        QString m_name;
        QString m_isoName;
        int m_date;
        mode_t m_access;
        QString m_user;
        QString m_group;
        QString m_symlink;
        Iso9660* m_archive;
    };


    class LIBK3B_EXPORT Iso9660Directory : public Iso9660Entry
    {
    public:
        Iso9660Directory( Iso9660* archive,
                          const QString& isoName,
                          const QString& name,
                          int access,
                          int date,
                          int adate,
                          int cdate,
                          const QString& user,
                          const QString& group,
                          const QString& symlink,
                          unsigned int pos = 0,
                          unsigned int size = 0 );
        ~Iso9660Directory() override;

        /**
         * Returns a list of sub-entries.
         * @return the names of all entries in this directory (filenames, no path).
         */
        QStringList entries() const;

        /**
         * Returns the entry with the given name.
         * @param name may be "test1", "mydir/test3", "mydir/mysubdir/test3", etc.
         * @return a pointer to the entry in the directory.
         */
        Iso9660Entry* entry( const QString& name );

        /**
         * Returns the entry with the given name.
         * @param name may be "test1", "mydir/test3", "mydir/mysubdir/test3", etc.
         * @return a pointer to the entry in the directory.
         */
        const Iso9660Entry* entry( const QString& name ) const;

        /**
         * Returns a list of sub-entries.
         * Searches for Iso9660 names.
         * @return the names of all entries in this directory (filenames, no path).
         */
        QStringList iso9660Entries() const;

        /**
         * Returns the entry with the given name.
         * Searches for Iso9660 names.
         * @param name may be "test1", "mydir/test3", "mydir/mysubdir/test3", etc.
         * @return a pointer to the entry in the directory.
         */
        Iso9660Entry* iso9660Entry( const QString& name );

        /**
         * Returns the entry with the given name.
         * Searches for Iso9660 names.
         * @param name may be "test1", "mydir/test3", "mydir/mysubdir/test3", etc.
         * @return a pointer to the entry in the directory.
         */
        const Iso9660Entry* iso9660Entry( const QString& name ) const;

        /**
         * @internal
         * Adds a new entry to the directory.
         */
        void addEntry( Iso9660Entry* );

        /**
         * Checks whether this entry is a directory.
         * @return true, since this entry is a directory
         */
        bool isDirectory() const override { return true; }

    private:
        void expand();

        QHash<QString, Iso9660Entry*> m_entries;
        QHash<QString, Iso9660Entry*> m_iso9660Entries;

        bool m_bExpanded;
        unsigned int m_startSector;
        unsigned int m_size;
    };


    class LIBK3B_EXPORT Iso9660File : public Iso9660Entry
    {
    public:
        /**
         * @param pos start sector
         */
        Iso9660File( Iso9660* archive,
                     const QString& isoName,
                     const QString& name,
                     int access,
                     int date,
                     int adate,
                     int cdate,
                     const QString& user,
                     const QString& group,
                     const QString& symlink,
                     unsigned int pos,
                     unsigned int size );
        ~Iso9660File() override;

        bool isFile() const override { return true; }

        void setZF( char algo[2], char parms[2], int realsize );
        int realsize() const { return m_realsize; }

        /**
         * @return size in bytes.
         */
        unsigned int size() const { return m_size; }

        /**
         * Returns the startSector of the file.
         */
        unsigned int startSector() const { return m_startSector; }

        /**
         * Returns the startOffset of the file in bytes.
         */
        unsigned long long startPostion() const { return (unsigned long long)m_startSector * 2048; }

        /**
         * @param pos offset in bytes
         * @param len max number of bytes to read
         */
        int read( unsigned int pos, char* data, int len ) const;

        /**
         * Copy this file to a url.
         */
        bool copyTo( const QString& url ) const;

    private:
        char m_algo[2];
        char m_parms[2];
        int m_realsize;

        unsigned int m_curpos;
        unsigned int m_startSector;
        unsigned int m_size;
    };


    /**
     * This class is based on the KIso class by
     * Gy�gy Szombathelyi <gyurco@users.sourceforge.net>.
     * A lot has been changed and bugfixed.
     * The API has been improved to be useful.
     *
     * Due to the stupid Qt which does not support large files as default
     * we cannot use QIODevice with DVDs! That's why we have our own
     * reading code which is not allowed by KArchive (which is limited to int
     * by the way... who the hell designed this?)
     * I also removed the KArchive inheritance because of the named reasons.
     * So this stuff contains a lot KArchive code which has been made usable.
     *
     * That does not mean that this class is well designed. No, it's not. :)
     *
     * Opening a Iso9660 object should be fast since creation of the directory
     * and file entries is not done until a call to Iso9660Directory::entries.
     */
    class LIBK3B_EXPORT Iso9660
    {
    public:
        /**
         * Creates an instance that operates on the given filename.
         * using the compression filter associated to given mimetype.
         *
         * @param filename is a local path (e.g. "/home/weis/myfile.tgz")
         */
        explicit Iso9660( const QString& filename );

        /**
         * Special case which always reads the TOC from the specified sector
         * thus supporting multisession CDs.
         */
        explicit Iso9660( Device::Device* dev, unsigned int startSector = 0 );

        /**
         * @param fd open file descriptor
         */
        explicit Iso9660( int fd );

        /**
         * Directly specify the backend to read from.
         * Iso9660 will take ownership of the backend and delete it.
         */
        explicit Iso9660( Iso9660Backend* );

        /**
         * If the .iso is still opened, then it will be
         * closed automatically by the destructor.
         */
        virtual ~Iso9660();

        /**
         * Set where to start reading in the source.
         */
        void setStartSector( unsigned int startSector );

        /**
         * If set to true before opening Iso9660 will ignore RR and joliet extensions
         * and only create plain iso9660 names.
         */
        void setPlainIso9660( bool );

        bool plainIso9660() const;

        /**
         * Opens the archive for reading.
         * Parses the directory listing of the archive
         * and creates the Iso9660Directory/Iso9660File entries.
         */
        bool open();

        bool isOpen() const;

        /**
         * Closes everything.
         * This is also called in the destructor
         */
        void close();

        /**
         * @param sector startsector
         * @param len number of sectors
         * @return number of sectors read or -1 on error
         */
        int read( unsigned int sector, char* data, int len );

        /**
         * The name of the os file, as passed to the constructor
         * Null if you did not use the QString constructor.
         */
        QString fileName() { return m_filename; }

        const Iso9660Directory* firstJolietDirEntry() const;
        const Iso9660Directory* firstRRDirEntry() const;
        const Iso9660Directory* firstIsoDirEntry() const;
        const Iso9660Directory* firstElToritoEntry() const;

        /**
         * @returns 0 if no joliet desc could be found
         *          the joliet level (1-3) otherwise
         */
        int jolietLevel() const { return m_joliet; }

        const Iso9660SimplePrimaryDescriptor& primaryDescriptor() const;

        void debug() const;

    private:
        /**
         * @internal
         */
        void addBoot( struct el_torito_boot_descriptor* bootdesc );
        void createSimplePrimaryDesc( struct iso_primary_descriptor* desc );

        void debugEntry( const Iso9660Entry*, int depth ) const;

        int m_joliet;

        // only used for creation
        static int read_callback( char* buf, sector_t start, int len, void* udata );
        static int isofs_callback( struct iso_directory_record* idr, void *udata );
        Iso9660Directory *dirent;
        bool m_rr;
        friend class Iso9660Directory;

    private:
        QString m_filename;

        class Private;
        Private * d;
    };
}

#endif
