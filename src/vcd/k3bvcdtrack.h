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
#include <qptrlist.h>


class K3bVcdTrack
{
 public:
  K3bVcdTrack( QList<K3bVcdTrack>* parent, const QString& filename );
  ~K3bVcdTrack();

  QString fileName() const { return QFileInfo(m_file).fileName(); }
  QString absPath() const { return QFileInfo(m_file).absFilePath(); }
  QString mpegType() const {return m_mpegtype;}
  QString mpegDuration() const {return m_mpegduration;}
  QString mpegSize() const {return m_mpegsize;};
  QString mpegDisplaySize() const {return m_mpegdisplaysize;};
  QString mpegFps() const {return m_mpegfps;};
  QString mpegMbps() const {return m_mpegmbps;};
  
  unsigned long size() const;
  /** returns the index in the list */
  int index() const;
  void setMpegType(const QString&);
  void setMpegDuration(const QString&);
  void setMpegSize(const QString&);
  void setMpegDisplaySize(const QString&);
  void setMpegFps(const QString&);
  void setMpegMbps(const QString&);
  
 protected:
  QList<K3bVcdTrack>* m_parent;
  int m_filetype;
  QFile m_file;
  QString m_mpegtype;
  QString m_mpegduration;
  QString m_mpegsize;
  QString m_mpegdisplaysize;
  QString m_mpegfps;
  QString m_mpegmbps;

};


#endif
