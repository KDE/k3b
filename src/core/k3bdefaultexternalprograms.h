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



#ifndef _K3B_DEFAULT_EXTERNAL_BIN_PROGRAMS_H_
#define _K3B_DEFAULT_EXTERNAL_BIN_PROGRAMS_H_

#include "k3bexternalbinmanager.h"

class K3bExternalBinManager;

namespace K3b
{
  void addDefaultPrograms( K3bExternalBinManager* );
}


class K3bCdrecordProgram : public K3bExternalProgram
{
 public:
  K3bCdrecordProgram( bool dvdPro );

  bool scan( const QString& );

 private:
  bool m_dvdPro;
};


class K3bDvdrecordProgram : public K3bExternalProgram
{
 public:
  K3bDvdrecordProgram();

  bool scan( const QString& );
};


class K3bMkisofsProgram : public K3bExternalProgram
{
 public:
  K3bMkisofsProgram();

  bool scan( const QString& );
};


class K3bReadcdProgram : public K3bExternalProgram
{
 public:
  K3bReadcdProgram();

  bool scan( const QString& );
};


class K3bCdrdaoProgram : public K3bExternalProgram
{
 public:
  K3bCdrdaoProgram();

  bool scan( const QString& );
};


class K3bTranscodeProgram : public K3bExternalProgram
{
 public:
  K3bTranscodeProgram( const QString& transcodeProgram );

  bool scan( const QString& );

  // no user parameters (yet)
  bool supportsUserParameters() const { return false; }

 private:
  QString m_transcodeProgram;
};


class K3bVcdbuilderProgram : public K3bExternalProgram
{
 public:
  K3bVcdbuilderProgram( const QString& );

  bool scan( const QString& );

 private:
  QString m_vcdbuilderProgram;
};


class K3bNormalizeProgram : public K3bExternalProgram
{
 public:
  K3bNormalizeProgram();

  bool scan( const QString& );
};


class K3bGrowisofsProgram : public K3bExternalProgram
{
 public:
  K3bGrowisofsProgram();

  bool scan( const QString& );
};


class K3bDvdformatProgram : public K3bExternalProgram
{
 public:
  K3bDvdformatProgram();

  bool scan( const QString& );
};


class K3bCdda2wavProgram : public K3bExternalProgram
{
 public:
  K3bCdda2wavProgram();

  bool scan( const QString& );
};

#endif
