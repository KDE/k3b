/***************************************************************************
                          k3bcdrecordwriter.h  -  description
                             -------------------
    begin                : Mon Mar 26 15:30:59 CEST 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3B_CDRECORD_WRITER_H
#define K3B_CDRECORD_WRITER_H


#include "k3babstractwriter.h"

class K3bExternalBin;
class K3bProcess;
class K3bDevice;


class K3bCdrecordWriter : public K3bAbstractWriter
{
  Q_OBJECT

 public:
  K3bCdrecordWriter( K3bDevice*, QObject* parent = 0, const char* name = 0 );
  ~K3bCdrecordWriter();

  /**
   * call this before adding new arguments
   * it will clear the aruments and add device and speed
   * and stuff
   */
  void prepareArgumentList();

  /**
   * to be used in chain: addArgument(x)->addArgument(y)
   */
  K3bCdrecordWriter* addArgument( const QString& );

  bool write( const char* data, int len );

 public slots:
  void start();
  void cancel();

  void setDao( bool b ) { m_dao = b; }
  void setRawWrite( bool b ) { m_rawWrite = b; }
  void setProvideStdin( bool b ) { m_stdin = b; }

 private slots:
  void slotStdLine( const QString& line );
  void slotProcessExited(KProcess*);

 private:
  const K3bExternalBin* m_cdrecordBinObject;
  K3bProcess* m_process;

  bool m_dao;
  bool m_rawWrite;
  bool m_stdin;
  bool m_totalTracksParsed;

  int m_currentTrack;
  int m_totalTracks;
  int m_totalSize;
  int m_trackSize;
  int m_alreadyWritten;

  enum CdrecordError { UNKNOWN, 
		       OVERSIZE, 
		       BAD_OPTION, 
		       SHMGET_FAILED, 
		       OPC_FAILED,
		       CANNOT_SET_SPEED };
  int m_cdrecordError;

  bool m_writeSpeedInitialized;
};

#endif
