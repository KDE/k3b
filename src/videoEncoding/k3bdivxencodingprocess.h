/***************************************************************************
                          k3bdivxencodingprocess.h  -  description
                             -------------------
    begin                : Sun Apr 28 2002
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

#ifndef K3BDIVXENCODINGPROCESS_H
#define K3BDIVXENCODINGPROCESS_H

#include <qwidget.h>
#include "../k3bjob.h"

class K3bDivxCodecData;
class KProcess;
class KShellProcess;

/**
  *@author Sebastian Trueg
  */

class K3bDivXEncodingProcess : public K3bJob  {
   Q_OBJECT
public:
    K3bDivXEncodingProcess(K3bDivxCodecData *data, QWidget *parent=0, const char *name=0);
    ~K3bDivXEncodingProcess();
public slots:
    void start();
    void cancel();
    void slotEncodingExited( KProcess *p );
    void slotParseEncoding( KProcess *p, char *buffer, int lenght);
    void slotAudioExited( KProcess *p );
    void slotParseAudio( KProcess *p, char *buffer, int lenght);

    //void slotPercent( unsigned int );
private:
    K3bDivxCodecData *m_data;
    KShellProcess *m_process;
    int m_speedFlag;
    int m_speedTrigger;
    int m_pass;

    void startEncoding();

};

#endif
