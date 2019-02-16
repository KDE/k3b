/*
 *
 * Copyright (C) 2003-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
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
#include <climits>


namespace K3b {

    class Thread;

    /**
     * A Job that runs in a different thread. Instead of reimplementing
     * start() reimplement run() to perform all operations in a different
     * thread. Otherwise usage is the same as Job.
     */
    class LIBK3B_EXPORT ThreadJob : public Job
    {
        Q_OBJECT

    public:
        explicit ThreadJob( JobHandler*, QObject* parent = 0 );
        ~ThreadJob() override;

        /**
         * \reimplemented from Job
         *
         * \return true if the job has been started and has not yet
         * emitted the finished signal
         */
        bool active() const override;

        /**
         * reimplemented from JobHandler
         */
        Device::MediaType waitForMedium( Device::Device*,
                                                 Device::MediaStates mediaState = Device::STATE_EMPTY,
                                                 Device::MediaTypes mediaType = Device::MEDIA_WRITABLE_CD,
                                                 const K3b::Msf& minMediaSize = K3b::Msf(),
                                                 const QString& message = QString() ) override;

        /**
         * reimplemented from JobHandler
         */
        bool questionYesNo( const QString& text,
                                    const QString& caption = QString(),
                                    const KGuiItem& buttonYes = KStandardGuiItem::yes(),
                                    const KGuiItem& buttonNo = KStandardGuiItem::no() ) override;

        /**
         * reimplemented from JobHandler
         */
        void blockingInformation( const QString& text,
                                          const QString& caption = QString() ) override;


        /**
         * Call QThread::wait() on job's thread
         * \see QThread::wait()
         */
        bool wait( unsigned long time = ULONG_MAX );

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
        void start() override;

        /**
         * Cancel the job. The method will give the thread a certain
         * time to actually cancel. After that the thread is terminated.
         *
         * \sa canceled()
         */
        void cancel() override;

    protected:
        /**
         * Implement this method to do the actual work in the thread.
         * Do not emit started(), finished(), and canceled() signals
         * in this method. ThreadJob will do that automatically.
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
        void customEvent( QEvent* ) override;

        class Private;
        Private* const d;

        friend class Thread;
    };
}

#endif

