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


#ifndef K3B_CDRECORD_WRITER_H
#define K3B_CDRECORD_WRITER_H


#include "k3babstractwriter.h"

#include <qvaluelist.h>
#include <qstringlist.h>

class K3bExternalBin;
class K3bProcess;
class KProcess;
class K3bCdDevice::CdDevice;


class K3bCdrecordWriter : public K3bAbstractWriter
{
  Q_OBJECT

 public:
  K3bCdrecordWriter( K3bCdDevice::CdDevice*, QObject* parent = 0, const char* name = 0 );
  ~K3bCdrecordWriter();

  bool active() const;

  /**
   * to be used in chain: addArgument(x)->addArgument(y)
   */
  K3bCdrecordWriter* addArgument( const QString& );
  void clearArguments();

  bool write( const char* data, int len );

  int fd() const;

 public slots:
  void start();
  void cancel();

  void setDao( bool b );
  void setProvideStdin( bool b ) { m_stdin = b; }
  void setWritingMode( int );
  void setCueFile( const QString& s);
  void setClone( bool b );

  /**
   * If set true the job ignores the global K3b setting
   * and does not eject the CD-RW after finishing
   */
  void setForceNoEject( bool b ) { m_forceNoEject = b; }

 protected slots:
  void slotStdLine( const QString& line );
  void slotProcessExited(KProcess*);
  void slotThroughput( int t );

 protected:
  virtual void prepareProcess();

  const K3bExternalBin* m_cdrecordBinObject;
  K3bProcess* m_process;

  int m_writingMode;
  bool m_stdin;
  bool m_totalTracksParsed;
  bool m_clone;
  bool m_cue;

  QString m_cueFile;

  enum CdrecordError { UNKNOWN, 
		       OVERSIZE, 
		       BAD_OPTION, 
		       SHMGET_FAILED, 
		       OPC_FAILED,
		       CANNOT_SET_SPEED,
		       CANNOT_SEND_CUE_SHEET,
		       CANNOT_OPEN_NEW_SESSION,
		       PERMISSION_DENIED,
		       BUFFER_UNDERRUN,
		       HIGH_SPEED_MEDIUM };

  QStringList m_arguments;

 private:  
  int m_currentTrack;
  int m_totalTracks;
  int m_totalSize;
  int m_alreadyWritten;

  int m_lastFifoValue;

  int m_cdrecordError;

  QValueList<int> m_trackSizes;

  bool m_forceNoEject;

  class Private;
  Private* d;
};

#endif
