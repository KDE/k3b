/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_ISO9660_H_
#define _K3B_ISO9660_H_

#include <karchive.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <qdatetime.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qdict.h>


namespace K3bCdDevice {
  class CdDevice;
}


/**
 * Simplyfied primary descriptor which just contains the fields
 * used by K3b.
 */
class K3bIso9660SimplePrimaryDescriptor
{
 public:
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


class K3bIso9660Directory : public KArchiveDirectory
{
 public: 
  K3bIso9660Directory( KArchive* archive, const QString& name, int access, int date,
		       int adate,int cdate, const QString& user, const QString& group,
		       const QString& symlink);
  ~K3bIso9660Directory();
  
  int adate() const { return m_adate; }
  int cdate() const { return m_cdate; }

 private:
  int m_adate;
  int m_cdate;
};


class K3bIso9660File : public KArchiveFile
{
 public: 
  K3bIso9660File( KArchive* archive, const QString& name, int access, int date,
		  int adate,int cdate, const QString& user, const QString& group,
		  const QString& symlink, int pos, int size);
  ~K3bIso9660File();

  void setZF(char algo[2],char parms[2],int realsize);
  int adate() const { return m_adate; }
  int cdate() const { return m_cdate; }
  long long realsize() const { return m_realsize; }

  virtual QByteArray data(long long pos, int count) const;
  int read( long long pos, char* data, int len ) const;

 private:
  /**
   * hide this member function, it's broken by design, because the full
   * data often requires too much memory
   */
  virtual QByteArray data() const;
  char m_algo[2];
  char m_parms[2];
  long long m_realsize;
  int m_adate;
  int m_cdate;
  long long m_curpos;
};


/**
 * This class is based on the KIso class by
 * György Szombathelyi <gyurco@users.sourceforge.net>.
 * A lot has been changed and bugfixed.
 * The API has been improved to be useful.
 */
class K3bIso9660 : public KArchive
{
 public:
  /**
   * Creates an instance that operates on the given filename.
   * using the compression filter associated to given mimetype.
   *
   * @param filename is a local path (e.g. "/home/weis/myfile.tgz")
   */
  K3bIso9660( const QString& filename );

  /**
   * Creates an instance that operates on the given device.
   */
  K3bIso9660( QIODevice * dev );

  /**
   * Special case which always reads the TOC from the specified sector
   * thus supporting multisession CDs.
   */
  K3bIso9660( K3bCdDevice::CdDevice* dev, unsigned long startSector );

  /**
   * @param fd open file descriptor
   */
  K3bIso9660( int fd );

  /**
   * If the .iso is still opened, then it will be
   * closed automatically by the destructor.
   */
  virtual ~K3bIso9660();

  /**
   * The name of the os file, as passed to the constructor
   * Null if you used the QIODevice constructor.
   */
  const QString& fileName() { return m_filename; }

  bool writeDir( const QString& name, const QString& user, const QString& group );
  bool prepareWriting( const QString& name, const QString& user, const QString& group, uint size );
  bool doneWriting( uint size );

  const K3bIso9660Directory* firstJolietDirEntry() const;
  const K3bIso9660Directory* firstIsoDirEntry() const;
  const K3bIso9660Directory* firstElToritoEntry() const;

  /**
   * @returns 0 if no joliet desc could be found
   *          the joliet level (1-3) otherwise
   */
  int jolietLevel() const { return m_joliet; }

  const K3bIso9660SimplePrimaryDescriptor& primaryDescriptor() const;

  void debug() const;

  int level;
  K3bIso9660Directory *dirent;

 protected:
  /**
   * Opens the archive for reading.
   * Parses the directory listing of the archive
   * and creates the KArchiveDirectory/KArchiveFile entries.
   *
   */
  virtual bool openArchive( int mode );
  virtual bool closeArchive();

 private:
  /**
   * @internal
   */
  void addBoot(struct el_torito_boot_descriptor* bootdesc);
  void createSimplePrimaryDesc( struct iso_primary_descriptor* desc );

  void debugEntry( const KArchiveEntry*, int depth ) const;

  QString m_filename;
  int m_joliet;

 protected:
  virtual void virtual_hook( int id, void* data );
 
 private:
  class Private;
  Private * d;
};

#endif
