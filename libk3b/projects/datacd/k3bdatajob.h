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


#ifndef K3BDATAJOB_H
#define K3BDATAJOB_H

#include "k3bjob.h"
#include "k3bdatadoc.h"

#include <QFile>

class QString;

namespace K3b {

    class AbstractWriter;
    class IsoImager;

    namespace Device {
        class Device;
    }

    class DataJob : public BurnJob
    {
        Q_OBJECT

    public:
        DataJob( DataDoc*, JobHandler*, QObject* parent = 0 );
        ~DataJob() override;

        Doc* doc() const;
        Device::Device* writer() const override;

        bool hasBeenCanceled() const override;

        QString jobDescription() const override;
        QString jobDetails() const override;

    public Q_SLOTS:
        void cancel() override;
        void start() override;

        /**
         * Used to specify a non-default writer.
         * If this does notget called DataJob determines
         * the writer itself.
         */
        void setWriterJob( AbstractWriter* );
        void setImager( IsoImager* );

    protected Q_SLOTS:
        void slotIsoImagerFinished( bool success );
        void slotIsoImagerPercent(int);
        void slotWriterJobPercent( int p );
        void slotWriterNextTrack( int t, int tt );
        void slotWriterJobFinished( bool success );
        void slotVerificationProgress( int );
        void slotVerificationFinished( bool );
        void writeImage();

        /**
         * \return true if some job was still running and was
         * cancelled. Thus, this can be called multiple times
         * and also to check if the job can be finished.
         */
        bool cancelAll();

    private Q_SLOTS:
        void slotMultiSessionParamterSetupDone( bool );

    protected:
        virtual bool prepareWriterJob();
        virtual void prepareImager();
        virtual void cleanup();

        DataDoc::MultiSessionMode usedMultiSessionMode() const;

        AbstractWriter* m_writerJob;
        IsoImager* m_isoImager;

    private:
        bool waitForBurnMedium();
        bool startWriterJob();
        bool startOnTheFlyWriting();
        void prepareWriting();
        void connectImager();
        bool setupCdrecordJob();
        bool setupCdrdaoJob();
        bool setupGrowisofsJob();
        void startPipe();
        void finishCopy();

        class Private;
        Private* d;
    };
}

#endif
