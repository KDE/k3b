/* 
 *
 * $Id: $
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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
