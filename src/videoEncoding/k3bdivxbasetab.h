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


#ifndef K3BDVDBASETAB_H
#define K3BDVDBASETAB_H

#include <qwidget.h>
class K3bDivxDirectories;
class K3bDivxAVSet;
class K3bDivxAVExtend;
class K3bDivxCodecData;
class K3bDivxInfo;


class K3bDivxBaseTab : public QWidget  {
   Q_OBJECT
public:
    K3bDivxBaseTab( K3bDivxCodecData *data, QWidget *parent=0, const char *name=0);
    ~K3bDivxBaseTab();
public slots:
    void slotUpdateView(  );
    void slotInitView(  );
signals:
    void projectLoaded();
private:
    K3bDivxCodecData *m_data;
    K3bDivxDirectories *m_directories;
    K3bDivxAVSet *m_avsettings;
    K3bDivxAVExtend *m_avextended;
    K3bDivxInfo *m_info;
    void setupGui();
};

#endif
