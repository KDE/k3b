/*
    This file is part of the KDE libraries

    SPDX-FileCopyrightText: 2007 Oswald Buddenhagen <ossi@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "k3bkprocess_p.h"

#include <QStandardPaths>
#include <kshell.h>
#ifdef Q_OS_WIN
# include <kshell_p.h>
#endif

#include <QFile>

#ifdef Q_OS_WIN
# include <windows.h>
#else
# include <unistd.h>
# include <errno.h>
#endif

#ifndef Q_OS_WIN
# define STD_OUTPUT_HANDLE 1
# define STD_ERROR_HANDLE 2
#endif

void K3bKProcessPrivate::writeAll(const QByteArray &buf, int fd)
{
#ifdef Q_OS_WIN
    HANDLE h = GetStdHandle(fd);
    if (h) {
        DWORD wr;
        WriteFile(h, buf.data(), buf.size(), &wr, 0);
    }
#else
    int off = 0;
    do {
        int ret = ::write(fd, buf.data() + off, buf.size() - off);
        if (ret < 0) {
            if (errno != EINTR)
                return;
        } else {
            off += ret;
        }
    } while (off < buf.size());
#endif
}

void K3bKProcessPrivate::forwardStd(::QProcess::ProcessChannel good, int fd)
{
    Q_Q(K3bKProcess);

    QProcess::ProcessChannel oc = q->readChannel();
    q->setReadChannel(good);
    writeAll(q->readAll(), fd);
    q->setReadChannel(oc);
}

void K3bKProcessPrivate::_k_forwardStdout()
{
    forwardStd(QProcess::StandardOutput, STD_OUTPUT_HANDLE);
}

void K3bKProcessPrivate::_k_forwardStderr()
{
    forwardStd(QProcess::StandardError, STD_ERROR_HANDLE);
}

/////////////////////////////
// public member functions //
/////////////////////////////

K3bKProcess::K3bKProcess(QObject *parent) :
    K3bQProcess(parent),
    d_ptr(new K3bKProcessPrivate)
{
    d_ptr->q_ptr = this;
    setOutputChannelMode(KProcess::ForwardedChannels);
}

K3bKProcess::K3bKProcess(K3bKProcessPrivate *d, QObject *parent) :
    K3bQProcess(parent),
    d_ptr(d)
{
    d_ptr->q_ptr = this;
    setOutputChannelMode(KProcess::ForwardedChannels);
}

K3bKProcess::~K3bKProcess()
{
    delete d_ptr;
}

void K3bKProcess::setOutputChannelMode(KProcess::OutputChannelMode mode)
{
    Q_D(K3bKProcess);

    d->outputChannelMode = mode;
    disconnect(this, SIGNAL(readyReadStandardOutput()));
    disconnect(this, SIGNAL(readyReadStandardError()));
    switch (mode) {
    case KProcess::OnlyStdoutChannel:
        connect(this, SIGNAL(readyReadStandardError()), SLOT(_k_forwardStderr()));
        break;
    case KProcess::OnlyStderrChannel:
        connect(this, SIGNAL(readyReadStandardOutput()), SLOT(_k_forwardStdout()));
        break;
    default:
        K3bQProcess::setProcessChannelMode(static_cast<QProcess::ProcessChannelMode>(mode));
        return;
    }
    K3bQProcess::setProcessChannelMode(QProcess::SeparateChannels);
}

KProcess::OutputChannelMode K3bKProcess::outputChannelMode() const
{
    Q_D(const K3bKProcess);

    return d->outputChannelMode;
}

void K3bKProcess::setNextOpenMode(QIODevice::OpenMode mode)
{
    Q_D(K3bKProcess);

    d->openMode = mode;
}

#define DUMMYENV "_KPROCESS_DUMMY_="

void K3bKProcess::clearEnvironment()
{
    setEnvironment(QStringList() << QString::fromLatin1(DUMMYENV));
}

void K3bKProcess::setEnv(const QString &name, const QString &value, bool overwrite)
{
    QStringList env = environment();
    if (env.isEmpty()) {
        env = systemEnvironment();
        env.removeAll(QString::fromLatin1(DUMMYENV));
    }
    QString fname(name);
    fname.append('=');
    for (QStringList::Iterator it = env.begin(); it != env.end(); ++it)
        if ((*it).startsWith(fname)) {
            if (overwrite) {
                *it = fname.append(value);
                setEnvironment(env);
            }
            return;
        }
    env.append(fname.append(value));
    setEnvironment(env);
}

void K3bKProcess::unsetEnv(const QString &name)
{
    QStringList env = environment();
    if (env.isEmpty()) {
        env = systemEnvironment();
        env.removeAll(QString::fromLatin1(DUMMYENV));
    }
    QString fname(name);
    fname.append('=');
    for (QStringList::Iterator it = env.begin(); it != env.end(); ++it)
        if ((*it).startsWith(fname)) {
            env.erase(it);
            if (env.isEmpty())
                env.append(DUMMYENV);
            setEnvironment(env);
            return;
        }
}

void K3bKProcess::setProgram(const QString &exe, const QStringList &args)
{
    Q_D(K3bKProcess);

    d->prog = exe;
    d->args = args;
}

void K3bKProcess::setProgram(const QStringList &argv)
{
    Q_D(K3bKProcess);

    Q_ASSERT( !argv.isEmpty() );
    d->args = argv;
    d->prog = d->args.takeFirst();
}

K3bKProcess &K3bKProcess::operator<<(const QString &arg)
{
    Q_D(K3bKProcess);

    if (d->prog.isEmpty())
        d->prog = arg;
    else
        d->args << arg;
    return *this;
}

K3bKProcess &K3bKProcess::operator<<(const QStringList &args)
{
    Q_D(K3bKProcess);

    if (d->prog.isEmpty())
        setProgram(args);
    else
        d->args << args;
    return *this;
}

void K3bKProcess::clearProgram()
{
    Q_D(K3bKProcess);

    d->prog.clear();
    d->args.clear();
}

void K3bKProcess::setShellCommand(const QString &cmd)
{
    Q_D(K3bKProcess);

    KShell::Errors err;
    d->args = KShell::splitArgs(
            cmd, KShell::AbortOnMeta | KShell::TildeExpand, &err);
    if (err == KShell::NoError && !d->args.isEmpty()) {
        d->prog = QStandardPaths::findExecutable(d->args[0]);
        if (!d->prog.isEmpty()) {
            d->args.removeFirst();
            return;
        }
    }

    d->args.clear();

#ifdef Q_OS_UNIX
// #ifdef NON_FREE // ... as they ship non-POSIX /bin/sh
# if !defined(__linux__) && !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__) && !defined(__DragonFly__) && !defined(__GNU__)
    // If /bin/sh is a symlink, we can be pretty sure that it points to a
    // POSIX shell - the original bourne shell is about the only non-POSIX
    // shell still in use and it is always installed natively as /bin/sh.
    d->prog = QFile::symLinkTarget(QString::fromLatin1("/bin/sh"));
    if (d->prog.isEmpty()) {
        // Try some known POSIX shells.
        d->prog = QStandardPaths::findExecutable("ksh");
        if (d->prog.isEmpty()) {
            d->prog = QStandardPaths::findExecutable("ash");
            if (d->prog.isEmpty()) {
                d->prog = QStandardPaths::findExecutable("bash");
                if (d->prog.isEmpty()) {
                    d->prog = QStandardPaths::findExecutable("zsh");
                    if (d->prog.isEmpty())
                        // We're pretty much screwed, to be honest ...
                        d->prog = QString::fromLatin1("/bin/sh");
                }
            }
        }
    }
# else
    d->prog = QString::fromLatin1("/bin/sh");
# endif

    d->args << "-c" << cmd;
#else // Q_OS_UNIX
    // KMacroExpander::expandMacrosShellQuote(), KShell::quoteArg() and
    // KShell::joinArgs() may generate these for security reasons.
    setEnv(PERCENT_VARIABLE, "%");

    //see also TrollTechTaskTracker entry 88373.
    d->prog = QStandardPaths::findExecutable("kcmdwrapper");

    UINT size;
    WCHAR sysdir[MAX_PATH + 1];
    size = GetSystemDirectoryW(sysdir, MAX_PATH + 1);
    QString cmdexe = QString::fromUtf16((const ushort *) sysdir, size);
    cmdexe.append("\\cmd.exe");

    d->args << cmdexe << cmd;
#endif
}

QStringList K3bKProcess::program() const
{
    Q_D(const K3bKProcess);

    QStringList argv = d->args;
    argv.prepend(d->prog);
    return argv;
}

void K3bKProcess::start()
{
    Q_D(K3bKProcess);

    K3bQProcess::start(d->prog, d->args, d->openMode);
}

int K3bKProcess::execute(int msecs)
{
    start();
    if (!waitForFinished(msecs)) {
        kill();
        waitForFinished(-1);
        return -2;
    }
    return (exitStatus() == QProcess::NormalExit) ? exitCode() : -1;
}

// static
int K3bKProcess::execute(const QString &exe, const QStringList &args, int msecs)
{
    K3bKProcess p;
    p.setProgram(exe, args);
    return p.execute(msecs);
}

// static
int K3bKProcess::execute(const QStringList &argv, int msecs)
{
    K3bKProcess p;
    p.setProgram(argv);
    return p.execute(msecs);
}

int K3bKProcess::startDetached()
{
    Q_D(K3bKProcess);

    qint64 pid;
    if (!K3bQProcess::startDetached(d->prog, d->args, workingDirectory(), &pid))
        return 0;
    return int(pid);
}

// static
int K3bKProcess::startDetached(const QString &exe, const QStringList &args)
{
    qint64 pid;
    if (!K3bQProcess::startDetached(exe, args, QString(), &pid))
        return 0;
    return int(pid);
}

// static
int K3bKProcess::startDetached(const QStringList &argv)
{
    QStringList args = argv;
    QString prog = args.takeFirst();
    return startDetached(prog, args);
}

int K3bKProcess::pid() const
{
#ifdef Q_OS_UNIX
    return int(K3bQProcess::pid());
#else
    return K3bQProcess::pid() ? K3bQProcess::pid()->dwProcessId : 0;
#endif
}

#include "moc_k3bkprocess.cpp"
