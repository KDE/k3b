/***************************************************************************
                          k3bprogressdialog.h  -  description
                             -------------------
    begin                : Thu Mar 29 2001
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

#ifndef K3BPROGRESSDIALOG_H
#define K3BPROGRESSDIALOG_H

#include <kdialog.h>

class KProgress;
class KPushButton;

/**Shows progress of adding a new file to the project.
  *@author Sebastian Trueg
  */

class K3bProgressDialog : public KDialog  {

   Q_OBJECT

public:
	K3bProgressDialog( const QString& fileName, QWidget *parent=0, const char *name=0);
	~K3bProgressDialog();

  /** reimplemented from QDialog */
  void show();
	
private:
	KProgress* m_progress;
	KPushButton* m_cancelButton;

signals:
  void canceled();

public slots:
  /** set the progress, if >= 100, the dialog will close and
	   destroy itself! */
  void setPercent( int value );
};

#endif
