/***************************************************************************
                          k3bvcdtrackdialog.h  -  description
                             -------------------
    begin                : Don Jan 16 2003
    copyright            : (C) 2003 by Sebastian Trueg, Christian Kvasny
    email                : trueg@informatik.uni-freiburg.de
                           chris@ckvsoft.at
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BVCDTRACKDIALOG_H
#define K3BVCDTRACKDIALOG_H


#include <kdialogbase.h>
#include <qptrlist.h>

class K3bVcdTrack;
class QLineEdit;
class QMultiLineEdit;
class QLabel;
class QCheckBox;
class QComboBox;
class KToggleAction;
class KIntNumInput;
class KCutLabel;


/**
  *@author Sebastian Trueg
  */

class K3bVcdTrackDialog : public KDialogBase
{
  Q_OBJECT

 public:
  K3bVcdTrackDialog( QList<K3bVcdTrack>&, QWidget *parent=0, const char *name=0);
  ~K3bVcdTrackDialog();

 protected slots:
  void slotOk();

 private:
  QList<K3bVcdTrack> m_tracks;

  QLabel* m_labelMimeType;
  KCutLabel* m_displayFileName;
  QLabel* m_displaySize;
  QLabel* m_displayLength;

  void setupGui();
};

#endif
