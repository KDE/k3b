/***************************************************************************
                          k3bcdrdaowriter.h  -  description
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

#ifndef K3B_CDRDAO_WRITER_H
#define K3B_CDRDAO_WRITER_H


#include "k3babstractwriter.h"
#include "k3bcdrdaoparser.h"
#include <qsocket.h>
#include <qsocketdevice.h>

class K3bExternalBin;
class K3bProcess;
class K3bDevice;


class K3bCdrdaoWriter : public K3bAbstractWriter
{
  Q_OBJECT

 public:

  enum Command { WRITE, COPY, READ, BLANK };
  enum BlankMode { FULL, MINIMAL };
  enum SubMode { None, RW, RW_RAW };

  K3bCdrdaoWriter( K3bDevice* dev, QObject* parent = 0, const char* name = 0 );
  ~K3bCdrdaoWriter();

  /**
   * to be used in chain: addArgument(x)->addArgument(y)
   */
  K3bCdrdaoWriter* addArgument( const QString& );
  K3bDevice* sourceDevice() { return m_sourceDevice; };

 public slots:
  void start();
  void cancel();

  // options
  // ---------------------
  void setCommand( int c ) { m_command = c; }
  void setBlankMode( int b ) { m_blankMode = b; }
  void setMulti( bool b ) { m_multi = b; }
  void setForce( bool b ) { m_force = b; }
  void setOnTheFly( bool b ) { m_onTheFly = b; }
  void setDataFile( const QString& s ) { m_dataFile = s; }
  void setTocFile( const QString& s ) { m_tocFile = s; }

  void setSourceDevice( K3bDevice* dev ) { m_sourceDevice = dev; }
  void setSourceDriver(QString& s) { m_sourceDriver=s; }
  void setFastToc( bool b ) { m_fastToc = b; }
  void setReadRaw( bool b ) { m_readRaw = b; }
  void setReadSubchan(SubMode m) { m_readSubchan=m; };
  void setParanoiaMode( int i ) { m_paranoiaMode = i; }
  void setTaoSource(bool b) { m_taoSource=b; };
  void setTaoSourceAdjust(int a) { m_taoSourceAdjust=a; };
  // ---------------------

  void setProvideStdin( bool b ) { m_stdin = b; }

 private slots:
  void slotStdLine( const QString& line );
  void slotProcessExited(KProcess*);
  void getCdrdaoMessage();
  void slotUnknownCdrdaoLine( const QString& );

 private:
  void prepareArgumentList();
  void setWriteArguments();
  void setReadArguments();
  void setCopyArguments();
  void setBlankArguments();
  void setCommonArguments();

  bool write(const char* data, int len);
  // options
  // ---------------------
  int        m_command;
  int        m_blankMode;
  K3bDevice* m_sourceDevice;
  QString    m_sourceDriver; 
  QString    m_dataFile;
  QString    m_tocFile;
  bool       m_readRaw;
  bool       m_multi;
  bool       m_force;
  bool       m_onTheFly;
  bool       m_fastToc;
  SubMode    m_readSubchan;
  bool       m_taoSource;
  int        m_taoSourceAdjust;
  int        m_paranoiaMode;
  int        m_session;
  // ---------------------

  const K3bExternalBin* m_cdrdaoBinObject;
  K3bProcess* m_process;

  int m_currentTrack;
  int m_cdrdaoComm[2];
  QSocket         *m_comSock;
  K3bCdrdaoParser *m_parser;

  bool m_stdin;
};

#endif
