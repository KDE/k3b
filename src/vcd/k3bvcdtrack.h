/***************************************************************************
                          k3bvcdtrack.h  -  description
                             -------------------
    begin                : Mon Nov 4 2002
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

#ifndef K3BVCDTRACK_H
#define K3BVCDTRACK_H

#include <qstring.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qlist.h>


class K3bVcdTrack
{
 public:
  K3bVcdTrack( QList<K3bVcdTrack>* parent, const QString& filename );
  ~K3bVcdTrack();

  QString fileName() const { return QFileInfo(m_file).fileName(); }
  QString absPath() const { return QFileInfo(m_file).absFilePath(); }
  QString mimeComment() const {return m_mimetype;}
  unsigned long size() const;
  /** returns the index in the list */
  int index() const;
  void setMimeType(const QString&);

 protected:
  QList<K3bVcdTrack>* m_parent;
  int m_filetype;
  QFile m_file;
  QString m_mimetype;

};


#endif
