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

#ifndef _K3B_MOVIX_PROGRAM_H_
#define _K3B_MOVIX_PROGRAM_H_

#include <k3bexternalbinmanager.h>


class K3bMovixBin : public K3bExternalBin
{
 public:
  K3bMovixBin( K3bExternalProgram* p )
    : K3bExternalBin( p ) {
  }

  const QStringList& supportedBootLabels() const { return m_supportedBootLabels; }
  const QStringList& supportedSubtitleFonts() const { return m_supportedSubtitleFonts; }
  const QStringList& supportedLanguages() const { return m_supportedLanguages; }
  const QStringList& movixFiles() const { return m_movixFiles; }
  const QStringList& isolinuxFiles() const { return m_isolinuxFiles; }

  /** returnes empty string if font was not found */
  QString subtitleFontDir( const QString& font ) const;
  /** returnes empty string if lang was not found */
  QString languageDir( const QString& lang ) const;

 private:
  QString m_movixPath;
  QStringList m_movixFiles;
  QStringList m_isolinuxFiles;
  QStringList m_supportedBootLabels;
  QStringList m_supportedSubtitleFonts;
  QStringList m_supportedLanguages;

  friend class K3bMovixProgram;
};


class K3bMovixProgram : public K3bExternalProgram
{
 public:
  K3bMovixProgram();

  bool scan( const QString& );

  bool supportsUserParameters() const { return false; }
};



#endif
