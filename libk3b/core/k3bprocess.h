/* 
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3B_PROCESS_H
#define K3B_PROCESS_H


#include <k3process.h>
#include <qstring.h>
//Added by qt3to4:
#include <Q3CString>
#include "k3b_export.h"

class K3bExternalBin;


/**
 * This is an enhanced K3Process.
 * It splits the stderr output to lines making sure the client gets every line as it 
 * was written by the process.
 * Aditionally one may set raw stdout and stdin handling using the stdin() and stdout() methods
 * to get the process' file descriptors.
 * Last but not least K3bProcess is able to duplicate stdout making it possible to connect two 
 * K3bProcesses like used in K3bDataJob to duplicate mkisofs' stdout to the stdin of the writer 
 * (cdrecord or cdrdao)
 */
class LIBK3B_EXPORT K3bProcess : public K3Process
{
  Q_OBJECT
    
 public:
  class OutputCollector;

 public:
  K3bProcess();
  ~K3bProcess();

  /**
   * In the future this might also set the nice value
   */
  K3bProcess& operator<<( const K3bExternalBin* );

  K3bProcess& operator<<( const QString& arg );
  K3bProcess& operator<<( const char* arg );
  K3bProcess& operator<<( const Q3CString& arg );
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

 public slots:
  void setSplitStdout( bool b ) { m_bSplitStdout = b; }
 
  /**
   * default is true
   */
  void setSuppressEmptyLines( bool b );

  bool closeStdin();
  bool closeStdout();

 private slots:
  void slotSplitStderr( K3Process*, char*, int );
  void slotSplitStdout( K3Process*, char*, int );

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
   * reimplemeted from K3Process
   */
  int commSetupDoneP();

  /**
   * reimplemeted from K3Process
   */
  int commSetupDoneC();

  /**
   * reimplemeted from K3Process
   */
  int setupCommunication( Communication comm );

  /**
   * reimplemeted from K3Process
   */
  void commClose();

 private:
  static QStringList splitOutput( char*, int, QString&, bool );

  class Data;
  Data* d;

  bool m_bSplitStdout;
};

class LIBK3B_EXPORT K3bProcessOutputCollector: public QObject
{
  Q_OBJECT
    
 public:
  K3bProcessOutputCollector( K3Process* );
  void setProcess( K3Process* );
  
  const QString& output() const { return m_gatheredOutput; }
  const QString& stderrOutput() const { return m_stderrOutput; }
  const QString& stdoutOutput() const { return m_stdoutOutput; }
  
 private slots:
  void slotGatherStderr( K3Process*, char*, int );
  void slotGatherStdout( K3Process*, char*, int );
  
 private:
  QString m_gatheredOutput;
  QString m_stderrOutput;
  QString m_stdoutOutput;
  K3Process* m_process;
};


#endif
