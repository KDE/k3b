/***************************************************************************
                          k3bcdda.h  -  description
                             -------------------
    begin                : Sat Nov 3 2001
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

#ifndef K3BCDDA_H
#define K3BCDDA_H

class QString;
struct cdrom_drive;

/**
  *@author Sebastian Trueg
  */

class K3bCdda {
public: 
	K3bCdda();
	~K3bCdda();
	void closeDrive( struct cdrom_drive *drive );
   struct cdrom_drive* pickDrive( QString newPath );
   long getRawTrackSize(int track, struct cdrom_drive *drive);
   //void paranoiaRead(struct cdrom_drive * drive,  long firstSector, long lastSector);
   void paranoiaRead(struct cdrom_drive * drive,  int track);

private:
  //void paranoiaCallback(long, int);

};

#endif
