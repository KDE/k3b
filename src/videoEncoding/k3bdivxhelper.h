/***************************************************************************
                        k3bdivxhelper.cpp  -  description
                           -------------------
  begin                : Sun Jan 5 2003
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
#ifndef K3BDIVXHELPER_H
#define K3BDIVXHELPER_H

#include <qobject.h>

class KProcess;
class KShellProcess;
class K3bDivxCodecData;

class K3bDivxHelper : public QObject{
    Q_OBJECT
public:
    K3bDivxHelper();
    ~K3bDivxHelper();
    void deleteIfos( K3bDivxCodecData *data );
    
public slots:    
    void slotDeleteFinished( );

signals:
    void finished( bool );
};

#endif

