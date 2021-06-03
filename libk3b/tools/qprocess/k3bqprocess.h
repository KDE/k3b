/*
    SPDX-FileCopyrightText: 2009 Nokia Corporation and /or its subsidiary(-ies).
    Contact: Qt Software Information (qt-info@nokia.com)

    This file is part of the QtCore module of the Qt Toolkit.

    $QT_BEGIN_LICENSE:LGPL$
    Commercial Usage
    Licensees holding valid Qt Commercial licenses may use this file in
    accordance with the Qt Commercial License Agreement provided with the
    Software or, alternatively, in accordance with the terms contained in
    a written agreement between you and Nokia.

    GNU Lesser General Public License Usage
    Alternatively, this file may be used under the terms of the GNU Lesser
    General Public License version 2.1 as published by the Free Software
    Foundation and appearing in the file LICENSE.LGPL included in the
    packaging of this file.  Please review the following information to
    ensure the GNU Lesser General Public License version 2.1 requirements
    will be met: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.

    In addition, as a special exception, Nokia gives you certain
    additional rights. These rights are described in the Nokia Qt LGPL
    Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
    package.

    GNU General Public License Usage
    Alternatively, this file may be used under the terms of the GNU
    General Public License version 3.0 as published by the Free Software
    Foundation and appearing in the file LICENSE.GPL included in the
    packaging of this file.  Please review the following information to
    ensure the GNU General Public License version 3.0 requirements will be
    met: https://www.gnu.org/licenses/gpl-3.0.html.

    If you are unsure which license is appropriate for your use, please
    contact the sales department at qt-sales@nokia.com.
    $QT_END_LICENSE$

*/

#ifndef K3B_QPROCESS_H
#define K3B_QPROCESS_H

#include <qiodevice.h>
#include <QStringList>
#include <qprocess.h>

#include "k3b_export.h"

// QT_BEGIN_HEADER

// QT_BEGIN_NAMESPACE

// QT_MODULE(Core)

#ifndef QT_NO_PROCESS

#if (!defined(Q_OS_WIN32) && !defined(Q_OS_WINCE)) || defined(qdoc)
typedef qint64 Q_PID;
#else
QT_END_NAMESPACE
typedef struct _PROCESS_INFORMATION *Q_PID;
QT_BEGIN_NAMESPACE
#endif

class K3bQProcessPrivate;

class LIBK3B_EXPORT K3bQProcess : public QIODevice
{
    Q_OBJECT
public:
    // BE AWARE: we use the original enums from QProcess to make the future transition of slots easier
    /*
    enum ProcessError {
        FailedToStart, //### file not found, resource error
        Crashed,
        Timedout,
        ReadError,
        WriteError,
        UnknownError
    };
    enum ProcessState {
        NotRunning,
        Starting,
        Running
    };
    enum ProcessChannel {
        StandardOutput,
        StandardError
    };
    enum ProcessChannelMode {
        SeparateChannels,
        MergedChannels,
        ForwardedChannels
    };
    enum ExitStatus {
        NormalExit,
        CrashExit
    };
    */
    enum ProcessFlag {
        NoFlags = 0x0,
        RawStdin = 0x1,
        RawStdout = 0x2
    };
    Q_DECLARE_FLAGS( ProcessFlags, ProcessFlag )

    explicit K3bQProcess(QObject *parent = 0);
    ~K3bQProcess() override;

    void start(const QString &program, const QStringList &arguments, OpenMode mode = ReadWrite);
    void start(const QString &program, OpenMode mode = ReadWrite);

    ::QProcess::ProcessChannelMode readChannelMode() const;
    void setReadChannelMode(::QProcess::ProcessChannelMode mode);
    ::QProcess::ProcessChannelMode processChannelMode() const;
    void setProcessChannelMode(::QProcess::ProcessChannelMode mode);

    ProcessFlags flags() const;
    void setFlags( ProcessFlags flags );

    ::QProcess::ProcessChannel readChannel() const;
    void setReadChannel(::QProcess::ProcessChannel channel);

    void closeReadChannel(::QProcess::ProcessChannel channel);
    void closeWriteChannel();

    void setStandardInputFile(const QString &fileName);
    void setStandardOutputFile(const QString &fileName, OpenMode mode = Truncate);
    void setStandardErrorFile(const QString &fileName, OpenMode mode = Truncate);
    void setStandardOutputProcess(K3bQProcess *destination);

    QString workingDirectory() const;
    void setWorkingDirectory(const QString &dir);

    void setEnvironment(const QStringList &environment);
    QStringList environment() const;

    ::QProcess::ProcessError error() const;
    ::QProcess::ProcessState state() const;

    // #### Qt 5: Q_PID is a pointer on Windows and a value on Unix
    Q_PID pid() const;

    bool waitForStarted(int msecs = 30000);
    bool waitForReadyRead(int msecs = 30000) override;
    bool waitForBytesWritten(int msecs = 30000) override;
    bool waitForFinished(int msecs = 30000);

    QByteArray readAllStandardOutput();
    QByteArray readAllStandardError();

    int exitCode() const;
    ::QProcess::ExitStatus exitStatus() const;

    // QIODevice
    qint64 bytesAvailable() const override;
    qint64 bytesToWrite() const override;
    bool isSequential() const override;
    bool canReadLine() const override;
    void close() override;
    bool atEnd() const override;

    bool isReadyWrite() const;

    static int execute(const QString &program, const QStringList &arguments);
    static int execute(const QString &program);

    static bool startDetached(const QString &program, const QStringList &arguments, const QString &workingDirectory,
                              qint64 *pid = 0);
    static bool startDetached(const QString &program, const QStringList &arguments);
    static bool startDetached(const QString &program);

    static QStringList systemEnvironment();

public Q_SLOTS:
    void terminate();
    void kill();

Q_SIGNALS:
    void started();
    void finished(int exitCode);
    void finished(int exitCode, QProcess::ExitStatus exitStatus);
    void error(QProcess::ProcessError error);
    void stateChanged(QProcess::ProcessState state);

    void readyReadStandardOutput();
    void readyReadStandardError();
    void readyWrite();

protected:
    void setProcessState(QProcess::ProcessState state);

    virtual void setupChildProcess();

    // QIODevice
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    Q_DECLARE_PRIVATE(K3bQProcess)
    Q_DISABLE_COPY(K3bQProcess)

    K3bQProcessPrivate* d_ptr;

    Q_PRIVATE_SLOT(d_func(), bool _q_canReadStandardOutput())
    Q_PRIVATE_SLOT(d_func(), bool _q_canReadStandardError())
    Q_PRIVATE_SLOT(d_func(), bool _q_canWrite())
    Q_PRIVATE_SLOT(d_func(), bool _q_startupNotification())
    Q_PRIVATE_SLOT(d_func(), bool _q_processDied())
    Q_PRIVATE_SLOT(d_func(), bool _q_notifyProcessDied())
    Q_PRIVATE_SLOT(d_func(), void _q_notified())
    friend class K3bQProcessManager;
};

#endif // QT_NO_PROCESS

// QT_END_NAMESPACE

// QT_END_HEADER

Q_DECLARE_OPERATORS_FOR_FLAGS( K3bQProcess::ProcessFlags )

#endif // QPROCESS_H
