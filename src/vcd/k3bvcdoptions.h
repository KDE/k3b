/***************************************************************************
                          k3bvcdoptions.h  -  description
                             -------------------
    begin                : Sam Nov 23 2002
    copyright            : (C) 2002 by Sebastian Trueg
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

#ifndef K3B_VCD_OPTIONS_H
#define K3B_VCD_OPTIONS_H

#include <qstring.h>

class KConfig;


class K3bVcdOptions
{
 public:
  K3bVcdOptions();

  const QString& systemId() const { return m_systemId; }
  const QString& albumId() const { return m_albumID; }
  const QString& volumeId() const { return m_volumeID; }
  const QString& volumeSetId() const { return m_volumeSetId; }
  const QString& publisher() const { return m_publisher; }
  const QString& preparer() const { return m_preparer; }

  void setSystemId( const QString& s ) { m_systemId = s; }
  void setAlbumId( const QString& s ) { m_albumID = s; }
  void setVolumeId( const QString& s ) { m_volumeID = s; }
  void setVolumeSetId( const QString& s ) { m_volumeSetId = s; }
  void setPublisher( const QString& s ) { m_publisher = s; }
  void setPreparer( const QString& s ) { m_preparer = s; }

  void save( KConfig* c );

  static K3bVcdOptions load( KConfig* c );
  static K3bVcdOptions defaults();

 private:
  // volume descriptor
  QString m_volumeID;
  QString m_albumID;
  QString m_applicationID;  
  QString m_preparer;
  QString m_publisher;
  QString m_systemId;
  QString m_volumeSetId;

};

#endif
