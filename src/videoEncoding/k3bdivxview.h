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
/**
  *@author Sebastian Trueg
  */

class K3bDivxView : public KDialogBase  {
     Q_OBJECT
public:
    K3bDivxView( QWidget* parent=0, const char *name=0 );
    ~K3bDivxView();
private slots:
    void slotUser1();
    void slotUser2();
    void slotEnableSizeTab();

private:
    K3bDivxCodecData *m_codingData;
    //K3bDivxDoc* m_doc;
    K3bDivxBaseTab *m_baseTab;
    K3bDivxSizeTab *m_sizeTab;
    K3bDivXEncodingProcess *m_divxJob;
    K3bBurnProgressDialog *m_divxDialog;

    void setupGui();
    int checkSettings();
};

#endif
