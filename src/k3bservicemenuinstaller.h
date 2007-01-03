/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
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

#ifndef _K3B_SERVICE_MENU_INSTALLER_H_
#define _K3B_SERVICE_MENU_INSTALLER_H_

#include <qstring.h>

class QWidget;


/**
 * The K3bServiceInstaller installs konqueror servicemenus locally.
 * These servicemenus have to be installed in the appdata dir under
 * the subfolder "servicemenus".
 */
class K3bServiceInstaller
{
 public:
  K3bServiceInstaller( const QString& appname = "k3b" );
  ~K3bServiceInstaller();

  /**
   * Checks if all servicemenus are properly installed.
   */
  bool allInstalled() const;

  /**
   * If parent != 0 a messagebox will be shown in case of an error.
   */
  bool install( QWidget* parent = 0 );

  /**
   * If parent != 0 a messagebox will be shown in case of an error.
   */
  bool remove( QWidget* parent = 0 );

 private:
  class Private;
  Private* d;
};

#endif
