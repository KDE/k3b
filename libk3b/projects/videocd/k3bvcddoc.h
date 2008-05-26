/*
*
* Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
* Copyright (C) 2007 Sebastian Trueg <trueg@k3b.org>
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

// Qt Includes
#include <qfile.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qdatetime.h>
#include <qimage.h>
#include <qqueue.h>

// Kde Includes
#include <kurl.h>

// K3b Includes
#include "k3bvcdoptions.h"
#include "mpeginfo/k3bmpeginfo.h"
#include <k3bdoc.h>
#include "k3b_export.h"
class K3bVcdTrack;
//class K3bView;
class QTimer;
class QDomElement;



class LIBK3B_EXPORT K3bVcdDoc : public K3bDoc
{
        Q_OBJECT

    public:
        K3bVcdDoc( QObject* );
        ~K3bVcdDoc();

	int type() const { return VCD; }

	int supportedMediaTypes() const;

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

/*         K3bVcdTrack* first() */
/*         { */
/*             return m_tracks->first(); */
/*         } */
/*         K3bVcdTrack* current() const */
/*         { */
/*             return m_tracks->current(); */
/*         } */
/*         K3bVcdTrack* next() */
/*         { */
/*             return m_tracks->next(); */
/*         } */
/*         K3bVcdTrack* prev() */
/*         { */
/*             return m_tracks->prev(); */
/*         } */
        K3bVcdTrack* at( uint i )
        {
            return m_tracks->at( i );
        }
/*         K3bVcdTrack* take( uint i ) */
/*         { */
/*             return m_tracks->take( i ); */
/*         } */

	const QList<K3bVcdTrack*>* tracks() const
        {
            return m_tracks;
        }

        /** get the current size of the project */
        KIO::filesize_t size() const;
        K3b::Msf length() const;

        K3bBurnJob* newBurnJob( K3bJobHandler* hdl, QObject* parent );
        K3bVcdOptions* vcdOptions() const
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
        void addTrack( K3bVcdTrack* track, uint position = 0 );

        // --- TODO: this should read: removeTrack( K3bVcdTrack* )
        void removeTrack( K3bVcdTrack* );
        void moveTrack( K3bVcdTrack* track, K3bVcdTrack* after );

    protected Q_SLOTS:
        /** processes queue "urlsToAdd" **/
        void slotWorkUrlQueue();

     Q_SIGNALS:
        void newTracks();

        void trackRemoved( K3bVcdTrack* );

    protected:
        /** reimplemented from K3bDoc */
        bool loadDocumentData( QDomElement* root );
        /** reimplemented from K3bDoc */
        bool saveDocumentData( QDomElement* );

        QString typeString() const;

    private:
        K3bVcdTrack* createTrack( const KUrl& url );
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

        QList<K3bVcdTrack*>* m_tracks;
        KIO::filesize_t calcTotalSize() const;
        KIO::filesize_t ISOsize() const;

        bool isImage( const KUrl& url );

        K3bVcdTrack* m_lastAddedTrack;
        K3bVcdOptions* m_vcdOptions;

        int m_vcdType;
        int lastAddedPosition;
};

#endif
