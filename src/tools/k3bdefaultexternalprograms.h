/***************************************************************************
 *   Copyright (C) 2002 by Sebastian Trueg                                 *
 *   trueg@k3b.org                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/


#ifndef _K3B_DEFAULT_EXTERNAL_BIN_PROGRAMS_H_
#define _K3B_DEFAULT_EXTERNAL_BIN_PROGRAMS_H_

#include "k3bexternalbinmanager.h"

class K3bExternalBinManager;


void addDefaultPrograms( K3bExternalBinManager* );

class K3bCdrecordProgram : public K3bExternalProgram
{
 public:
  K3bCdrecordProgram();

  bool scan( const QString& );
};


class K3bMkisofsProgram : public K3bExternalProgram
{
 public:
  K3bMkisofsProgram();

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

 private:
  QString m_transcodeProgram;
};


class K3bEMovixProgram : public K3bExternalProgram
{
 public:
  K3bEMovixProgram();

  bool scan( const QString& );
};


class K3bVcdbuilderProgram : public K3bExternalProgram
{
 public:
  K3bVcdbuilderProgram( const QString& );

  bool scan( const QString& );

 private:
  QString m_vcdbuilderProgram;
};


#endif
