/***************************************************************************
                          kstringlistdialog.h  -  description
                             -------------------
    begin                : Fri Mar 8 2002
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

#ifndef KSTRINGLISTDIALOG_H
#define KSTRINGLISTDIALOG_H

#include <kdialogbase.h>
#include <qstringlist.h>

/**Displays a list of strings and a message
  *@author Sebastian Trueg
  */

class KStringListDialog : public KDialogBase  
{
   Q_OBJECT

 public: 
  KStringListDialog( const QStringList&, const QString& caption = QString::null, 
		     const QString& message = QString::null, bool modal = true, 
		     QWidget *parent = 0, const char *name = 0 );
  ~KStringListDialog();

  virtual QSize sizeHint() const;
};

#endif
