/* 
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3B_PROCESS_H
#define K3B_PROCESS_H


#include "k3process.h"
#include <qstring.h>

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
    K3bProcess();
    ~K3bProcess();

    /**
     * In the future this might also set the nice value
     */
    K3bProcess& operator<<( const K3bExternalBin* );

    K3bProcess& operator<<( const QString& arg );
    K3bProcess& operator<<( const char* arg );
    K3bProcess& operator<<( const QByteArray& arg );
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
     * close stdin channel
     *
     * Once this class is ported to use KProcess instead of K3Process this
     * method can be deleted and the QProcess::closeWriteChannel() can be
     * called directly.
     *
     * This is similar to closeStdin() but it will also close a dup'ed channel.
     */
    void closeWriteChannel();

    /**
     * wait until process exited
     *
     * Once this class is ported to use KProcess instead of K3Process this
     * method can be deleted and the QProcess::waitForFinished() can be
     * called directly.
     *
     * The timeout value MUST be -1 for now as everything else is not implemented.
     */
    bool waitForFinished(int timeout);

    /**
     * write data to stdin
     *
     * Once this class is ported to use KProcess instead of K3Process this
     * method can be deleted and the QProcess::write() can be called directly.
     */
    qint64 write(const char * data, qint64 maxSize);

public Q_SLOTS:
    void setSplitStdout( bool b ) { m_bSplitStdout = b; }

    /**
     * default is true
     */
    void setSuppressEmptyLines( bool b );

    bool closeStdin();
    bool closeStdout();

    private Q_SLOTS:
    void slotSplitStderr( K3Process*, char*, int );
    void slotSplitStdout( K3Process*, char*, int );

Q_SIGNALS:
    void stderrLine( const QString& line );
    void stdoutLine( const QString& line );

    /** 
     * Gets emitted if raw stdout mode has been requested
     * The data has to be read from @p fd.
     */
    void stdoutReady( int fd );

private:
    static QStringList splitOutput( char*, int, QString&, bool );

    class Data;
    Data* d;

    bool m_bSplitStdout;
};

#endif
