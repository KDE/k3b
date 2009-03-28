/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

//#define QPROCESS_DEBUG

#if defined QPROCESS_DEBUG
#include <qdebug.h>
#include <qstring.h>
#include <ctype.h>
#if !defined(Q_OS_WINCE)
#include <errno.h>
#endif

//QT_BEGIN_NAMESPACE
/*
    Returns a human readable representation of the first \a len
    characters in \a data.
*/
static QByteArray qt_prettyDebug(const char *data, int len, int maxSize)
{
    if (!data) return "(null)";
    QByteArray out;
    for (int i = 0; i < len && i < maxSize; ++i) {
        char c = data[i];
        if (isprint(c)) {
            out += c;
        } else switch (c) {
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            char buf[5];
            qsnprintf(buf, sizeof(buf), "\\%3o", c);
            buf[4] = '\0';
            out += QByteArray(buf);
        }
    }

    if (len < maxSize)
        out += "...";

    return out;
}

//QT_END_NAMESPACE

#endif

#include "k3bqprocess.h"
#include "k3bqprocess_p.h"

#include <qbytearray.h>
#include <qdatetime.h>
#include <qcoreapplication.h>
#include <qsocketnotifier.h>
#include <qtimer.h>

#ifdef Q_WS_WIN
#include <private/qwineventnotifier_p.h>
#endif

#ifndef QT_NO_PROCESS

//QT_BEGIN_NAMESPACE

void K3bQProcessPrivate::Channel::clear()
{
    switch (type) {
    case PipeSource:
        Q_ASSERT(process);
        process->stdinChannel.type = Normal;
        process->stdinChannel.process = 0;
        break;
    case PipeSink:
        Q_ASSERT(process);
        process->stdoutChannel.type = Normal;
        process->stdoutChannel.process = 0;
        break;
    }

    type = Normal;
    file.clear();
    process = 0;
}

/*!
    \class QProcess

    \brief The QProcess class is used to start external programs and
    to communicate with them.

    \ingroup io
    \ingroup misc
    \mainclass
    \reentrant

    To start a process, pass the name and command line arguments of
    the program you want to run as arguments to start(). For example:

    \snippet doc/src/snippets/qprocess/qprocess-simpleexecution.cpp 0
    \dots
    \snippet doc/src/snippets/qprocess/qprocess-simpleexecution.cpp 1
    \snippet doc/src/snippets/qprocess/qprocess-simpleexecution.cpp 2

    QProcess then enters the \l Starting state, and when the program
    has started, QProcess enters the \l Running state and emits
    started().

    QProcess allows you to treat a process as a sequential I/O
    device. You can write to and read from the process just as you
    would access a network connection using QTcpSocket. You can then
    write to the process's standard input by calling write(), and
    read the standard output by calling read(), readLine(), and
    getChar(). Because it inherits QIODevice, QProcess can also be
    used as an input source for QXmlReader, or for generating data to
    be uploaded using QFtp.

    \note On Windows CE, reading and writing to a process is not supported.

    When the process exits, QProcess reenters the \l NotRunning state
    (the initial state), and emits finished().

    The finished() signal provides the exit code and exit status of
    the process as arguments, and you can also call exitCode() to
    obtain the exit code of the last process that finished, and
    exitStatus() to obtain its exit status. If an error occurs at
    any point in time, QProcess will emit the error() signal. You
    can also call error() to find the type of error that occurred
    last, and state() to find the current process state.

    \section1 Communicating via Channels

    Processes have two predefined output channels: The standard
    output channel (\c stdout) supplies regular console output, and
    the standard error channel (\c stderr) usually supplies the
    errors that are printed by the process. These channels represent
    two separate streams of data. You can toggle between them by
    calling setReadChannel(). QProcess emits readyRead() when data is
    available on the current read channel. It also emits
    readyReadStandardOutput() when new standard output data is
    available, and when new standard error data is available,
    readyReadStandardError() is emitted. Instead of calling read(),
    readLine(), or getChar(), you can explicitly read all data from
    either of the two channels by calling readAllStandardOutput() or
    readAllStandardError().

    The terminology for the channels can be misleading. Be aware that
    the process's output channels correspond to QProcess's
    \e read channels, whereas the process's input channels correspond
    to QProcess's \e write channels. This is because what we read
    using QProcess is the process's output, and what we write becomes
    the process's input.

    QProcess can merge the two output channels, so that standard
    output and standard error data from the running process both use
    the standard output channel. Call setProcessChannelMode() with
    MergedChannels before starting the process to activative
    this feature. You also have the option of forwarding the output of
    the running process to the calling, main process, by passing
    ForwardedChannels as the argument.

    Certain processes need special environment settings in order to
    operate. You can set environment variables for your process by
    calling setEnvironment(). To set a working directory, call
    setWorkingDirectory(). By default, processes are run in the
    current working directory of the calling process.

    \section1 Synchronous Process API

    QProcess provides a set of functions which allow it to be used
    without an event loop, by suspending the calling thread until
    certain signals are emitted:

    \list
    \o waitForStarted() blocks until the process has started.

    \o waitForReadyRead() blocks until new data is
    available for reading on the current read channel.

    \o waitForBytesWritten() blocks until one payload of
    data has been written to the process.

    \o waitForFinished() blocks until the process has finished.
    \endlist

    Calling these functions from the main thread (the thread that
    calls QApplication::exec()) may cause your user interface to
    freeze.

    The following example runs \c gzip to compress the string "Qt
    rocks!", without an event loop:

    \snippet doc/src/snippets/process/process.cpp 0

    \section1 Notes for Windows Users

    Some Windows commands (for example, \c dir) are not provided by
    separate applications, but by the command interpreter itself.
    If you attempt to use QProcess to execute these commands directly,
    it won't work. One possible solution is to execute the command
    interpreter itself (\c{cmd.exe} on some Windows systems), and ask
    the interpreter to execute the desired command.

    \sa QBuffer, QFile, QTcpSocket
*/

/*!
    \enum QProcess::ProcessChannel

    This enum describes the process channels used by the running process.
    Pass one of these values to setReadChannel() to set the
    current read channel of QProcess.

    \value StandardOutput The standard output (stdout) of the running
           process.

    \value StandardError The standard error (stderr) of the running
           process.

    \sa setReadChannel()
*/

/*!
    \enum QProcess::ProcessChannelMode

    This enum describes the process channel modes of QProcess. Pass
    one of these values to setProcessChannelMode() to set the
    current read channel mode.

    \value SeparateChannels QProcess manages the output of the
    running process, keeping standard output and standard error data
    in separate internal buffers. You can select the QProcess's
    current read channel by calling setReadChannel(). This is the
    default channel mode of QProcess.

    \value MergedChannels QProcess merges the output of the running
    process into the standard output channel (\c stdout). The
    standard error channel (\c stderr) will not receive any data. The
    standard output and standard error data of the running process
    are interleaved.

    \value ForwardedChannels QProcess forwards the output of the
    running process onto the main process. Anything the child process
    writes to its standard output and standard error will be written
    to the standard output and standard error of the main process.

    \sa setReadChannelMode()
*/

