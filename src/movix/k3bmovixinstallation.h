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


#ifndef _K3B_MOVIX_INSTALLTION_H_
#define _K3B_MOVIX_INSTALLTION_H_

#include <qstringlist.h>


class K3bMovixInstallation
{
 public:
  ~K3bMovixInstallation();

  /** path to the eMovix installation */
  const QString& path() const { return m_path; }
  const QStringList& supportedBootLabels() const { return m_supportedBootLabels; }
  const QStringList& supportedSubtitleFonts() const { return m_supportedSubtitleFonts; }
  const QStringList& supportedLanguages() const { return m_supportedLanguages; }

  /**
   * returns 0 if not every necessary files could be found
   */
  static K3bMovixInstallation* probeInstallation( const QString& path );

 private:
  K3bMovixInstallation( const QString& path );

  QString m_path;
  QStringList m_supportedBootLabels;
  QStringList m_supportedSubtitleFonts;
  QStringList m_supportedLanguages;
};

#endif
