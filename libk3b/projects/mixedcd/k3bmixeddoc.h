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

#ifndef K3B_MIXED_DOC_H
#define K3B_MIXED_DOC_H

#include "k3bdoc.h"
#include "k3bdatadoc.h"
#include "k3baudiodoc.h"
#include "k3btoc.h"
#include "k3b_export.h"

class QDomElement;
namespace K3b {
    class BurnJob;

    class LIBK3B_EXPORT MixedDoc : public Doc
    {
        Q_OBJECT

    public:
        explicit MixedDoc( QObject* parent = 0 );
        ~MixedDoc() override;
        
        Type type() const override { return MixedProject; }
        QString typeString() const override { return QString::fromLatin1("mixed"); }

        QString name() const override;

        Device::MediaTypes supportedMediaTypes() const override;

        bool newDocument() override;
        void clear() override;

        void setModified( bool m = true ) override;
        bool isModified() const override;

        KIO::filesize_t size() const override;
        Msf length() const override;

        int numOfTracks() const override;

        BurnJob* newBurnJob( JobHandler*, QObject* parent = 0 ) override;

        AudioDoc* audioDoc() const { return m_audioDoc; }
        DataDoc* dataDoc() const { return m_dataDoc; }

        enum MixedType { DATA_FIRST_TRACK,
                         DATA_LAST_TRACK,
                         DATA_SECOND_SESSION };

        int mixedType() const { return m_mixedType; }

        void setURL( const QUrl& url ) override;

        /**
         * Represent the structure of the doc as CD Table of Contents.
         * Be aware that the length of the data track is just an estimate
         * and needs to be corrected if not specified here.
         *
         * @param dataMode mode of the data track (MODE1 or XA_FORM1)
         * @param dataTrackLength exact length of the dataTrack
         */
        Device::Toc toToc( K3b::Device::Track::DataMode dataMode, const Msf& dataTrackLength = 0 ) const;

    public Q_SLOTS:
        void setMixedType( MixedType t ) { m_mixedType = t; }
        void addUrls( const QList<QUrl>& urls ) override;

    protected:
        bool loadDocumentData( QDomElement* ) override;
        bool saveDocumentData( QDomElement* ) override;

    private:
        DataDoc* m_dataDoc;
        AudioDoc* m_audioDoc;

        int m_mixedType;
    };
}


#endif