/*!
    \enum QProcess::ProcessError

    This enum describes the different types of errors that are
    reported by QProcess.

    \value FailedToStart The process failed to start. Either the
    invoked program is missing, or you may have insufficient
    permissions to invoke the program.

    \value Crashed The process crashed some time after starting
    successfully.

    \value Timedout The last waitFor...() function timed out. The
    state of QProcess is unchanged, and you can try calling
    waitFor...() again.

    \value WriteError An error occurred when attempting to write to the
    process. For example, the process may not be running, or it may
    have closed its input channel.

    \value ReadError An error occurred when attempting to read from
    the process. For example, the process may not be running.

    \value UnknownError An unknown error occurred. This is the default
    return value of error().

    \sa error()
*/

/*!
    \enum QProcess::ProcessState

    This enum describes the different states of QProcess.

    \value NotRunning The process is not running.

    \value Starting The process is starting, but the program has not
    yet been invoked.

    \value Running The process is running and is ready for reading and
    writing.

    \sa state()
*/

/*!
    \enum QProcess::ExitStatus

    This enum describes the different exit statuses of QProcess.

    \value NormalExit The process exited normally.

    \value CrashExit The process crashed.

    \sa exitStatus()
*/

/*!
    \fn void QProcess::error(QProcess::ProcessError error)

    This signal is emitted when an error occurs with the process. The
    specified \a error describes the type of error that occurred.
*/

/*!
    \fn void QProcess::started()

    This signal is emitted by QProcess when the process has started,
    and state() returns \l Running.
*/

/*!
    \fn void QProcess::stateChanged(QProcess::ProcessState newState)

    This signal is emitted whenever the state of QProcess changes. The
    \a newState argument is the state QProcess changed to.
*/

/*!
    \fn void QProcess::finished(int exitCode)
    \obsolete
    \overload

    Use finished(int exitCode, QProcess::ExitStatus status) instead.
*/

/*!
    \fn void QProcess::finished(int exitCode, QProcess::ExitStatus exitStatus)

    This signal is emitted when the process finishes. \a exitCode is the exit
    code of the process, and \a exitStatus is the exit status.  After the
    process has finished, the buffers in QProcess are still intact. You can
    still read any data that the process may have written before it finished.

    \sa exitStatus()
*/

/*!
    \fn void QProcess::readyReadStandardOutput()

    This signal is emitted when the process has made new data
    available through its standard output channel (\c stdout). It is
    emitted regardless of the current \l{readChannel()}{read channel}.

    \sa readAllStandardOutput(), readChannel()
*/

/*!
    \fn void QProcess::readyReadStandardError()

    This signal is emitted when the process has made new data
    available through its standard error channel (\c stderr). It is
    emitted regardless of the current \l{readChannel()}{read
    channel}.

    \sa readAllStandardError(), readChannel()
*/

/*! \internal
*/
K3bQProcessPrivate::K3bQProcessPrivate()
{
    processChannel = ::QProcess::StandardOutput;
    processChannelMode = ::QProcess::SeparateChannels;
    processError = ::QProcess::UnknownError;
    processState = ::QProcess::NotRunning;
    pid = 0;
    sequenceNumber = 0;
    exitCode = 0;
    exitStatus = ::QProcess::NormalExit;
    startupSocketNotifier = 0;
    deathNotifier = 0;
    notifier = 0;
    pipeWriter = 0;
    childStartedPipe[0] = INVALID_Q_PIPE;
    childStartedPipe[1] = INVALID_Q_PIPE;
    deathPipe[0] = INVALID_Q_PIPE;
    deathPipe[1] = INVALID_Q_PIPE;
    exitCode = 0;
    crashed = false;
    dying = false;
    emittedReadyRead = false;
    emittedBytesWritten = false;
#ifdef Q_WS_WIN
    pipeWriter = 0;
    processFinishedNotifier = 0;
#endif // Q_WS_WIN
#ifdef Q_OS_UNIX
    serial = 0;
#endif
}

/*! \internal
*/
K3bQProcessPrivate::~K3bQProcessPrivate()
{
    if (stdinChannel.process)
        stdinChannel.process->stdoutChannel.clear();
    if (stdoutChannel.process)
        stdoutChannel.process->stdinChannel.clear();
}

/*! \internal
*/
void K3bQProcessPrivate::cleanup()
{
    q_func()->setProcessState(::QProcess::NotRunning);
#ifdef Q_OS_WIN
    if (pid) {
        CloseHandle(pid->hThread);
        CloseHandle(pid->hProcess);
        delete pid;
        pid = 0;
    }
    if (processFinishedNotifier) {
        processFinishedNotifier->setEnabled(false);
        qDeleteInEventHandler(processFinishedNotifier);
        processFinishedNotifier = 0;
    }

#endif
    pid = 0;
    sequenceNumber = 0;
    dying = false;

    if (stdoutChannel.notifier) {
        stdoutChannel.notifier->setEnabled(false);
        delete stdoutChannel.notifier;
        stdoutChannel.notifier = 0;
    }
    if (stderrChannel.notifier) {
        stderrChannel.notifier->setEnabled(false);
        delete stderrChannel.notifier;
        stderrChannel.notifier = 0;
    }
    if (stdinChannel.notifier) {
        stdinChannel.notifier->setEnabled(false);
        delete stdinChannel.notifier;
        stdinChannel.notifier = 0;
    }
    if (startupSocketNotifier) {
        startupSocketNotifier->setEnabled(false);
        delete startupSocketNotifier;
        startupSocketNotifier = 0;
    }
    if (deathNotifier) {
        deathNotifier->setEnabled(false);
        delete deathNotifier;
        deathNotifier = 0;
    }
    if (notifier) {
        delete notifier;
        notifier = 0;
    }
    destroyPipe(stdoutChannel.pipe);
    destroyPipe(stderrChannel.pipe);
    destroyPipe(stdinChannel.pipe);
    destroyPipe(childStartedPipe);
    destroyPipe(deathPipe);
#ifdef Q_OS_UNIX
    serial = 0;
#endif
}

