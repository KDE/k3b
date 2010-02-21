/*
 *
 * Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
 * Copyright (C) 2007 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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

#ifndef K3BVCDDOC_H
#define K3BVCDDOC_H

#include "k3bdoc.h"
#include "k3bvcdoptions.h"
#include "mpeginfo/k3bmpeginfo.h"

#include <QStringList>
#include <QQueue>

#include "k3b_export.h"

class QTimer;
class QDomElement;

namespace K3b {
    class VcdTrack;

    class LIBK3B_EXPORT VcdDoc : public Doc
    {
        Q_OBJECT

    public:
        VcdDoc( QObject* );
        ~VcdDoc();

        virtual Type type() const { return VcdProject; }
        virtual QString typeString() const { return QString::fromLatin1("vcd"); }

        Device::MediaTypes supportedMediaTypes() const;

        QString name() const;

        enum vcdTypes { VCD11, VCD20, SVCD10, HQVCD, NONE};

        bool newDocument();
        void clear();

        int numOfTracks() const
        {
            return m_tracks->count();
        }

        const QString& vcdImage() const
        {
            return m_vcdImage;
        }
        void setVcdImage( const QString& s )
        {
            m_vcdImage = s;
        }

        VcdTrack* at( uint i )
        {
            return m_tracks->at( i );
        }

        const QList<VcdTrack*>* tracks() const
        {
            return m_tracks;
        }

        /** get the current size of the project */
        KIO::filesize_t size() const;
        Msf length() const;

        BurnJob* newBurnJob( JobHandler* hdl, QObject* parent );
        VcdOptions* vcdOptions() const
        {
            return m_vcdOptions;
        }

        int vcdType() const
        {
            return m_vcdType;
        }
        void setVcdType( int type );
        void setPbcTracks();

    public Q_SLOTS:
        /**
         * will test the file and add it to the project.
         * connect to at least result() to know when
         * the process is finished and check error()
         * to know about the result.
         **/
        void addUrls( const KUrl::List& );
        void addTrack( const KUrl&, uint );
        void addTracks( const KUrl::List&, uint );
        /** adds a track without any testing */
        void addTrack( K3b::VcdTrack* track, uint position = 0 );

        // --- TODO: this should read: removeTrack( VcdTrack* )
        void removeTrack( K3b::VcdTrack* track );
        
        /**
         * @arg track - track about to be moved
         * @arg before - place where track is to be moved.
         *               If before=0 the track will be moved to the end
         */
        void moveTrack( K3b::VcdTrack* track, K3b::VcdTrack* before );

    protected Q_SLOTS:
        /** processes queue "urlsToAdd" **/
        void slotWorkUrlQueue();

    Q_SIGNALS:
        void aboutToAddVCDTracks( int pos, int count );
        void addedVCDTracks();
        void aboutToRemoveVCDTracks( int pos, int count );
        void removedVCDTracks();

        void newTracks();

        void trackRemoved( K3b::VcdTrack* );

    protected:
        /** reimplemented from Doc */
        bool loadDocumentData( QDomElement* root );
        /** reimplemented from Doc */
        bool saveDocumentData( QDomElement* );

    private:
        VcdTrack* createTrack( const KUrl& url );
        void informAboutNotFoundFiles();

        QStringList m_notFoundFiles;
        QString m_vcdImage;

        class PrivateUrlToAdd
        {
        public:
            PrivateUrlToAdd( const KUrl& u, int _pos )
                : url( u ), position( _pos )
            {}
            KUrl url;
            int position;
        };

        /** Holds all the urls that have to be added to the list of tracks. **/
        QQueue<PrivateUrlToAdd*> urlsToAdd;
        QTimer* m_urlAddingTimer;

        QList<VcdTrack*>* m_tracks;
        KIO::filesize_t calcTotalSize() const;
        KIO::filesize_t ISOsize() const;

        bool isImage( const KUrl& url );

        VcdTrack* m_lastAddedTrack;
        VcdOptions* m_vcdOptions;

        int m_vcdType;
        int lastAddedPosition;
    };
}

#endif
