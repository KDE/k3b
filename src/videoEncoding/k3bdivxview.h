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
                          k3bdvdview.h  -  description
                             -------------------
    begin                : Sun Mar 31 2002
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

#ifndef K3BDVDVIEW_H
#define K3BDVDVIEW_H

#include <kdialogbase.h>

//class K3bDivxDoc;
class K3bDivxDirectories;
class K3bDivxAVSet;
class K3bDivxAVExtend;
class K3bDivxCodecData;
class K3bDivxBaseTab;
class K3bDivxSizeTab;
class K3bDivXEncodingProcess;
class K3bBurnProgressDialog;
class K3bDivxAdvancedTab;
/**
  *@author Sebastian Trueg
  */

class K3bDivxView : public KDialogBase  {
     Q_OBJECT
public:
    K3bDivxView( QWidget* parent=0, const char *name=0 );
    K3bDivxView( K3bDivxCodecData *data, QWidget* parent=0, const char *name=0);
    ~K3bDivxView();
public slots:
    void slotUpdateView();
private slots:
    void slotUser1();
    void slotUser2();
    void slotEnableSizeTab();

private:
    K3bDivxCodecData *m_codingData;
    //K3bDivxDoc* m_doc;
    K3bDivxBaseTab *m_baseTab;
    K3bDivxSizeTab *m_sizeTab;
    K3bDivxAdvancedTab *m_advancedTab;
    K3bDivXEncodingProcess *m_divxJob;
    K3bBurnProgressDialog *m_divxDialog;

    void setupGui();
    int checkSettings();
};

#endif
