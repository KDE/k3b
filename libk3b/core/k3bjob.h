/*
 *
 * Copyright (C) 2003-2010 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2011 Michal Malek <michalm@jabster.pl>
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


#ifndef K3BJOB_H
#define K3BJOB_H

#include "k3b_export.h"
#include "k3bdevicetypes.h"
#include "k3bglobals.h"
#include "k3bjobhandler.h"

#include <QObject>
#include <QString>

namespace K3b {
    namespace Device {
        class Device;
    }

    /**
     * This is the baseclass for all the jobs in K3b which actually do the work like burning a cd!
     * The Job object takes care of registering with the k3bcore or with a parent Job.
     *
     * Every job has a jobhandler which can be another job (in which case the job is handled as
     * a subjob) or an arbitrary class implementing the JobHandler interface.
     *
     * A Job should never create any widgets. User interaction should be done through the methods
     * questionYesNo, waitForMedium.
     *
     * @author Sebastian Trueg
     */
    class LIBK3B_EXPORT Job : public QObject, public JobHandler
    {
        Q_OBJECT

    public:
        ~Job() override;

        /**
         * \reimplemented from JobHandler
         */
        bool isJob() const override { return true; }

        JobHandler* jobHandler() const;

        /**
         * Is the job active?
         * The default implementation is based on the jobStarted() and jobFinished()
         * methods and there is normally no need to reimplement this.
         */
        virtual bool active() const;

        /**
         * The default implementation is based on the canceled() signal.
         *
         * This means that one cannot count on this value being valid
         * in a slot connected to the canceled() signal. It is, however, save
         * to call this method from a slot connected to the finished() signal
         * in case the job makes proper usage of the jobStarted/jobFinished
         * methods.
         */
        virtual bool hasBeenCanceled() const;

        virtual QString jobDescription() const { return "Job"; }
        virtual QString jobDetails() const { return QString(); }
        
        /**
         * @returns job source (e.g. path to image file)
         */
        virtual QString jobSource() const { return QString(); }
        
        /**
         * @return job target (e.g. name of the burner)
         */
        virtual QString jobTarget() const { return QString(); }

        /**
         * @returns the number of running subjobs.
         * this is useful for proper cancellation of jobs.
         */
        int numRunningSubJobs() const;

        QList<Job*> runningSubJobs() const;

        static const char DEFAULT_SIGNAL_CONNECTION[];

        /**
         * Connect a job 1-to-1. Useful for wrapper jobs.
         *
         * If a parameter is set to 0 it will not be connected at all
         */
        void connectJob( Job* subJob,
                         const char* finishedSlot = DEFAULT_SIGNAL_CONNECTION,
                         const char* newTaskSlot = DEFAULT_SIGNAL_CONNECTION,
                         const char* newSubTaskSlot = DEFAULT_SIGNAL_CONNECTION,
                         const char* progressSlot = DEFAULT_SIGNAL_CONNECTION,
                         const char* subProgressSlot = DEFAULT_SIGNAL_CONNECTION,
                         const char* processedSizeSlot = DEFAULT_SIGNAL_CONNECTION,
                         const char* processedSubSizeSlot = DEFAULT_SIGNAL_CONNECTION );

        /**
         * Connect a subjob.
         *
         * \param newTaskSlot    If DEFAULT_SIGNAL_CONNECTION the newTask signal from the subjob will
         *                       be connected to the newSubTask signal
         * \param newSubTaskSlot If DEFAULT_SIGNAL_CONNECTION the newSubTask signal from the subjob
         *                       will create an infoMessage signal
         * \param progressSlot   If DEFAULT_SIGNAL_CONNECTION the percent signal of the subjob will be
         *                       connected to the subPercent signal.
         * debuggingOutput and infoMessage will always be direcctly connected.
         *
         * If a parameter is set to 0 it will not be connected at all
         */
        void connectSubJob( Job* subJob,
                            const char* finishedSlot = DEFAULT_SIGNAL_CONNECTION,
                            const char* newTaskSlot = DEFAULT_SIGNAL_CONNECTION,
                            const char* newSubTaskSlot = DEFAULT_SIGNAL_CONNECTION,
                            const char* progressSlot = DEFAULT_SIGNAL_CONNECTION,
                            const char* subProgressSlot = DEFAULT_SIGNAL_CONNECTION,
                            const char* processedSizeSlot = DEFAULT_SIGNAL_CONNECTION,
                            const char* processedSubSizeSlot = DEFAULT_SIGNAL_CONNECTION );

        /**
         * Message types to be used in combination with the infoMessage signal.
         *
         * \see infoMessage()
         */
        enum MessageType {
            MessageInfo,     /**< Informational message. For example a message that informs the user about what is
                                currently going on */
            MessageWarning,  /**< A warning message. Something did not go perfectly but the job may continue. */
            MessageError,    /**< An error. Only use this message type if the job will actually fail afterwards
                                with a call to jobFinished( false ) */
            MessageSuccess   /**< This message type may be used to inform the user that a sub job has
                                been successfully finished. */
        };

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
         * Wait for the job to finish. In case the job is not running
         * this method returns immediately. Otherwise it will start a local
         * event loop to wait for the job to finish.
         */
        void wait();

    public Q_SLOTS:
        /**
         * This is the slot that starts the job. The first call should always
         * be jobStarted().
         *
         * Once the job has finished it has to call jobFinished() with the result as
         * a parameter.
         *
         * \see jobStarted()
         * \see jobFinished()
         */
        virtual void start() = 0;

        /**
         * This slot should cancel the job. The job has to emit the canceled() signal and make a call
         * to jobFinished().
         * It is not important to do any of those two directly in this slot though.
         */
        virtual void cancel() = 0;

        void setJobHandler( K3b::JobHandler* );

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
         * Emitted once the job has been finished. Never emit this signal directly.
         * Use jobFinished() instead, otherwise the job will not be properly deregistered
         */
        void finished( bool success );

    protected:
        /**
         * \param hdl the handler of the job. This allows for some user interaction without
         *            specifying any details (like the GUI).
         *            The job handler can also be another job. In that case this job is a sub job
         *            and will be part of the parents running sub jobs.
         *
         * \see runningSubJobs()
         * \see numRunningSubJobs()
         */
        Job( JobHandler* hdl, QObject* parent = 0 );

        /**
         * Call this in start() to properly register the job and emit the started() signal.
         * Do never emit the started() signal manually.
         *
         * Always call Job::jobStarted in reimplementations.
         */
        virtual void jobStarted();

        /**
         * Call this at the end of the job to properly deregister the job and emit the finished() signal.
         * Do never emit the finished() signal manually.
         *
         * Always call Job::jobFinished in reimplementations.
         */
        virtual void jobFinished( bool success );

    private Q_SLOTS:
        void slotCanceled();
        void slotNewSubTask( const QString& str );

    private:
        void registerSubJob( Job* );
        void unregisterSubJob( Job* );

        class Private;
        Private* const d;
    };


    /**
     * Every job used to actually burn a medium is derived from BurnJob.
     * This class implements additional signals like buffer status or writing speed
     * as well as a handling of the used writing application.
     */
    class LIBK3B_EXPORT BurnJob : public Job
    {
        Q_OBJECT

    public:
        explicit BurnJob( JobHandler* hdl, QObject* parent = 0 );
        ~BurnJob() override;

        /**
         * The writing device used by this job.
         */
        virtual Device::Device* writer() const { return 0; }

        /**
         * use WritingApp
         */
        WritingApp writingApp() const;

        /**
         * WritingApp "ored" together
         */
        virtual WritingApps supportedWritingApps() const;

    public Q_SLOTS:
        /**
         * use WritingApp
         */
        void setWritingApp( K3b::WritingApp w );

    Q_SIGNALS:
        void bufferStatus( int );

        void deviceBuffer( int );

        /**
         * @param speed current writing speed in Kb
         * @param multiplicator use 150 for CDs and 1380 for DVDs
         * FIXME: maybe one should be able to ask the burnjob if it burns a CD or a DVD and remove the
         *        multiplicator parameter)
         */
        void writeSpeed( int speed, K3b::Device::SpeedMultiplicator multiplicator );

        /**
         * This signal may be used to inform when the burning starts or ends
         * The BurningProgressDialog for example uses it to enable and disable
         * the buffer and writing speed displays.
         */
        void burning(bool);

    private:
        class Private;
        Private* const d;
     };
}

#endif
