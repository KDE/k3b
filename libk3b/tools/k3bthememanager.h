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

#ifndef _K3B_THEME_MANAGER_H_
#define _K3B_THEME_MANAGER_H_

#include <qobject.h>
#include <qptrlist.h>
#include <qstring.h>
#include <qmap.h>
#include <qcolor.h>
#include <qpixmap.h>


#define k3bthememanager K3bThemeManager::k3bThemeManager()

class KConfig;


class K3bTheme
{
 public:
  QColor backgroundColor() const { return m_bgColor; }
  QColor foregroundColor() const { return m_fgColor; }

  const QPixmap& pixmap( const QString& name ) const;

  const QString& name() const { return m_name; }
  const QString& author() const { return m_author; }
  const QString& comment() const { return m_comment; }
  const QString& version() const { return m_version; }

  const QString& path() const { return m_path; }

 private:
  QString m_path;
  QString m_name;
  QString m_author;
  QString m_comment;
  QString m_version;
  QColor m_bgColor;
  QColor m_fgColor;

  mutable QMap<QString, QPixmap> m_pixmapMap;

  QPixmap m_emptyPixmap;

  friend class K3bThemeManager;
};


class K3bThemeManager : public QObject
{
  Q_OBJECT

 public:
  K3bThemeManager( QObject* parent = 0, const char* name = 0 );
  ~K3bThemeManager();

  const QPtrList<K3bTheme>& themes() const;

  /**
   * This is never null. If no theme could be found an empty dummy theme
   * will be returnes which does not contains any pixmaps.
   */
  K3bTheme* currentTheme() const;
  K3bTheme* findTheme( const QString& ) const;

  static K3bThemeManager* k3bThemeManager() { return s_k3bThemeManager; }

 signals:
  void themeChanged();
  void themeChanged( K3bTheme* );

 public slots:
  void readConfig( KConfig* );
  void saveConfig( KConfig* );
  void setCurrentTheme( const QString& );
  void setCurrentTheme( K3bTheme* );
  void loadThemes();

 private:
  void loadTheme( const QString& name );

  class Private;
  Private* d;

  static K3bThemeManager* s_k3bThemeManager;
};

#endif
