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

/***************************************************************************
                          k3bdvdsizetab.h  -  description
                             -------------------
    begin                : Mon Apr 1 2002
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

#ifndef K3BDVDSIZETAB_H
#define K3BDVDSIZETAB_H

#include <qwidget.h>

class K3bDivxCodecData;
class K3bDivxCrop;
class K3bDivxInfoExtend;
class K3bDivxResize;

/**
  *@author Sebastian Trueg
  */

class K3bDivxSizeTab : public QWidget  {
   Q_OBJECT
public:
    K3bDivxSizeTab( K3bDivxCodecData *data, QWidget *parent=0, const char *name=0);
    ~K3bDivxSizeTab();
    void show();
    void resetView();
    void updateView();
private:
    K3bDivxCodecData *m_data;
    K3bDivxCrop *m_crop;
    K3bDivxInfoExtend *m_info;
    K3bDivxResize *m_resize;
    bool m_initialized;

    void setupGui();
};

#endif
