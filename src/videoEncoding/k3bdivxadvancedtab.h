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

/***************************************************************************
                          k3bdivxadvancedtab.h  -  description
                             -------------------
    begin                : Tue Jul 30 2002
    copyright          : (C) 2002 by Sebastian Trueg
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

#ifndef K3BDIVXADVANCEDTAB_H
#define K3BDIVXADVANCEDTAB_H

#include <qwidget.h>

class K3bDivxCodecData;
class K3bDivxExtSettings;
/**
  *@author Sebastian Trueg
  */

class K3bDivxAdvancedTab : public QWidget  {
   Q_OBJECT
public: 
    K3bDivxAdvancedTab(K3bDivxCodecData *, QWidget *parent=0, const char *name=0);
    ~K3bDivxAdvancedTab();
public slots:
    void slotUpdateView();
private:
    K3bDivxCodecData *m_data;
    K3bDivxExtSettings *m_extension;

    void setupGui();
};

#endif
