/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *                    Klaus-Dieter Krannich <kd@k3b.org>
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


#ifndef K3B_CDRDAO_WRITER_H
#define K3B_CDRDAO_WRITER_H


#include "k3babstractwriter.h"
#include "k3b_export.h"

class K3bExternalBin;
class K3bProcess;
class K3Process;
class K3bDevice::Device;
class Q3Socket;



class LIBK3B_EXPORT K3bCdrdaoWriter : public K3bAbstractWriter
{
  Q_OBJECT

 public:

  enum Command { WRITE, COPY, READ, BLANK };
  enum BlankMode { FULL, MINIMAL };
  enum SubMode { None, RW, RW_RAW };

  K3bCdrdaoWriter( K3bDevice::Device* dev, K3bJobHandler*, 
		   QObject* parent = 0 );
  ~K3bCdrdaoWriter();

  /**
   * to be used in chain: addArgument(x)->addArgument(y)
   */
  K3bCdrdaoWriter* addArgument( const QString& );
  K3bDevice::Device* sourceDevice() { return m_sourceDevice; };

  int fd() const;

  bool active() const;

 private:
  void reinitParser();
  void parseCdrdaoLine( const QString& line );
  void parseCdrdaoWrote( const QString& line );
  void parseCdrdaoError( const QString& line ); 

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

  void setSourceDevice( K3bDevice::Device* dev ) { m_sourceDevice = dev; }
  void setFastToc( bool b ) { m_fastToc = b; }
  void setReadRaw( bool b ) { m_readRaw = b; }
  void setReadSubchan(SubMode m) { m_readSubchan=m; };
  void setParanoiaMode( int i ) { m_paranoiaMode = i; }
  void setTaoSource(bool b) { m_taoSource=b; };
  void setTaoSourceAdjust(int a) { m_taoSourceAdjust=a; };
  void setSession(int s) { m_session=s; };
  void setEject(bool e) { m_eject=e; };  
// ---------------------

 private slots:
  void slotStdLine( const QString& line );
  void slotProcessExited(K3Process*);
  void parseCdrdaoMessage();
  void slotThroughput( int t );

 private:
  void unknownCdrdaoLine( const QString& );
  void prepareArgumentList();
  void setWriteArguments();
  void setReadArguments();
  void setCopyArguments();
  void setBlankArguments();
  void setCommonArguments();

  bool cueSheet();

  QString findDriverFile( const K3bExternalBin* bin );
  bool defaultToGenericMMC( K3bDevice::Device* dev, bool writer );

  // options
  // ---------------------
  int        m_command;
  int        m_blankMode;
  K3bDevice::Device* m_sourceDevice;
  QString    m_dataFile;
  QString    m_tocFile;
  QString    m_cueFileLnk;
  QString    m_binFileLnk;
  QString m_backupTocFile;
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
  bool       m_eject;
  // ---------------------

  const K3bExternalBin* m_cdrdaoBinObject;
  K3bProcess* m_process;

  int m_cdrdaoComm[2];
  Q3Socket         *m_comSock;

  bool m_canceled;

  bool m_knownError;

// parser

  int m_size;
  int m_currentTrack;

  class Private;
  Private* d;
};

#endif
