/*

    SPDX-FileCopyrightText: 2003 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef _K3B_VIDEODVD_DOC_H_
#define _K3B_VIDEODVD_DOC_H_

#include "k3bdatadoc.h"
#include "k3b_export.h"

namespace K3b {
    class LIBK3B_EXPORT VideoDvdDoc : public DataDoc
    {
    public:
        explicit VideoDvdDoc( QObject* parent = 0 );
        ~VideoDvdDoc() override;

        Type type() const override { return VideoDvdProject; }
        QString typeString() const override { return QString::fromLatin1("video_dvd"); }

        Device::MediaTypes supportedMediaTypes() const override;

        BurnJob* newBurnJob( JobHandler* hdl, QObject* parent ) override;

        bool newDocument() override;
        void clear() override;

        DirItem* videoTsDir() const { return m_videoTsDir; }

        // TODO: implement load- and saveDocumentData since we do not need all those options
        bool saveDocumentData(QDomElement*) override;

    private:
        void addAudioVideoTsDirs();

        DirItem* m_videoTsDir = nullptr;
        DirItem* m_audioTsDir = nullptr;
    };
}

#endif
