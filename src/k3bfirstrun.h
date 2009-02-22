/* 
 *
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_FIRST_RUN_H_
#define _K3B_FIRST_RUN_H_

#include <kdialog.h>


namespace K3b {
class FirstRun : public KDialog
{
  Q_OBJECT

 public:
  static void run( QWidget* parent = 0 );

 private:
  FirstRun( QWidget* parent );
  ~FirstRun();
};
}

#endif
