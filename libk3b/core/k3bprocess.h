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
#include <qprocess.h>

#include "k3b_export.h"

namespace K3b {
    class ExternalBin;


    /**
     * This is an enhanced K3Process.
     * It splits the stderr output to lines making sure the client gets every line as it
     * was written by the process.
     * Aditionally one may set raw stdout and stdin handling using the stdin() and stdout() methods
     * to get the process' file descriptors.
     * Last but not least Process is able to duplicate stdout making it possible to connect two
     * Processes like used in DataJob to duplicate mkisofs' stdout to the stdin of the writer
     * (cdrecord or cdrdao)
     */
    class LIBK3B_EXPORT Process : public K3Process
    {
        Q_OBJECT

    public:
        Process();
        ~Process();

        /**
         * In the future this might also set the nice value
         */
        Process& operator<<( const ExternalBin* );

        Process& operator<<( const QString& arg );
        Process& operator<<( const char* arg );
        Process& operator<<( const QByteArray& arg );
        Process& operator<<( const QStringList& args );

        bool start( Communication com );

        /**
         * get stdin file descriptor
         * Only makes sense while process is running.
         *
         * Only use with setRawStdin
         */
        int stdinFd() const;

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
         * close stdin channel
         *
         * Once this class is ported to use KProcess instead of K3Process this
         * method can be deleted and the QProcess::closeWriteChannel() can be
         * called directly.
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

        /**
         * returned joined list of program arguments
         */
        QString joinedArgs();

    public Q_SLOTS:
        void setSplitStdout( bool b ) { m_bSplitStdout = b; }

        /**
         * default is true
         */
        void setSuppressEmptyLines( bool b );

    private Q_SLOTS:
        void slotSplitStderr( K3Process*, char*, int );
        void slotSplitStdout( K3Process*, char*, int );
        void slotProcessExited( K3Process * );

    Q_SIGNALS:
        void stderrLine( const QString& line );
        void stdoutLine( const QString& line );

        /**
         * the same as QProcess::finished()
         */
        void finished( int exitCode, QProcess::ExitStatus exitStatus );

    private:
        static QStringList splitOutput( char*, int, QString&, bool );

        class Data;
        Data* d;

        bool m_bSplitStdout;
    };
}

#endif
