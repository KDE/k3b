/***************************************************************************
                          k3bdvdcontent.h  -  description
                             -------------------
    begin                : Sun Feb 24 2002
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

#ifndef K3BDVDCONTENT_H
#define K3BDVDCONTENT_H

#include <qstring.h>
#include <qstringlist.h>

/**
  *@author Sebastian Trueg
  */

class K3bDvdContent {
public: 
    K3bDvdContent();
    ~K3bDvdContent();
    QString m_input, m_mode, m_res, m_aspect, m_time;
    QString m_video, m_audio, m_frames, m_framerate;
    QStringList m_audioList;
    int chapters;
};

#endif
