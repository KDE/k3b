/* 
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

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
