/***************************************************************************
                          k3bcddacopy.h  -  description
                             -------------------
    begin                : Sun Nov 4 2001
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

#ifndef K3BCDDACOPY_H
#define K3BCDDACOPY_H

#include "config.h"

#include <qthread.h>
#include <qarray.h>

#include "k3bcdview.h"

struct cdrom_drive;
class QStringList;
class KProgress;

/**
  *@author Sebastian Trueg
  */
#ifdef QT_THREAD_SUPPORT
class K3bCddaCopy : public QThread {
#else
class K3bCddaCopy {
#endif

public:
  K3bCddaCopy(int arraySize);
  ~K3bCddaCopy();
#ifdef QT_THREAD_SUPPORT
    virtual void run();
#else
    void run();
#endif
    void setDrive(QString device);
    void setCopyTracks( QArray<int> tracks );
    void setCopyFiles( QStringList list );
    void setCopyCount( int );
    void setFinish(bool stop);
    void setProgressBar(KProgress*, long);

private:
    QStringList m_list;
    QString m_device;
    int m_count;
    bool m_interrupt;
    QArray<int> m_track;
    struct cdrom_drive *m_drive;
    K3bCdda *m_cdda;
    long m_bytes;
    long m_bytesAll;
    KProgress *m_progress;
    bool paranoiaRead(struct cdrom_drive *drive, int track, QString dest);
    void writeWavHeader(QDataStream *s, long byteCount);

};

#endif
