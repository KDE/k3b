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


#ifndef K3B_PROCESS_H
#define K3B_PROCESS_H


#include <kprocess.h>
#include <qstring.h>


/**
 * This is an enhanced KProcess.
 * It splits the stderr output to lines making sure the client gets every line as it 
 * was written by the process.
 * Aditionally one may set raw stdout and stdin handling using the stdin() and stdout() methods
 * to get the process' file descriptors.
 * Last but not least K3bProcess is able to duplicate stdout making it possible to connect two 
 * K3bProcesses like used in K3bDataJob to duplicate mkisofs' stdout to the stdin of the writer 
 * (cdrecord or cdrdao)
 */
class K3bProcess : public KProcess
{
  Q_OBJECT

 public:
  K3bProcess();
  ~K3bProcess();

  bool start( RunMode run = NotifyOnExit, Communication com = NoCommunication );

  /** 
   * get stdin file descriptor
   * Only makes sense while process is running.
   */
  int stdin() const;

  /** 
   * get stdout file descriptor
   * Only makes sense while process is running.
   */
  int stdout() const;

  /**
   * makes the stdout fd of this process a copy of @p fd
   * closing it first.
   * This means that all this process writes to stdout will directly
   * be written to @p fd.
   * Be aware that you won't get any stdoutReady() or receivedStdout()
   * signals anymore
   * Only use this before starting the process.
   */
  void dupStdout( int fd );

  /** 
   * If set to true one needs to create a socketnotifier on one's own.
   * There will be no wroteStdin() signal
   */
  void setRawStdin(bool b) { m_rawStdin = b; }

  /** 
   * If set to true K3bProcess emits stdoutReady instead of the KProcess receivedStdout() 
   * signal and the data has to read by the user directly from the file descriptor
   */
  void setRawStdout(bool b) { m_rawStdout = b; }

 public slots:
  void setSplitStdout( bool b ) { m_bSplitStdout = b; }
 
  /**
   * default is true
   */
  void setSuppressEmptyLines( bool b ) { m_suppressEmptyLines = b; }

 private slots:
  void slotSplitStderr( KProcess*, char*, int );
  void slotSplitStdout( KProcess*, char*, int );

 signals:
  void stderrLine( const QString& line );
  void stdoutLine( const QString& line );

  /** 
   * Gets emitted if raw stdout mode has been requested
   * The data has to be read from @p fd.
   */
  void stdoutReady( int fd );

 protected:
  /**
   * reimplemeted from KProcess
   */
  int commSetupDoneP();

  /**
   * reimplemeted from KProcess
   */
  int commSetupDoneC();

 private:
  void splitOutput( char*, int, bool );

  QString m_unfinishedStdoutLine;
  QString m_unfinishedStderrLine;
  bool m_bSplitStdout;

  bool m_rawStdin;
  bool m_rawStdout;

  int m_dupStdoutFd;

  bool m_suppressEmptyLines;
};


#endif