/*! \internal
*/
bool K3bQProcessPrivate::_q_canReadStandardOutput()
{
    Q_Q(K3bQProcess);
    qint64 available = bytesAvailableFromStdout();
    if (available == 0) {
        if (stdoutChannel.notifier)
            stdoutChannel.notifier->setEnabled(false);
        destroyPipe(stdoutChannel.pipe);
#if defined QPROCESS_DEBUG
        qDebug("K3bQProcessPrivate::canReadStandardOutput(), 0 bytes available");
#endif
        return false;
    }

    if (!(processFlags & K3bQProcess::RawStdout)) {
        char *ptr = outputReadBuffer.reserve(available);
        qint64 readBytes = readFromStdout(ptr, available);
        if (readBytes == -1) {
            processError = ::QProcess::ReadError;
            q->setErrorString(K3bQProcess::tr("Error reading from process"));
            emit q->error(processError);
#if defined QPROCESS_DEBUG
            qDebug("K3bQProcessPrivate::canReadStandardOutput(), failed to read from the process");
#endif
            return false;
        }
#if defined QPROCESS_DEBUG
        qDebug("K3bQProcessPrivate::canReadStandardOutput(), read %d bytes from the process' output",
               int(readBytes));
#endif

        if (stdoutChannel.closed) {
            outputReadBuffer.chop(readBytes);
            return false;
        }

        outputReadBuffer.chop(available - readBytes);

        bool didRead = false;
        if (readBytes == 0) {
            if (stdoutChannel.notifier)
                stdoutChannel.notifier->setEnabled(false);
        } else if (processChannel == ::QProcess::StandardOutput) {
            didRead = true;
            if (!emittedReadyRead) {
                emittedReadyRead = true;
                emit q->readyRead();
                emittedReadyRead = false;
            }
        }
        emit q->readyReadStandardOutput();
        return didRead;
    }
    else {
        if (!emittedReadyRead) {
            emittedReadyRead = true;
            emit q->readyRead();
            emittedReadyRead = false;
        }
        emit q->readyReadStandardOutput();
        return true;
    }
}

/*! \internal
*/
bool K3bQProcessPrivate::_q_canReadStandardError()
{
    Q_Q(K3bQProcess);
    qint64 available = bytesAvailableFromStderr();
    if (available == 0) {
        if (stderrChannel.notifier)
            stderrChannel.notifier->setEnabled(false);
        destroyPipe(stderrChannel.pipe);
        return false;
    }

    char *ptr = errorReadBuffer.reserve(available);
    qint64 readBytes = readFromStderr(ptr, available);
    if (readBytes == -1) {
        processError = ::QProcess::ReadError;
        q->setErrorString(K3bQProcess::tr("Error reading from process"));
        emit q->error(processError);
        return false;
    }
    if (stderrChannel.closed) {
        errorReadBuffer.chop(readBytes);
        return false;
    }

    errorReadBuffer.chop(available - readBytes);

    bool didRead = false;
    if (readBytes == 0) {
        if (stderrChannel.notifier)
            stderrChannel.notifier->setEnabled(false);
    } else if (processChannel == ::QProcess::StandardError) {
        didRead = true;
        if (!emittedReadyRead) {
            emittedReadyRead = true;
            emit q->readyRead();
            emittedReadyRead = false;
        }
    }
    emit q->readyReadStandardError();
    return didRead;
}

/*! \internal
*/
bool K3bQProcessPrivate::_q_canWrite()
{
    Q_Q(K3bQProcess);
    if (processFlags & K3bQProcess::RawStdin) {
        if (stdinChannel.notifier)
            stdinChannel.notifier->setEnabled(false);
        isReadyWrite = true;
        emit q->readyWrite();
    }
    else {
        if (stdinChannel.notifier)
            stdinChannel.notifier->setEnabled(false);

        if (writeBuffer.isEmpty()) {
#if defined QPROCESS_DEBUG
            qDebug("K3bQProcessPrivate::canWrite(), not writing anything (empty write buffer).");
#endif
            return false;
        }

        qint64 written = writeToStdin(writeBuffer.readPointer(),
                                      writeBuffer.nextDataBlockSize());
        if (written < 0) {
            destroyPipe(stdinChannel.pipe);
            processError = ::QProcess::WriteError;
            q->setErrorString(K3bQProcess::tr("Error writing to process"));
#if defined(QPROCESS_DEBUG) && !defined(Q_OS_WINCE)
            qDebug("K3bQProcessPrivate::canWrite(), failed to write (%s)", strerror(errno));
#endif
            emit q->error(processError);
            return false;
        }

#if defined QPROCESS_DEBUG
        qDebug("K3bQProcessPrivate::canWrite(), wrote %d bytes to the process input", int(written));
#endif

        writeBuffer.free(written);
        if (!emittedBytesWritten) {
            emittedBytesWritten = true;
            emit q->bytesWritten(written);
            emittedBytesWritten = false;
        }
        if (stdinChannel.notifier && !writeBuffer.isEmpty())
            stdinChannel.notifier->setEnabled(true);
        if (writeBuffer.isEmpty() && stdinChannel.closed)
            closeWriteChannel();
    }
    return true;
}

/*! \internal
*/
bool K3bQProcessPrivate::_q_processDied()
{
    Q_Q(K3bQProcess);
#if defined QPROCESS_DEBUG
    qDebug("K3bQProcessPrivate::_q_processDied()");
#endif
#ifdef Q_OS_UNIX
    if (!waitForDeadChild())
        return false;
#endif
#ifdef Q_OS_WIN
    if (processFinishedNotifier)
        processFinishedNotifier->setEnabled(false);
#endif

    // the process may have died before it got a chance to report that it was
    // either running or stopped, so we will call _q_startupNotification() and
    // give it a chance to emit started() or error(FailedToStart).
    if (processState == ::QProcess::Starting) {
        if (!_q_startupNotification())
            return true;
    }

    if (dying) {
        // at this point we know the process is dead. prevent
        // reentering this slot recursively by calling waitForFinished()
        // or opening a dialog inside slots connected to the readyRead
        // signals emitted below.
        return true;
    }
    dying = true;

    // in case there is data in the pipe line and this slot by chance
    // got called before the read notifications, call these two slots
    // so the data is made available before the process dies.
    if ( processFlags&K3bQProcess::RawStdout ) {
        // wait for all data to be read
        if ( bytesAvailableFromStdout() > 0 ) {
            QMetaObject::invokeMethod( q, "_q_processDied", Qt::QueuedConnection );
            return false;
        }
    }
    else {
        _q_canReadStandardOutput();
    }
    _q_canReadStandardError();

    findExitCode();

    if (crashed) {
        exitStatus = ::QProcess::CrashExit;
        processError = ::QProcess::Crashed;
        q->setErrorString(K3bQProcess::tr("Process crashed"));
        emit q->error(processError);
    }

    bool wasRunning = (processState == ::QProcess::Running);

    cleanup();

    if (wasRunning) {
        // we received EOF now:
        emit q->readChannelFinished();
        // in the future:
        //emit q->standardOutputClosed();
        //emit q->standardErrorClosed();

        emit q->finished(exitCode);
        emit q->finished(exitCode, exitStatus);
    }
#if defined QPROCESS_DEBUG
    qDebug("K3bQProcessPrivate::_q_processDied() process is dead");
#endif
    return true;
}

