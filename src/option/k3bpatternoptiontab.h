/*
 *
 * $Id$
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


#ifndef K3BPATTERNOPTIONTAB_H
#define K3BPATTERNOPTIONTAB_H

#include "base_k3bpatternoptiontab.h"

#include <cddb/k3bcddbquery.h>


/**
  *@author Sebastian Trueg
  */
class K3bPatternOptionTab : public base_K3bPatternOptionTab
{
  Q_OBJECT

 public:
  K3bPatternOptionTab( QWidget *parent = 0, const char *name = 0 );
  ~K3bPatternOptionTab();

  void readSettings();
  void apply();

 protected slots:
  void slotUpdateExample();
  void slotSeeSpecialStrings();

 private:
  QString basicDirectoryPattern() const;
  QString basicFilenamePattern() const;
  QString patternForName( const QString& name ) const;
  QString basicPatternItemFromIndex( int, bool );

  K3bCddbResultEntry m_exampleEntry;
};

#endif
