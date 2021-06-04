/*
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_SIMPLE_JOB_HANDLER_H_
#define _K3B_SIMPLE_JOB_HANDLER_H_

#include "k3bjobhandler.h"
#include "k3b_export.h"

#include <QObject>


namespace K3b {
    /**
     * This is a simplified job handler which just consumes the
     * job handler calls without doing anything.
     * Use it for very simple jobs that don't need the job handler
     * methods.
     */
    class LIBK3B_EXPORT SimpleJobHandler : public QObject, public JobHandler
    {
        Q_OBJECT

    public:
        explicit SimpleJobHandler( QObject* parent = 0 );
        ~SimpleJobHandler() override;

        /*
         * \return MEDIA_UNKNOWN
         */
        Device::MediaType waitForMedium( Device::Device*,
                                         K3b::Device::MediaStates mediaState = Device::STATE_EMPTY,
                                         K3b::Device::MediaTypes mediaType = Device::MEDIA_WRITABLE_CD,
                                         const K3b::Msf& minMediaSize = K3b::Msf(),
                                         const QString& message = QString() ) override;
        /**
         * \return true
         */
        bool questionYesNo( const QString& text,
                            const QString& caption = QString(),
                            const KGuiItem& buttonYes = KStandardGuiItem::yes(),
                            const KGuiItem& buttonNo = KStandardGuiItem::no() ) override;

        /**
         * Does nothing
         */
        void blockingInformation( const QString& text,
                                  const QString& caption = QString() ) override;
    };
}

#endif