/*! \internal
*/
bool K3bQProcessPrivate::_q_startupNotification()
{
    Q_Q(K3bQProcess);
#if defined QPROCESS_DEBUG
    qDebug("K3bQProcessPrivate::startupNotification()");
#endif

    if (startupSocketNotifier)
        startupSocketNotifier->setEnabled(false);
    if (processStarted()) {
        q->setProcessState(::QProcess::Running);
        emit q->started();
        return true;
    }

    q->setProcessState(::QProcess::NotRunning);
    processError = ::QProcess::FailedToStart;
    emit q->error(processError);
#ifdef Q_OS_UNIX
    // make sure the process manager removes this entry
    waitForDeadChild();
    findExitCode();
#endif
    cleanup();
    return false;
}

/*! \internal
*/
void K3bQProcessPrivate::closeWriteChannel()
{
#if defined QPROCESS_DEBUG
    qDebug("K3bQProcessPrivate::closeWriteChannel()");
#endif
    if (stdinChannel.notifier) {
        stdinChannel.notifier->setEnabled(false);
        if (stdinChannel.notifier) {
            delete stdinChannel.notifier;
            stdinChannel.notifier = 0;
        }
    }
#ifdef Q_OS_WIN
    // ### Find a better fix, feeding the process little by little
    // instead.
    flushPipeWriter();
#endif
    destroyPipe(stdinChannel.pipe);
}

qint64 K3bQProcessPrivate::readData( char *data, qint64 maxlen, QProcess::ProcessChannel channel )
{
    if (processFlags&K3bQProcess::RawStdout &&
        channel == ::QProcess::StandardOutput) {
        return readFromStdout(data, maxlen);
    }
    else {
        QRingBuffer *readBuffer = (channel == ::QProcess::StandardError)
                                  ? &errorReadBuffer
                                  : &outputReadBuffer;

        if (maxlen == 1 && !readBuffer->isEmpty()) {
            int c = readBuffer->getChar();
            if (c == -1) {
#if defined QPROCESS_DEBUG
                qDebug("QProcess::readData(%p \"%s\", %d) == -1",
                       data, qt_prettyDebug(data, 1, maxlen).constData(), 1);
#endif
                return -1;
            }
            *data = (char) c;
#if defined QPROCESS_DEBUG
            qDebug("QProcess::readData(%p \"%s\", %d) == 1",
                   data, qt_prettyDebug(data, 1, maxlen).constData(), 1);
#endif
            return 1;
        }

        qint64 bytesToRead = qint64(qMin(readBuffer->size(), (int)maxlen));
        qint64 readSoFar = 0;
        while (readSoFar < bytesToRead) {
            const char *ptr = readBuffer->readPointer();
            int bytesToReadFromThisBlock = qMin<qint64>(bytesToRead - readSoFar,
                                                        readBuffer->nextDataBlockSize());
            memcpy(data + readSoFar, ptr, bytesToReadFromThisBlock);
            readSoFar += bytesToReadFromThisBlock;
            readBuffer->free(bytesToReadFromThisBlock);
        }

#if defined QPROCESS_DEBUG
        qDebug("QProcess::readData(%p \"%s\", %lld) == %lld",
               data, qt_prettyDebug(data, readSoFar, 16).constData(), maxlen, readSoFar);
#endif
        if (!readSoFar && processState == ::QProcess::NotRunning)
            return -1;              // EOF
        return readSoFar;
    }
}

/*!
    Constructs a QProcess object with the given \a parent.
*/
K3bQProcess::K3bQProcess(QObject *parent)
    : QIODevice(parent),
      d_ptr( new K3bQProcessPrivate )
{
    d_ptr->q_ptr = this;
//#if defined QPROCESS_DEBUG
    qDebug("K3bQProcess::QProcess(%p)", parent);
//#endif
}

/*!
    Destructs the QProcess object, i.e., killing the process.

    Note that this function will not return until the process is
    terminated.
*/
K3bQProcess::~K3bQProcess()
{
    Q_D(K3bQProcess);
    if (d->processState != ::QProcess::NotRunning) {
        qWarning("QProcess: Destroyed while process is still running.");
        kill();
        waitForFinished();
    }
#ifdef Q_OS_UNIX
    // make sure the process manager removes this entry
    d->findExitCode();
#endif
    d->cleanup();
    delete d;
}

K3bQProcess::ProcessFlags K3bQProcess::flags() const
{
    Q_D(const K3bQProcess);
    return d->processFlags;
}

void K3bQProcess::setFlags( K3bQProcess::ProcessFlags flags )
{
    Q_D(K3bQProcess);
    d->processFlags = flags;
}

/*!
    \obsolete
    Returns the read channel mode of the QProcess. This function is
    equivalent to processChannelMode()

    \sa processChannelMode()
*/
::QProcess::ProcessChannelMode K3bQProcess::readChannelMode() const
{
    return processChannelMode();
}

/*!
    \obsolete

    Use setProcessChannelMode(\a mode) instead.

    \sa setProcessChannelMode()
*/
void K3bQProcess::setReadChannelMode(::QProcess::ProcessChannelMode mode)
{
    setProcessChannelMode(mode);
}

/*!
    \since 4.2

    Returns the channel mode of the QProcess standard output and
    standard error channels.

    \sa setReadChannelMode(), ProcessChannelMode, setReadChannel()
*/
::QProcess::ProcessChannelMode K3bQProcess::processChannelMode() const
{
    Q_D(const K3bQProcess);
    return d->processChannelMode;
}

/*!
    \since 4.2

    Sets the channel mode of the QProcess standard output and standard
    error channels to the \a mode specified.
    This mode will be used the next time start() is called. For example:

    \snippet doc/src/snippets/code/src_corelib_io_qprocess.cpp 0

    \sa readChannelMode(), ProcessChannelMode, setReadChannel()
*/
void K3bQProcess::setProcessChannelMode(::QProcess::ProcessChannelMode mode)
{
    Q_D(K3bQProcess);
    d->processChannelMode = mode;
}

/*!
    Returns the current read channel of the QProcess.

    \sa setReadChannel()
*/
QProcess::ProcessChannel K3bQProcess::readChannel() const
{
    Q_D(const K3bQProcess);
    return d->processChannel;
}

