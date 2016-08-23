/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_VIDEODVD_DOC_H_
#define _K3B_VIDEODVD_DOC_H_

#include "k3bdatadoc.h"
#include "k3b_export.h"

namespace K3b {
    class LIBK3B_EXPORT VideoDvdDoc : public DataDoc
    {
    public:
        VideoDvdDoc( QObject* parent = 0 );
        virtual ~VideoDvdDoc();

        virtual Type type() const { return VideoDvdProject; }
        virtual QString typeString() const { return QString::fromLatin1("video_dvd"); }

        Device::MediaTypes supportedMediaTypes() const;

        virtual BurnJob* newBurnJob( JobHandler* hdl, QObject* parent );

        virtual bool newDocument();

        DirItem* videoTsDir() const { return m_videoTsDir; }

        // TODO: implement load- and saveDocumentData since we do not need all those options
        bool saveDocumentData(QDomElement*);

    private:
        DirItem* m_videoTsDir;
    };
}

#endif
