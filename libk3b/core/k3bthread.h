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


#ifndef _K3B_THREAD_H_
#define _K3B_THREAD_H_

#include <qthread.h>
#include "k3b_export.h"
#include <k3bdevicetypes.h>

#include <kdemacros.h>


class QObject;
namespace K3bDevice {
    class Device;
}

/**
 * The threaded couterpart to K3bJob
 *
 * K3bThreadJob can be used to automatically wrap the thread in a K3bJob.
 *
 * As in K3bJob it is important to call emitStarted and emitFinished.
 *
 * See K3bThreadJob for more information.
 */
class LIBK3B_EXPORT K3bThread : public QThread
{
    Q_OBJECT

public:
    K3bThread( QObject* eventHandler = 0, QObject* parent = 0 );
    virtual ~K3bThread();

    void setProgressInfoEventHandler( QObject* eventHandler );

    /**
     * Initialize the thread before starting it in the GUi thread.
     * K3bThreadJob automatically calls this.
     *
     * The default implementation does nothing.
     */
    virtual void init();

    /**
     * to provide the same api like K3bJob
     * the default implementation calls terminate and
     * emitCancled() and emitFinished(false)
     */
    virtual void cancel();

    virtual QString jobDescription() const;
    virtual QString jobDetails() const;

    /**
     * waits until all running K3bThread have finished.
     * This is used by K3bApplication.
     */
    static void waitUntilFinished();

Q_SIGNALS:
    void infoMessage( const QString& msg, int type );
    void percent( int p );
    void subPercent( int p );
    void processedSize( int processed, int size );
    void processedSubSize( int processed, int size );
    void newTask( const QString& job );
    void newSubTask( const QString& job );
    void debuggingOutput(const QString&, const QString&);
    void nextTrack( int track, int numTracks );

    void canceled();

    /**
     * Emitted once the job has been started. Never emit this signal directly.
     * Use jobStarted() instead, otherwise the job will not be properly registered
     */
    void started();

    /**
     * Emitted once the job has been finshed. Never emit this signal directly.
     * Use jobFinished() instead, otherwise the job will not be properly deregistered
     */
    void finished( bool success );

protected:
    virtual void run() = 0;

    /**
     * uses the K3bJob::MessageType enum
     */
    KDE_DEPRECATED void emitInfoMessage( const QString& msg, int type );
    KDE_DEPRECATED void emitPercent( int p );
    KDE_DEPRECATED void emitSubPercent( int p );
    KDE_DEPRECATED void emitStarted();
    KDE_DEPRECATED void emitCanceled();
    KDE_DEPRECATED void emitFinished( bool success );
    KDE_DEPRECATED void emitProcessedSize( int processed, int size );
    KDE_DEPRECATED void emitProcessedSubSize( int processed, int size );
    KDE_DEPRECATED void emitNewTask( const QString& job );
    KDE_DEPRECATED void emitNewSubTask( const QString& job );
    KDE_DEPRECATED void emitDebuggingOutput(const QString&, const QString&);
    KDE_DEPRECATED void emitNextTrack( int track, int trackNum );

    /**
     * \sa K3bJobHandler::waitForMedia
     */
    int waitForMedia( K3bDevice::Device*,
                      int mediaState = K3bDevice::STATE_EMPTY,
                      int mediaType = K3bDevice::MEDIA_WRITABLE_CD,
                      const QString& message = QString::null );

    /**
     * \sa K3bJobHandler::questionYesNo
     */
    bool questionYesNo( const QString& text,
                        const QString& caption = QString::null,
                        const QString& yesText = QString::null,
                        const QString& noText = QString::null );

    /**
     * \sa K3bJobHandler::blockingInformation
     */
    void blockingInformation( const QString& text,
                              const QString& caption = QString::null );

private:
    class Private;
    Private* d;
};

#endif
