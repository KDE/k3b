/***************************************************************************
                          k3bcddb.h  -  description
                             -------------------
    begin                : Sun Oct 7 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BCDDB_H
#define K3BCDDB_H

struct cdrom_drive;
class QString;
class QStringList;
class CDDB;

class K3bCddb {
  public:
    K3bCddb( );
    K3bCddb( bool, QString, unsigned int );
    ~K3bCddb(  );
    /**
	* Searches for a cd in a drive and open the drive.
	*/
    //struct cdrom_drive *pickDrive( QString newPath );
    /**
	* Reads the content and parses for cddb entries if enabled.
	*/
    void updateCD( struct cdrom_drive * );
    /**
	* Gets the disc id of the cd in the drive.
	*/
    unsigned int get_discid( struct cdrom_drive *drive );
    /**
    *
    */
    void update( QString *device );
	/**
	* Closes the current connection to drive. Must be called if cd has changed.
	*/
    //void closeDrive(struct cdrom_drive *drive);

    void setServer(QString server) { m_cddbServer = server; };
    void setPort(unsigned int port) { m_cddbPort = port; };
    /**
    * Generates instance of cddb and enables cddb use.
    */
    void setUseCddb(bool useCddb);

    QStringList getTitles() { return titles; };
    QString getAlbum() { return cd_album; };
    QString getArtist() { return cd_artist; };

  private:
    bool m_useCddb;
    CDDB *m_cddb;
    QString m_cddbServer;
    unsigned int m_cddbPort;

    int trackIndex;
    unsigned int discid;
    int tracks;
    QString cd_album;
    QString cd_artist;
    QStringList titles;
    bool is_audio[100];
    bool based_on_cddb;
    QString s_track;
};

#endif