/*!
    Sets the current read channel of the QProcess to the given \a
    channel. The current input channel is used by the functions
    read(), readAll(), readLine(), and getChar(). It also determines
    which channel triggers QProcess to emit readyRead().

    \sa readChannel()
*/
void K3bQProcess::setReadChannel(::QProcess::ProcessChannel channel)
{
    Q_D(K3bQProcess);
//     if (d->processChannel != channel) {
//         QByteArray buf = d->buffer.readAll();
//         if (d->processChannel == QProcess::StandardOutput) {
//             for (int i = buf.size() - 1; i >= 0; --i)
//                 d->outputReadBuffer.ungetChar(buf.at(i));
//         } else {
//             for (int i = buf.size() - 1; i >= 0; --i)
//                 d->errorReadBuffer.ungetChar(buf.at(i));
//         }
//     }
    d->processChannel = channel;
}

/*!
    Closes the read channel \a channel. After calling this function,
    QProcess will no longer receive data on the channel. Any data that
    has already been received is still available for reading.

    Call this function to save memory, if you are not interested in
    the output of the process.

    \sa closeWriteChannel(), setReadChannel()
*/
void K3bQProcess::closeReadChannel(::QProcess::ProcessChannel channel)
{
    Q_D(K3bQProcess);

    if (channel == ::QProcess::StandardOutput) {
        d->stdoutChannel.closed = true;
        if ( d->processFlags&RawStdout )
            d->destroyPipe(d->stdoutChannel.pipe);
    }
    else
        d->stderrChannel.closed = true;
}

/*!
    Schedules the write channel of QProcess to be closed. The channel
    will close once all data has been written to the process. After
    calling this function, any attempts to write to the process will
    fail.

    Closing the write channel is necessary for programs that read
    input data until the channel has been closed. For example, the
    program "more" is used to display text data in a console on both
    Unix and Windows. But it will not display the text data until
    QProcess's write channel has been closed. Example:

    \snippet doc/src/snippets/code/src_corelib_io_qprocess.cpp 1

    The write channel is implicitly opened when start() is called.

    \sa closeReadChannel()
*/
void K3bQProcess::closeWriteChannel()
{
    Q_D(K3bQProcess);
    d->stdinChannel.closed = true; // closing
    if (d->writeBuffer.isEmpty())
        d->closeWriteChannel();
}

/*!
    \since 4.2

    Redirects the process' standard input to the file indicated by \a
    fileName. When an input redirection is in place, the QProcess
    object will be in read-only mode (calling write() will result in
    error).

    If the file \a fileName does not exist at the moment start() is
    called or is not readable, starting the process will fail.

    Calling setStandardInputFile() after the process has started has no
    effect.

    \sa setStandardOutputFile(), setStandardErrorFile(),
        setStandardOutputProcess()
*/
void K3bQProcess::setStandardInputFile(const QString &fileName)
{
    Q_D(K3bQProcess);
    d->stdinChannel = fileName;
}

/*!
    \since 4.2

    Redirects the process' standard output to the file \a
    fileName. When the redirection is in place, the standard output
    read channel is closed: reading from it using read() will always
    fail, as will readAllStandardOutput().

    If the file \a fileName doesn't exist at the moment start() is
    called, it will be created. If it cannot be created, the starting
    will fail.

    If the file exists and \a mode is QIODevice::Truncate, the file
    will be truncated. Otherwise (if \a mode is QIODevice::Append),
    the file will be appended to.

    Calling setStandardOutputFile() after the process has started has
    no effect.

    \sa setStandardInputFile(), setStandardErrorFile(),
        setStandardOutputProcess()
*/
void K3bQProcess::setStandardOutputFile(const QString &fileName, OpenMode mode)
{
    Q_ASSERT(mode == Append || mode == Truncate);
    Q_D(K3bQProcess);

    d->stdoutChannel = fileName;
    d->stdoutChannel.append = mode == Append;
}

/*!
    \since 4.2

    Redirects the process' standard error to the file \a
    fileName. When the redirection is in place, the standard error
    read channel is closed: reading from it using read() will always
    fail, as will readAllStandardError(). The file will be appended to
    if \a mode is Append, otherwise, it will be truncated.

    See setStandardOutputFile() for more information on how the file
    is opened.

    Note: if setProcessChannelMode() was called with an argument of
    QProcess::MergedChannels, this function has no effect.

    \sa setStandardInputFile(), setStandardOutputFile(),
        setStandardOutputProcess()
*/
void K3bQProcess::setStandardErrorFile(const QString &fileName, OpenMode mode)
{
    Q_ASSERT(mode == Append || mode == Truncate);
    Q_D(K3bQProcess);

    d->stderrChannel = fileName;
    d->stderrChannel.append = mode == Append;
}

/*!
    \since 4.2

    Pipes the standard output stream of this process to the \a
    destination process' standard input.

    The following shell command:
    \snippet doc/src/snippets/code/src_corelib_io_qprocess.cpp 2

    Can be accomplished with QProcesses with the following code:
    \snippet doc/src/snippets/code/src_corelib_io_qprocess.cpp 3
*/
void K3bQProcess::setStandardOutputProcess(K3bQProcess *destination)
{
    K3bQProcessPrivate *dfrom = d_func();
    K3bQProcessPrivate *dto = destination->d_func();
    dfrom->stdoutChannel.pipeTo(dto);
    dto->stdinChannel.pipeFrom(dfrom);
}

/*!
    If QProcess has been assigned a working directory, this function returns
    the working directory that the QProcess will enter before the program has
    started. Otherwise, (i.e., no directory has been assigned,) an empty
    string is returned, and QProcess will use the application's current
    working directory instead.

    \sa setWorkingDirectory()
*/
QString K3bQProcess::workingDirectory() const
{
    Q_D(const K3bQProcess);
    return d->workingDirectory;
}

/*!
    Sets the working directory to \a dir. QProcess will start the
    process in this directory. The default behavior is to start the
    process in the working directory of the calling process.

    \sa workingDirectory(), start()
*/
void K3bQProcess::setWorkingDirectory(const QString &dir)
{
    Q_D(K3bQProcess);
    d->workingDirectory = dir;
}

/*!
    Returns the native process identifier for the running process, if
    available.  If no process is currently running, 0 is returned.
*/
Q_PID K3bQProcess::pid() const
{
    Q_D(const K3bQProcess);
    return d->pid;
}

/*! \reimp

    This function operates on the current read channel.

    \sa readChannel(), setReadChannel()
*/
bool K3bQProcess::canReadLine() const
{
    Q_D(const K3bQProcess);
    const QRingBuffer *readBuffer = (d->processChannel == ::QProcess::StandardError)
                                    ? &d->errorReadBuffer
                                    : &d->outputReadBuffer;
    return readBuffer->canReadLine() || QIODevice::canReadLine();
}

/*!
    Closes all communication with the process and kills it. After calling this
    function, QProcess will no longer emit readyRead(), and data can no
    longer be read or written.
*/
void K3bQProcess::close()
{
    emit aboutToClose();
    while (waitForBytesWritten(-1))
        ;
    kill();
    waitForFinished(-1);
    QIODevice::close();
}

