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

#ifndef _K3B_THROUGHPUT_ESTIMATOR_H_
#define _K3B_THROUGHPUT_ESTIMATOR_H_

#include <QObject>


namespace K3b {
    /**
     * Little helper class that allows an estimation of the current writing
     * speed. Just init with @p reset() then always call @p dataWritten with
     * the already written data in KB. The class will emit throughput signals
     * whenever the throughput changes.
     */
    class ThroughputEstimator : public QObject
    {
        Q_OBJECT

    public:
        explicit ThroughputEstimator( QObject* parent = 0 );
        ~ThroughputEstimator() override;

        int average() const;

    Q_SIGNALS:
        /**
         * kb/s if differs from previous
         */
        void throughput( int );

    public Q_SLOTS:
        void reset();

        /**
         * @param data written kb
         */
        void dataWritten( unsigned long data );

    private:
        class Private;
        Private* d;
    };
}

#endif
