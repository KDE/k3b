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


#ifndef K3BEMPTYDISCWAITER_H
#define K3BEMPTYDISCWAITER_H

#include "k3bjobhandler.h"
#include "k3bdiskinfo.h"

#include <QCloseEvent>
#include <QDialog>


/**
 * Tests for an empty cd in a given device.
 *
 * Use the static wait methods.
 *
 * @author Sebastian Trueg
 */
namespace K3b {

    namespace Device {
        class Device;
    }

    class EmptyDiscWaiter : public QDialog, public JobHandler
    {
        Q_OBJECT

    public:
        ~EmptyDiscWaiter() override;

        /**
         * the same as waitForEmptyDisc( false );
         */
        int exec() override;

        /**
         * @reimplemented from JobHandler
         * \internal do not use!
         */
        Device::MediaType waitForMedium( Device::Device*,
                                         Device::MediaStates mediaState = Device::STATE_EMPTY,
                                         Device::MediaTypes mediaType = Device::MEDIA_WRITABLE_CD,
                                         const K3b::Msf& minMediaSize = K3b::Msf(),
                                         const QString& message = QString() ) override;

        /**
         * @reimplemented from JobHandler
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
         * Starts the emptydiskwaiter.
         *
         * \param mediaState a bitwise combination of the Device::State enum
         * \param mediaType a bitwise combination of the Device::MediaType enum
         * \return the found MediaType on success, MEDIA_UNKNOWN if canceled
         */
        static Device::MediaType wait( Device::Device* device,
                                       Device::MediaStates mediaState,
                                       Device::MediaTypes mediaType = Device::MEDIA_WRITABLE_CD,
                                       const K3b::Msf& minMediaSize = K3b::Msf(),
                                       const QString& message = QString(),
                                       QWidget* parent = 0 );

    protected Q_SLOTS:
        void slotCancel();
        void slotEject();
        void slotLoad();
        void slotMediumChanged( K3b::Device::Device* );
        void showDialog();
        void continueWaiting();
        void slotErasingFinished( bool );

    protected:
        /**
         * Use the static wait methods.
         */
        explicit EmptyDiscWaiter( Device::Device* device, QWidget* parent = 0 );

        Device::MediaType waitForDisc( Device::MediaStates mediaState = Device::STATE_EMPTY,
                                       Device::MediaTypes mediaType = Device::MEDIA_WRITABLE_CD,
                                       const K3b::Msf& minMediaSize = K3b::Msf(),
                                       const QString& message = QString() );


        /**
         * Nobody closes this dialog but itself!
         */
        void closeEvent( QCloseEvent *e ) override { e->ignore(); }

    Q_SIGNALS:
        void leaveModality();

    private:
        void enterLoop();
        void finishWaiting( Device::MediaType );
        void prepareErasingDialog();

        QWidget* parentWidgetToUse();

        class Private;
        Private* d;
    };
}


#endif