/*! \reimp

   Returns true if the process is not running, and no more data is available
   for reading; otherwise returns false.
*/
bool K3bQProcess::atEnd() const
{
    Q_D(const K3bQProcess);
    const QRingBuffer *readBuffer = (d->processChannel == ::QProcess::StandardError)
                                    ? &d->errorReadBuffer
                                    : &d->outputReadBuffer;
    return QIODevice::atEnd() && (!isOpen() || readBuffer->isEmpty());
}

/*! \reimp
*/
bool K3bQProcess::isSequential() const
{
    return true;
}

/*! \reimp
*/
qint64 K3bQProcess::bytesAvailable() const
{
    Q_D(const K3bQProcess);
    const QRingBuffer *readBuffer = (d->processChannel == ::QProcess::StandardError)
                                    ? &d->errorReadBuffer
                                    : &d->outputReadBuffer;
#if defined QPROCESS_DEBUG
    qDebug("QProcess::bytesAvailable() == %i (%s)", readBuffer->size(),
           (d->processChannel == ::QProcess::StandardError) ? "stderr" : "stdout");
#endif
    return readBuffer->size() + QIODevice::bytesAvailable();
}

/*! \reimp
*/
qint64 K3bQProcess::bytesToWrite() const
{
    Q_D(const K3bQProcess);
    qint64 size = d->writeBuffer.size();
#ifdef Q_OS_WIN
    size += d->pipeWriterBytesToWrite();
#endif
    return size;
}

/*!
    Returns the type of error that occurred last.

    \sa state()
*/
::QProcess::ProcessError K3bQProcess::error() const
{
    Q_D(const K3bQProcess);
    return d->processError;
}

/*!
    Returns the current state of the process.

    \sa stateChanged(), error()
*/
::QProcess::ProcessState K3bQProcess::state() const
{
    Q_D(const K3bQProcess);
    return d->processState;
}

/*!
    Sets the environment that QProcess will use when starting a process to the
    \a environment specified which consists of a list of key=value pairs.

    For example, the following code adds the \c{C:\\BIN} directory to the list of
    executable paths (\c{PATHS}) on Windows:

    \snippet doc/src/snippets/qprocess-environment/main.cpp 0

    \sa environment(), systemEnvironment()
*/
void K3bQProcess::setEnvironment(const QStringList &environment)
{
    Q_D(K3bQProcess);
    d->environment = environment;
}

/*!
    Returns the environment that QProcess will use when starting a
    process, or an empty QStringList if no environment has been set
    using setEnvironment(). If no environment has been set, the
    environment of the calling process will be used.

    \note The environment settings are ignored on Windows CE,
    as there is no concept of an environment.

    \sa setEnvironment(), systemEnvironment()
*/
QStringList K3bQProcess::environment() const
{
    Q_D(const K3bQProcess);
    return d->environment;
}

/*!
    Blocks until the process has started and the started() signal has
    been emitted, or until \a msecs milliseconds have passed.

    Returns true if the process was started successfully; otherwise
    returns false (if the operation timed out or if an error
    occurred).

    This function can operate without an event loop. It is
    useful when writing non-GUI applications and when performing
    I/O operations in a non-GUI thread.

    \warning Calling this function from the main (GUI) thread
    might cause your user interface to freeze.

    If msecs is -1, this function will not time out.

    \sa started(), waitForReadyRead(), waitForBytesWritten(), waitForFinished()
*/
bool K3bQProcess::waitForStarted(int msecs)
{
    Q_D(K3bQProcess);
    if (d->processState == ::QProcess::Starting) {
        if (!d->waitForStarted(msecs))
            return false;
        setProcessState(::QProcess::Running);
        emit started();
    }
    return d->processState == ::QProcess::Running;
}

/*! \reimp
*/
bool K3bQProcess::waitForReadyRead(int msecs)
{
    Q_D(K3bQProcess);

    if (d->processState == ::QProcess::NotRunning)
        return false;
    if (d->processChannel == ::QProcess::StandardOutput && d->stdoutChannel.closed)
        return false;
    if (d->processChannel == ::QProcess::StandardError && d->stderrChannel.closed)
        return false;
    return d->waitForReadyRead(msecs);
}

/*! \reimp
*/
bool K3bQProcess::waitForBytesWritten(int msecs)
{
    Q_D(K3bQProcess);
    if (d->processState == ::QProcess::NotRunning)
        return false;
    if (d->processState == ::QProcess::Starting) {
        QTime stopWatch;
        stopWatch.start();
        bool started = waitForStarted(msecs);
        if (!started)
            return false;
        if (msecs != -1)
            msecs -= stopWatch.elapsed();
    }

    return d->waitForBytesWritten(msecs);
}

/*!
    Blocks until the process has finished and the finished() signal
    has been emitted, or until \a msecs milliseconds have passed.

    Returns true if the process finished; otherwise returns false (if
    the operation timed out or if an error occurred).

    This function can operate without an event loop. It is
    useful when writing non-GUI applications and when performing
    I/O operations in a non-GUI thread.

    \warning Calling this function from the main (GUI) thread
    might cause your user interface to freeze.

    If msecs is -1, this function will not time out.

    \sa finished(), waitForStarted(), waitForReadyRead(), waitForBytesWritten()
*/
bool K3bQProcess::waitForFinished(int msecs)
{
    Q_D(K3bQProcess);
    if (d->processState == ::QProcess::NotRunning)
        return false;
    if (d->processState == ::QProcess::Starting) {
        QTime stopWatch;
        stopWatch.start();
        bool started = waitForStarted(msecs);
        if (!started)
            return false;
        if (msecs != -1)
            msecs -= stopWatch.elapsed();
    }

    return d->waitForFinished(msecs);
}

/*!
    Sets the current state of the QProcess to the \a state specified.

    \sa state()
*/
void K3bQProcess::setProcessState(::QProcess::ProcessState state)
{
    Q_D(K3bQProcess);
    if (d->processState == state)
        return;
    d->processState = state;
    emit stateChanged(state);
}

/*!
  This function is called in the child process context just before the
    program is executed on Unix or Mac OS X (i.e., after \e fork(), but before
    \e execve()). Reimplement this function to do last minute initialization
    of the child process. Example:

    \snippet doc/src/snippets/code/src_corelib_io_qprocess.cpp 4

    You cannot exit the process (by calling exit(), for instance) from
    this function. If you need to stop the program before it starts
    execution, your workaround is to emit finished() and then call
    exit().

    \warning This function is called by QProcess on Unix and Mac OS X
    only. On Windows, it is not called.
*/
void K3bQProcess::setupChildProcess()
{
}

/*! \reimp
*/
qint64 K3bQProcess::readData(char *data, qint64 maxlen)
{
    Q_D(K3bQProcess);
    return d->readData( data, maxlen, d->processChannel );
}

