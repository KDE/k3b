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

//class K3bDvdDoc;
class K3bDvdDirectories;
class K3bDvdAVSet;
class K3bDvdAVExtend;
class K3bDvdCodecData;
class K3bDvdBaseTab;
class K3bDvdSizeTab;
class K3bDivXEncodingProcess;
class K3bBurnProgressDialog;
/**
  *@author Sebastian Trueg
  */

class K3bDvdView : public KDialogBase  {
     Q_OBJECT
public:
    K3bDvdView( QWidget* parent=0, const char *name=0 );
    ~K3bDvdView();
private slots:
    void slotUser1();
    void slotUser2();

private:
    K3bDvdCodecData *m_codingData;
    //K3bDvdDoc* m_doc;
    K3bDvdBaseTab *m_baseTab;
    K3bDvdSizeTab *m_sizeTab;
    K3bDivXEncodingProcess *m_divxJob;
    K3bBurnProgressDialog *m_divxDialog;

    void setupGui();
};

#endif
