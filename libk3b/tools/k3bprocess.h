/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3B_PROCESS_H
#define K3B_PROCESS_H


#include "k3b_export.h"

#include "k3bkprocess.h"

namespace K3b {
    class ExternalBin;


    /**
     * This is an enhanced K3Process.
     * It splits the stderr output to lines making sure the client gets every line as it
     * was written by the process.
     * Additionally one may set raw stdout and stdin handling using the stdin() and stdout() methods
     * to get the process' file descriptors.
     * Last but not least Process is able to duplicate stdout making it possible to connect two
     * Processes like used in DataJob to duplicate mkisofs' stdout to the stdin of the writer
     * (cdrecord or cdrdao)
     */
    class LIBK3B_EXPORT Process : public K3bKProcess
    {
        Q_OBJECT

    public:
        explicit Process( QObject* parent = 0 );
        ~Process() override;

        /**
         * In the future this might also set the nice value
         */
        Process& operator<<( const ExternalBin* );

        Process& operator<<( const char* arg );
        Process& operator<<( const QByteArray& arg );
        Process& operator<<( const QLatin1String& arg );

        /**
         * returned joined list of program arguments
         */
        QString joinedArgs();

        bool isRunning() const { return state() == QProcess::Running; }

        /**
         * Reimplemented from QProcess.
         * Closes the write channel but does not kill the process
         * as QProcess does.
         */
        void close() override;

        /**
         * Starts the process in \p mode and then waits for it
         * to be started.
         */
        bool start( KProcess::OutputChannelMode mode );

        using K3bKProcess::operator<<;

    public Q_SLOTS:
        void setSplitStdout( bool b );

        /**
         * default is true
         */
        void setSuppressEmptyLines( bool b );

    private Q_SLOTS:
        void slotReadyReadStandardError();
        void slotReadyReadStandardOutput();

    Q_SIGNALS:
        void stderrLine( const QString& line );
        void stdoutLine( const QString& line );

    private:
        class Private;
        Private* const d;
    };
}

#endif