/*! \reimp
*/
qint64 K3bQProcess::writeData(const char *data, qint64 len)
{
    Q_D(K3bQProcess);

#if defined(Q_OS_WINCE)
    Q_UNUSED(data);
    Q_UNUSED(len);
    d->processError = ::QProcess::WriteError;
    setErrorString(tr("Error writing to process"));
    emit error(d->processError);
    return -1;
#endif

    if (d->stdinChannel.closed) {
#if defined QPROCESS_DEBUG
    qDebug("QProcess::writeData(%p \"%s\", %lld) == 0 (write channel closing)",
           data, qt_prettyDebug(data, len, 16).constData(), len);
#endif
        return 0;
    }

    if (d->processFlags & K3bQProcess::RawStdin) {
        d->waitForBytesWritten();
        qint64 r = d->writeToStdin(data, len);
        if ( r > 0 )
            emit bytesWritten(r);
        return r;
    }
    else {
        if (len == 1) {
            d->writeBuffer.putChar(*data);
            if (d->stdinChannel.notifier)
                d->stdinChannel.notifier->setEnabled(true);
#if defined QPROCESS_DEBUG
            qDebug("QProcess::writeData(%p \"%s\", %lld) == 1 (written to buffer)",
                   data, qt_prettyDebug(data, len, 16).constData(), len);
#endif
            return 1;
        }

        char *dest = d->writeBuffer.reserve(len);
        memcpy(dest, data, len);
        if (d->stdinChannel.notifier)
            d->stdinChannel.notifier->setEnabled(true);
#if defined QPROCESS_DEBUG
        qDebug("QProcess::writeData(%p \"%s\", %lld) == %lld (written to buffer)",
               data, qt_prettyDebug(data, len, 16).constData(), len, len);
#endif
        return len;
    }
}

/*!
    Regardless of the current read channel, this function returns all
    data available from the standard output of the process as a
    QByteArray.

    \sa readyReadStandardOutput(), readAllStandardError(), readChannel(), setReadChannel()
*/
QByteArray K3bQProcess::readAllStandardOutput()
{
    Q_D(K3bQProcess);
    if (!(d->processFlags&RawStdout)) {
        ::QProcess::ProcessChannel tmp = readChannel();
        setReadChannel(::QProcess::StandardOutput);
        QByteArray data = readAll();
        setReadChannel(tmp);
        return data;
    }
    else {
        return QByteArray();
    }
}

/*!
    Regardless of the current read channel, this function returns all
    data available from the standard error of the process as a
    QByteArray.

    \sa readyReadStandardError(), readAllStandardOutput(), readChannel(), setReadChannel()
*/
QByteArray K3bQProcess::readAllStandardError()
{
    Q_D(K3bQProcess);
    if (d->processFlags&RawStdout) {
        //
        // HACK: this is an ugly hack to get around the following problem:
        // K3b uses QProcess from different threads. This is no problem unless
        // the read channel is changed here while the other thread tries to read
        // from stdout. It will then result in two reads from stderr instead
        // (this one and the other thread which originally wanted to read from
        // stdout).
        // The "solution" atm is to reimplement QIODevice::readAll here, ignoring its
        // buffer (no real problem since K3b::Process is always opened Unbuffered)
        //
        QByteArray tmp;
        tmp.resize(int(d->errorReadBuffer.size()));
        qint64 readBytes = d->readData(tmp.data(), tmp.size(), QProcess::StandardError);
        tmp.resize(readBytes < 0 ? 0 : int(readBytes));
        return tmp;
    }
    else {
        ::QProcess::ProcessChannel tmp = readChannel();
        setReadChannel(::QProcess::StandardError);
        QByteArray data = readAll();
        setReadChannel(tmp);
        return data;
    }
}

/*!
    Starts the program \a program in a new process, passing the
    command line arguments in \a arguments. The OpenMode is set to \a
    mode. QProcess will immediately enter the Starting state. If the
    process starts successfully, QProcess will emit started();
    otherwise, error() will be emitted.

    Note that arguments that contain spaces are not passed to the
    process as separate arguments.

    \bold{Windows:} Arguments that contain spaces are wrapped in quotes.

    \note Processes are started asynchronously, which means the started()
    and error() signals may be delayed. Call waitForStarted() to make
    sure the process has started (or has failed to start) and those signals
    have been emitted.

    \sa pid(), started(), waitForStarted()
*/
void K3bQProcess::start(const QString &program, const QStringList &arguments, OpenMode mode)
{
    Q_D(K3bQProcess);
    if (d->processState != ::QProcess::NotRunning) {
        qWarning("QProcess::start: Process is already running");
        return;
    }

#if defined QPROCESS_DEBUG
    qDebug() << "QProcess::start(" << program << "," << arguments << "," << mode << ")";
#endif

    d->outputReadBuffer.clear();
    d->errorReadBuffer.clear();

    d->isReadyWrite = false;

    if (d->stdinChannel.type != K3bQProcessPrivate::Channel::Normal)
        mode &= ~WriteOnly;     // not open for writing
    if (d->stdoutChannel.type != K3bQProcessPrivate::Channel::Normal &&
        (d->stderrChannel.type != K3bQProcessPrivate::Channel::Normal ||
         d->processChannelMode == ::QProcess::MergedChannels))
        mode &= ~ReadOnly;      // not open for reading
    if (mode == 0)
        mode = Unbuffered;
    QIODevice::open(mode);

    d->stdinChannel.closed = false;
    d->stdoutChannel.closed = false;
    d->stderrChannel.closed = false;

    d->program = program;
    d->arguments = arguments;

    d->exitCode = 0;
    d->exitStatus = ::QProcess::NormalExit;
    d->processError = ::QProcess::UnknownError;
    setErrorString( QString() );
    d->startProcess();
}


static QStringList parseCombinedArgString(const QString &program)
{
    QStringList args;
    QString tmp;
    int quoteCount = 0;
    bool inQuote = false;

    // handle quoting. tokens can be surrounded by double quotes
    // "hello world". three consecutive double quotes represent
    // the quote character itself.
    for (int i = 0; i < program.size(); ++i) {
        if (program.at(i) == QLatin1Char('"')) {
            ++quoteCount;
            if (quoteCount == 3) {
                // third consecutive quote
                quoteCount = 0;
                tmp += program.at(i);
            }
            continue;
        }
        if (quoteCount) {
            if (quoteCount == 1)
                inQuote = !inQuote;
            quoteCount = 0;
        }
        if (!inQuote && program.at(i).isSpace()) {
            if (!tmp.isEmpty()) {
                args += tmp;
                tmp.clear();
            }
        } else {
            tmp += program.at(i);
        }
    }
    if (!tmp.isEmpty())
        args += tmp;

    return args;
}

