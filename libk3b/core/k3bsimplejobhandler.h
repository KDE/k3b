/*
 *
 * Copyright (C) 2006-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_SIMPLE_JOB_HANDLER_H_
#define _K3B_SIMPLE_JOB_HANDLER_H_

#include <k3b_export.h>

#include <qobject.h>
#include <k3bjobhandler.h>


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
        SimpleJobHandler( QObject* parent = 0 );
        ~SimpleJobHandler();

        /*
         * \return 0
         */
        int waitForMedia( Device::Device*,
                          int mediaState = Device::STATE_EMPTY,
                          int mediaType = Device::MEDIA_WRITABLE_CD,
                          const QString& message = QString() );
        /**
         * \return true
         */
        bool questionYesNo( const QString& text,
                            const QString& caption = QString(),
                            const QString& yesText = QString(),
                            const QString& noText = QString() );

        /**
         * Does nothing
         */
        void blockingInformation( const QString& text,
                                  const QString& caption = QString() );
    };
}

#endif
