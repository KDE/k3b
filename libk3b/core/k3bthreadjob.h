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


#ifndef _K3B_THREAD_JOB_H_
#define _K3B_THREAD_JOB_H_

#include "k3bjob.h"
#include "k3b_export.h"


/**
 * A K3bJob that runs in a different thread. Instead of reimplementing
 * start() reimplement run() to perform all operations in a different
 * thread. Otherwise usage is the same as K3bJob.
 */
class LIBK3B_EXPORT K3bThreadJob : public K3bJob
{
    Q_OBJECT

public:
    K3bThreadJob( K3bJobHandler*, QObject* parent = 0 );
    virtual ~K3bThreadJob();

    /**
     * \reimplemented from K3bJob
     *
     * \return true if the job has been started and has not yet
     * emitted the finished signal
     */
    virtual bool active() const;

    /**
     * reimplemented from K3bJobHandler
     */
    virtual int waitForMedia( K3bDevice::Device*,
                              int mediaState = K3bDevice::STATE_EMPTY,
                              int mediaType = K3bDevice::MEDIA_WRITABLE_CD,
                              const QString& message = QString() );
  
    /**
     * reimplemented from K3bJobHandler
     */
    virtual bool questionYesNo( const QString& text,
                                const QString& caption = QString(),
                                const QString& yesText = QString(),
                                const QString& noText = QString() );

    /**
     * reimplemented from K3bJobHandler
     */
    virtual void blockingInformation( const QString& text,
                                      const QString& caption = QString() );


    /**
     * waits until all running K3bThread have finished.
     * This is used by K3bApplication.
     */
    static void waitUntilFinished();

public Q_SLOTS:
    /**
     * Starts the job in a different thread. Emits the started()
     * signal.
     *
     * When reimplementing this method to perform housekeeping
     * operations in the GUI thread make sure to call the 
     * parent implementation.
     *
     * \sa run()
     */
    virtual void start();

    /**
     * Cancel the job. The method will give the thread a certain
     * time to actually cancel. After that the thread is terminated.
     *
     * \sa canceled()
     */
    virtual void cancel();

protected:
    /**
     * Implement this method to do the actual work in the thread.
     * Do not emit started(), finished(), and canceled() signals
     * in this method. K3bThreadJob will do that automatically.
     *
     * \return \p true on success.
     */
    virtual bool run() = 0;

    /**
     * Use to check if the job has been canceled.
     * \sa cancel()
     */
    bool canceled() const;

private Q_SLOTS:
    /**
     * Called in the GUi thread once the job is done.
     * Emits the finished signal and performs some
     * housekeeping.
     */
    void slotThreadFinished();

private:
    void customEvent( QEvent* );

    class Private;
    Private* const d;

    friend class K3bThread;
};

#endif

