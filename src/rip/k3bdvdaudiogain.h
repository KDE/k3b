/***************************************************************************
                          k3bdvdaudiogain.h  -  description
                             -------------------
    begin                : Sun Mar 10 2002
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

#ifndef K3BDVDAUDIOGAIN_H
#define K3BDVDAUDIOGAIN_H

#include <qobject.h>

class KShellProcess;
class KProcess;
class QString;
/**
  *@author Sebastian Trueg
  */

class K3bDvdAudioGain : public QObject  {
    Q_OBJECT
public:
    K3bDvdAudioGain( const QString& dir);
    ~K3bDvdAudioGain();
    bool start();
    void writeStdin( const char*, int len );
    void closeStdin( );
    void kill();
signals:
    void finished();
private:
    KShellProcess *m_audioProcess;
    QString m_dirname;
private slots:
    void slotParseOutput( KProcess *p, char *buffer, int len );
    void slotParseError( KProcess *p, char *buffer, int len );
    void slotExited( KProcess *p);
    void slotWroteStdin( KProcess *p);
};

#endif
