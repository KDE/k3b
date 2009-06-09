/*
 *
 * Copyright (C) 2007-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_THREAD_JOB_COMMUNICATION_EVENT_H_
#define _K3B_THREAD_JOB_COMMUNICATION_EVENT_H_

#include <qevent.h>
#include <qstring.h>
#include <qwaitcondition.h>
#include <QEvent>

#include "k3bdevicetypes.h"

namespace K3b {
    namespace Device {
        class Device;
    }
}

namespace K3b {
    class ThreadJobCommunicationEvent : public QEvent
    {
    public:
        ~ThreadJobCommunicationEvent();

        enum Type {
            WaitForMedium = QEvent::User + 50,
            QuestionYesNo,
            BlockingInfo
        };

        int type() const;

        /**
         * Separate data object are used since events are deleted once delivered.
         * However, we need the data after the event has been delivered.
         */
        class Data {
        public:
            Data();

            Device::Device* device() const;
            Device::MediaStates wantedMediaState() const;
            Device::MediaTypes wantedMediaType() const;
            QString message() const;

            QString text() const;
            QString caption() const;

            QString yesText() const;
            QString noText() const;

            int intResult() const;
            bool boolResult() const;


            /**
             * Used by the calling thread to wait for the result
             */
            void wait();

            /**
             * Signal back to the calling thread.
             */
            void done( int result );

        private:
            Device::Device* m_device;
            Device::MediaStates m_wantedMediaState;
            Device::MediaTypes m_wantedMediaType;
            QString m_text;
            QString m_caption;
            QString m_yesText;
            QString m_noText;

            QWaitCondition m_threader;
            int m_result;

            friend class ThreadJobCommunicationEvent;
        };

        Data* data() const { return m_data; }

        static ThreadJobCommunicationEvent* waitForMedium( Device::Device* device,
                                                           Device::MediaStates mediaState,
                                                           Device::MediaTypes mediaType,
                                                           const QString& message );
        static ThreadJobCommunicationEvent* questionYesNo( const QString& text,
                                                           const QString& caption,
                                                           const QString& yesText,
                                                           const QString& noText );
        static ThreadJobCommunicationEvent* blockingInformation( const QString& text,
                                                                 const QString& caption );

    private:
        ThreadJobCommunicationEvent( int type );

        int m_type;
        Data* m_data;
    };
}

#endif
