/* 
 *
 *
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_MPC_WRAPPER_H_
#define _K3B_MPC_WRAPPER_H_

#include "k3bmpc_config.h"

#include <qstring.h>

#include "k3bmsf.h"

#include MPC_HEADER_FILE

class QFile;


class K3bMpcWrapper
{
 public:
  K3bMpcWrapper();
  ~K3bMpcWrapper();

  bool open( const QString& filename );
  void close();

  int decode( char*, int max );

  bool seek( const K3b::Msf& );

  K3b::Msf length() const;
  int samplerate() const;
  unsigned int channels() const;

  QFile* input() const { return m_input; }

 private:
  QFile* m_input;
  mpc_reader* m_reader;
#ifdef MPC_OLD_API
  mpc_decoder* m_decoder;
#else
  mpc_demux* m_decoder;
#endif
  mpc_streaminfo* m_info;
};

#endif
