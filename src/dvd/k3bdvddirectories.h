/***************************************************************************
                          k3bdvddirectories.h  -  description
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

#ifndef K3BDVDDIRECTORIES_H
#define K3BDVDDIRECTORIES_H

#include <qgroupbox.h>

class KLineEdit;
class QPushButton;

/**
  *@author Sebastian Trueg
  */

class K3bDvdDirectories : public QGroupBox  {
    Q_OBJECT
public: 
	K3bDvdDirectories( QWidget *parent=0, const char *name=0);
	~K3bDvdDirectories();
private:
    KLineEdit *m_editVideoPath;
    QPushButton *m_buttonVideoDir;
    KLineEdit *m_editAudioPath;
    QPushButton *m_buttonAudioDir;
    KLineEdit *m_editAviPath;
    QPushButton *m_buttonAviDir;

    void setupGui();

};

#endif
