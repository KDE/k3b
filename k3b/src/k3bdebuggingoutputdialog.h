/* 
 *
 * $Id$
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_DEBUGGING_OUTPUT_DIALOG_H_
#define _K3B_DEBUGGING_OUTPUT_DIALOG_H_

#include <kdialogbase.h>
#include <qmap.h>

class QTextEdit;

class K3bDebuggingOutputDialog : public KDialogBase
{
  Q_OBJECT

 public:
  K3bDebuggingOutputDialog( QWidget* parent );
  
 public slots:
  void setOutput( const QMap<QString, QStringList>& );
  void addOutput( const QString&, const QString& );
  void clear();

 private:
  void slotUser1();
  void slotUser2();
  
  QTextEdit* debugView;
  QMap<QString, int> m_paragraphMap;
};



#endif
