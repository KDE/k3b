/***************************************************************************
                          k3bcdrdaoreader.h  -  description
                             -------------------
    begin                : Mon Mar 26 15:30:59 CEST 2001
    copyright            : (C) 2001 by Sebastian Trueg and
                                       klaus-Dieter Krannich
    email                : trueg@informatik.uni-freiburg.de
                           kd@math.tu-cottbus.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3B_CDRDAO_READER_H
#define K3B_CDRDAO_READER_H


#include "k3babstractreader.h"
#include "k3bcdrdaoparser.h"
#include <qsocketnotifier.h>

class K3bExternalBin;
class K3bProcess;

class K3bCdrdaoReader : public K3bAbstractReader
{
  Q_OBJECT

 public:
  K3bCdrdaoReader( QObject* parent = 0, const char* name = 0 );
  ~K3bCdrdaoReader();

  /**
   * call this before adding new arguments
   * it will clear the aruments and add device and speed
   * and stuff
   */
  void prepareArgumentList();

  /**
   * to be used in chain: addArgument(x)->addArgument(y)
   */
  K3bCdrdaoReader* addArgument( const QString& );

 public slots:
  void start();
  void cancel();

 private slots:
  void slotStdLine( const QString& line );
  void slotProcessExited(KProcess*);
  void getCdrdaoMessage();

 private:
  const K3bExternalBin* m_cdrdaoBinObject;
  K3bProcess* m_process;

  bool m_rawWrite;

  int m_currentTrack;
  int cdrdaoComm[2];
  QSocketNotifier *qsn;
  K3bCdrdaoParser *m_parser;
};

#endif