/*!
    \overload

    Starts the program \a program in a new process. \a program is a
    single string of text containing both the program name and its
    arguments. The arguments are separated by one or more
    spaces. For example:

    \snippet doc/src/snippets/code/src_corelib_io_qprocess.cpp 5

    The \a program string can also contain quotes, to ensure that arguments
    containing spaces are correctly supplied to the new process. For example:

    \snippet doc/src/snippets/code/src_corelib_io_qprocess.cpp 6

    Note that, on Windows, quotes need to be both escaped and quoted.
    For example, the above code would be specified in the following
    way to ensure that \c{"My Documents"} is used as the argument to
    the \c dir executable:

    \snippet doc/src/snippets/code/src_corelib_io_qprocess.cpp 7

    The OpenMode is set to \a mode.
*/
void K3bQProcess::start(const QString &program, OpenMode mode)
{
    QStringList args = parseCombinedArgString(program);

    QString prog = args.first();
    args.removeFirst();

    start(prog, args, mode);
}

/*!
    Attempts to terminate the process.

    The process may not exit as a result of calling this function (it is given
    the chance to prompt the user for any unsaved files, etc).

    On Windows, terminate() posts a WM_CLOSE message to all toplevel windows
    of the process and then to the main thread of the process itself. On Unix
    and Mac OS X the SIGTERM signal is sent.

    Console applications on Windows that do not run an event loop, or whose
    event loop does not handle the WM_CLOSE message, can only be terminated by
    calling kill().

    \sa kill()
*/
void K3bQProcess::terminate()
{
    Q_D(K3bQProcess);
    d->terminateProcess();
}

/*!
    Kills the current process, causing it to exit immediately.

    On Windows, kill() uses TerminateProcess, and on Unix and Mac OS X, the
    SIGKILL signal is sent to the process.

    \sa terminate()
*/
void K3bQProcess::kill()
{
    Q_D(K3bQProcess);
    d->killProcess();
}

/*!
    Returns the exit code of the last process that finished.
*/
int K3bQProcess::exitCode() const
{
    Q_D(const K3bQProcess);
    return d->exitCode;
}

/*!
    \since 4.1

    Returns the exit status of the last process that finished.

    On Windows, if the process was terminated with TerminateProcess()
    from another application this function will still return NormalExit
    unless the exit code is less than 0.
*/
::QProcess::ExitStatus K3bQProcess::exitStatus() const
{
    Q_D(const K3bQProcess);
    return d->exitStatus;
}

/*!
    Starts the program \a program with the arguments \a arguments in a
    new process, waits for it to finish, and then returns the exit
    code of the process. Any data the new process writes to the
    console is forwarded to the calling process.

    The environment and working directory are inherited by the calling
    process.

    On Windows, arguments that contain spaces are wrapped in quotes.
*/
int K3bQProcess::execute(const QString &program, const QStringList &arguments)
{
    QProcess process;
    process.setReadChannelMode(::QProcess::ForwardedChannels);
    process.start(program, arguments);
    process.waitForFinished(-1);
    return process.exitCode();
}

/*!
    \overload

    Starts the program \a program in a new process. \a program is a
    single string of text containing both the program name and its
    arguments. The arguments are separated by one or more spaces.
*/
int K3bQProcess::execute(const QString &program)
{
    QProcess process;
    process.setReadChannelMode(::QProcess::ForwardedChannels);
    process.start(program);
    process.waitForFinished(-1);
    return process.exitCode();
}

/*!
    Starts the program \a program with the arguments \a arguments in a
    new process, and detaches from it. Returns true on success;
    otherwise returns false. If the calling process exits, the
    detached process will continue to live.

    Note that arguments that contain spaces are not passed to the
    process as separate arguments.

    \bold{Unix:} The started process will run in its own session and act
    like a daemon.

    \bold{Windows:} Arguments that contain spaces are wrapped in quotes.
    The started process will run as a regular standalone process.

    The process will be started in the directory \a workingDirectory.

    If the function is successful then *\a pid is set to the process
    identifier of the started process.
*/
bool K3bQProcess::startDetached(const QString &program,
			     const QStringList &arguments,
			     const QString &workingDirectory,
                             qint64 *pid)
{
    return K3bQProcessPrivate::startDetached(program,
					  arguments,
					  workingDirectory,
					  pid);
}

/*!
    Starts the program \a program with the given \a arguments in a
    new process, and detaches from it. Returns true on success;
    otherwise returns false. If the calling process exits, the
    detached process will continue to live.

    Note that arguments that contain spaces are not passed to the
    process as separate arguments.

    \bold{Unix:} The started process will run in its own session and act
    like a daemon.

    \bold{Windows:} Arguments that contain spaces are wrapped in quotes.
    The started process will run as a regular standalone process.
*/
bool K3bQProcess::startDetached(const QString &program,
			     const QStringList &arguments)
{
    return K3bQProcessPrivate::startDetached(program, arguments);
}

/*!
    \overload

    Starts the program \a program in a new process. \a program is a
    single string of text containing both the program name and its
    arguments. The arguments are separated by one or more spaces.

    The \a program string can also contain quotes, to ensure that arguments
    containing spaces are correctly supplied to the new process.
*/
bool K3bQProcess::startDetached(const QString &program)
{
    QStringList args = parseCombinedArgString(program);

    QString prog = args.first();
    args.removeFirst();

    return K3bQProcessPrivate::startDetached(prog, args);
}

QT_BEGIN_INCLUDE_NAMESPACE
#ifdef Q_OS_MAC
# include <crt_externs.h>
# define environ (*_NSGetEnviron())
#elif defined(Q_OS_WINCE)
  static char *qt_wince_environ[] = { 0 };
#define environ qt_wince_environ
#elif !defined(Q_OS_WIN)
  extern char **environ;
#endif
QT_END_INCLUDE_NAMESPACE

/*!
    \since 4.1

    Returns the environment of the calling process as a list of
    key=value pairs. Example:

    \snippet doc/src/snippets/code/src_corelib_io_qprocess.cpp 8

    \sa environment(), setEnvironment()
*/
QStringList K3bQProcess::systemEnvironment()
{
    QStringList tmp;
//     char *entry = 0;
//     int count = 0;
//     while ((entry = environ[count++]))
//         tmp << QString::fromLocal8Bit(entry);
    return tmp;
}

bool K3bQProcess::isReadyWrite() const
{
    Q_D(const K3bQProcess);
    return d->isReadyWrite;
}


/*!
    \typedef Q_PID
    \relates QProcess

    Typedef for the identifiers used to represent processes on the underlying
    platform. On Unix, this corresponds to \l qint64; on Windows, it
    corresponds to \c{_PROCESS_INFORMATION*}.

    \sa QProcess::pid()
*/


//QT_END_NAMESPACE

#include "moc_k3bqprocess.cpp"

#endif // QT_NO_PROCESS

