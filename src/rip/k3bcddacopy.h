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

#include <qobject.h>

#include "k3bcdview.h"
typedef Q_INT16 size16;
typedef Q_INT32 size32;

extern "C" {
#include <cdda_interface.h>
#include <cdda_paranoia.h>
}

struct cdrom_drive;
class QStringList;
class QFile;
class QDataStream;
class QTimer;
class KProgress;

/**
  *@author Sebastian Trueg
  */
class K3bCddaCopy : public QWidget {
    Q_OBJECT
public:
  K3bCddaCopy(int arraySize);
  ~K3bCddaCopy();
    bool run();
    void setDrive(QString device);
    void setCopyTracks( QArray<int> tracks );
    void setCopyFiles( QStringList list );
    void setCopyCount( int );
    void setFinish(bool stop);
    void setProgressBar(KProgress*, long);
    bool finished() { return m_finished; };

signals:
    void endRipping();
    void interrupted();

private slots:
    void slotReadData();

private:
    QStringList m_list;
    QString m_device;
    QFile *m_f;
    QDataStream *m_stream;
    int m_count;
    int m_progressBarValue;
    int m_currentTrackIndex;
    long m_currentSector;
    long m_lastSector;
    bool m_interrupt;
    QArray<int> m_track;
    struct cdrom_drive *m_drive;
    K3bCdda *m_cdda;
    long m_bytes;
    long m_bytesAll;
    bool m_finished;
    KProgress *m_progress;
    QTimer *t;
    cdrom_paranoia *m_paranoia;
    bool paranoiaRead(struct cdrom_drive *drive, int track, QString dest);
    void writeWavHeader(QDataStream *s, long byteCount);
    void readDataFinished();
    bool startRip(int i);
    void finishedRip();
};

#endif
