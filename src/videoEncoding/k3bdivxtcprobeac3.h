/* 
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BDIVXTCPROBEAC3_H
#define K3BDIVXTCPROBEAC3_H

#include <qobject.h>
#include <qstringlist.h>

class KShellProcess;
class KProcess;
class K3bDivxCodecData;
class K3bDivxHelper;


class K3bDivXTcprobeAc3 : public QObject  {
   Q_OBJECT

public: 
    K3bDivXTcprobeAc3();
    ~K3bDivXTcprobeAc3();
    void parseAc3Bitrate( K3bDivxCodecData*);
signals:
    void finished();
private:
    KShellProcess *m_process;
    K3bDivxCodecData *m_data;
    K3bDivxHelper *m_util;
    QString m_buffer;
private slots:
    void slotParseOutput( KProcess*, char *buffer, int length );
    void slotParsingExited( KProcess* );
    void slotInternalParsing();
};

#endif
