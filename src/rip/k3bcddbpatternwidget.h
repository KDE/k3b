/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_CDDB_PATTERN_WIDGET_H_
#define _K3B_CDDB_PATTERN_WIDGET_H_

#include "base_k3bcddbpatternwidget.h"

class KConfig;


class K3bCddbPatternWidget : public base_K3bCddbPatternWidget
{
  Q_OBJECT

 public:
  K3bCddbPatternWidget( QWidget* parent = 0, const char* name = 0 );
  ~K3bCddbPatternWidget();

  QString filenamePattern() const;
  QString playlistPattern() const;
  QString blankReplaceString() const;
  bool replaceBlanks() const;

 signals:
  void changed();

 public slots:
  void loadConfig( KConfig* );
  void saveConfig( KConfig* );
  void loadDefaults();

 private slots:
  void slotSeeSpecialStrings();
  void slotSeeConditionalInclusion();
};

#endif
