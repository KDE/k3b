/* 


    SPDX-FileCopyrightText: 2005 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_MPC_WRAPPER_H_
#define _K3B_MPC_WRAPPER_H_

#include "k3bmpc_config.h"

#include <QString>

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
