/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
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


class K3bExternalBin;
class QEvent;


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

  /**
   * In the future this might also set the nice value
   */
  K3bProcess& operator<<( const K3bExternalBin* );

  K3bProcess& operator<<( const QString& arg );
  K3bProcess& operator<<( const char* arg );
  K3bProcess& operator<<( const QCString& arg );
  K3bProcess& operator<<( const QStringList& args );

  bool start( RunMode run = NotifyOnExit, Communication com = NoCommunication );

  /** 
   * get stdin file descriptor
   * Only makes sense while process is running.
   *
   * Only use with setRawStdin
   */
  int stdinFd() const;

  /** 
   * get stdout file descriptor
   * Only makes sense while process is running.
   *
   * Only use with setRawStdout
   */
  int stdoutFd() const;

  /**
   * @deprecated use writeToFd
   */
  void dupStdout( int fd );

  /**
   * @deprecated use readFromFd
   */
  void dupStdin( int fd );

  /**
   * Make the process write to @fd instead of Stdout.
   * This means you won't get any stdoutReady() or receivedStdout()
   * signals anymore.
   *
   * Only use this before starting the process.
   */
  void writeToFd( int fd );

  /**
   * Make the process read from @fd instead of Stdin.
   * This means you won't get any wroteStdin()
   * signals anymore.
   *
   * Only use this before starting the process.
   */
  void readFromFd( int fd );

  /** 
   * If set true the process' stdin fd will be available
   * through @stdinFd.
   * Be aware that you will not get any wroteStdin signals
   * anymore.
   *
   * Only use this before starting the process.
   */
  void setRawStdin(bool b);

  /** 
   * If set true the process' stdout fd will be available
   * through @stdoutFd.
   * Be aware that you will not get any stdoutReady or receivedStdout
   * signals anymore.
   *
   * Only use this before starting the process.
   */
  void setRawStdout(bool b);

  /**
   * Process stderr in another thread to make it non-blocking.
   */
  void setThreadedStderr(bool b);

  bool event( QEvent* );

  class OutputCollector;

 public slots:
  void setSplitStdout( bool b ) { m_bSplitStdout = b; }
 
  /**
   * default is true
   */
  void setSuppressEmptyLines( bool b );

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

  /**
   * reimplemeted from KProcess
   */
  int setupCommunication( Communication comm );

  /**
   * reimplemeted from KProcess
   */
  void commClose();

 private:
  void handleStderrQueue();
  static QStringList splitOutput( char*, int, QString&, bool );

  class Data;
  Data* d;
  class StderrThread;
  friend class StderrThread;
  StderrThread* m_stderrThread;

  bool m_bSplitStdout;
};


class K3bProcess::OutputCollector : public QObject
{
  Q_OBJECT

 public:
  OutputCollector( KProcess* );
  void setProcess( KProcess* );

  const QString& output() const { return m_gatheredOutput; }

 private slots:
  void slotGatherOutput( KProcess*, char*, int );

 private:
  QString m_gatheredOutput;
  KProcess* m_process;
};

#endif
