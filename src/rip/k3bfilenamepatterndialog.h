/***************************************************************************
                          k3bfilenamepatterndialog.h  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#ifndef K3BFILENAMEPATTERNDIALOG_H
#define K3BFILENAMEPATTERNDIALOG_H

#include <kdialogbase.h>

class K3bPatternWidget;


/**
  *@author Sebastian Trueg
  */
class K3bFilenamePatternDialog : public KDialogBase
{
  Q_OBJECT

 public: 
  K3bFilenamePatternDialog( QWidget *parent=0, const char *name=0);
  ~K3bFilenamePatternDialog();

  void init( const QString& album, const QString& artist, const QString& title, const QString& number);

 private:
  K3bPatternWidget *m_frame;	
  
 private slots:
  void slotApply();
  void slotOk();
};

#endif
