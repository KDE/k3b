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

#ifndef _K3B_AUDIOSESSION_READING_JOB_H_
#define _K3B_AUDIOSESSION_READING_JOB_H_

#include "k3bthreadjob.h"

#include <QStringList>

class QIODevice;

namespace K3b {
    namespace Device {
        class Device;
        class Toc;
    }

    class AudioSessionReadingJob : public ThreadJob
    {
        Q_OBJECT

    public:
        explicit AudioSessionReadingJob( JobHandler*, QObject* parent = 0 );
        ~AudioSessionReadingJob();

        /**
         * For now this simply reads all the audio tracks at the beginning
         * since we only support CD-Extra mixed mode cds.
         */
        void setDevice( Device::Device* );

        /**
         * Use for faster initialization
         */
        void setToc( const Device::Toc& toc );

        /**
         * the data gets written directly into ioDev instead of imagefiles.
         * To disable just set ioDev to 0 (the default)
         */
        void writeTo( QIODevice* ioDev );

        /**
         * Used if fd == -1
         */
        void setImageNames( const QStringList& l );

        void setParanoiaMode( int m );
        void setReadRetries( int );
        void setNeverSkip( bool b );

    public Q_SLOTS:
        void start();

    private:
        void jobFinished( bool );
        bool run();

        class Private;
        Private* const d;
    };
}

#endif
