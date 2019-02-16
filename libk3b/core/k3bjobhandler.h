/*
 *
 * Copyright (C) 2004-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_JOB_HANDLER_H_
#define _K3B_JOB_HANDLER_H_


#include "k3bdiskinfo.h"
#include "k3bdevice.h"
#include <KStandardGuiItem>


namespace K3b {
    /**
     * See @p JobProgressDialog as an example for the usage of
     * the JobHandler interface.
     */
    class JobHandler
    {
    public:
        JobHandler() {}
        virtual ~JobHandler() {}

        /**
         * \return true if the handler itself is also a job
         */
        virtual bool isJob() const { return false; }

        /**
         * @return Device::MediaType on success,
         *         Device::MEDIA_UNKNOWN on error (canceled)
         */
        virtual Device::MediaType waitForMedium( Device::Device*,
                                                 Device::MediaStates mediaState = Device::STATE_EMPTY,
                                                 Device::MediaTypes mediaType = Device::MEDIA_WRITABLE_CD,
                                                 const K3b::Msf& minMediaSize = K3b::Msf(),
                                                 const QString& message = QString() ) = 0;

        virtual bool questionYesNo( const QString& text,
                                    const QString& caption = QString(),
                                    const KGuiItem& buttonYes = KStandardGuiItem::yes(),
                                    const KGuiItem& buttonNo = KStandardGuiItem::no() ) = 0;

        /**
         * Use this if you need the user to do something before the job is able to continue.
         * In all other cases an infoMessage should be used.
         */
        virtual void blockingInformation( const QString& text,
                                          const QString& caption = QString() ) = 0;

    };
}

#endif
