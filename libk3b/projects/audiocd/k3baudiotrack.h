/* 
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BAUDIOTRACK_H
#define K3BAUDIOTRACK_H

#include <QtCore/QObject>
#include <qstring.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <q3ptrlist.h>

#include <kio/global.h>

#include <k3bmsf.h>

#include <k3bcdtext.h>
#include <k3btrack.h>
#include "k3b_export.h"

class K3bAudioDataSource;
class K3bAudioDoc;


/**
 * @author Sebastian Trueg
 */
class LIBK3B_EXPORT K3bAudioTrack : public QObject
{
    Q_OBJECT

    friend class K3bAudioDataSource;
    friend class K3bAudioDoc;

public:
    K3bAudioTrack();
    K3bAudioTrack( K3bAudioDoc* parent );
    ~K3bAudioTrack();

    K3bAudioDoc* doc() const { return m_parent; }

    K3bDevice::Track toCdTrack() const;

    /** 
     * @return length of track in frames
     */
    K3b::Msf length() const;
    KIO::filesize_t size() const;

    QString artist() const { return m_cdText.performer(); }
    QString performer() const { return m_cdText.performer(); }
    QString title() const { return m_cdText.title(); }
    QString arranger() const { return m_cdText.arranger(); }
    QString songwriter() const { return m_cdText.songwriter(); }
    QString composer() const { return m_cdText.composer(); }
    QString isrc() const { return m_cdText.isrc(); }
    QString cdTextMessage() const { return m_cdText.message(); }
    K3bDevice::TrackCdText cdText() const { return m_cdText; }
	
    bool copyProtection() const { return m_copy; }
    bool preEmp() const { return m_preEmp; }
	
    /**
     * @obsolete use setPerformer
     **/
    void setArtist( const QString& a );
    void setPerformer( const QString& a );
    void setTitle( const QString& t );
    void setArranger( const QString& t );
    void setSongwriter( const QString& t );
    void setComposer( const QString& t );
    void setIsrc( const QString& t );
    void setCdTextMessage( const QString& t );

    void setCdText( const K3bDevice::TrackCdText& cdtext );

    void setPreEmp( bool b ) { m_preEmp = b; emitChanged(); }
    void setCopyProtection( bool b ) { m_copy = b; emitChanged(); }

    K3b::Msf index0() const;
    /**
     * The length of the postgap, ie. the number of blocks with index0.
     * This is always 0 for the last track.
     */
    K3b::Msf postGap() const;
    void setIndex0( const K3b::Msf& );

    /**
     * \return The track number starting at 1.
     */
    unsigned int trackNumber() const;

    /**
     * Remove this track from the list and return it.
     */
    K3bAudioTrack* take();

    /**
     * Move this track after @p track.
     * If @p track is null this track will be merged into the beginning
     * of the docs list.
     */
    void moveAfter( K3bAudioTrack* track );

    /**
     * Move this track ahead of @p track.
     * If @p track is null this track will be appended to the end
     * of the docs list.
     */
    void moveAhead( K3bAudioTrack* track );

    /**
     * Merge @p trackToMerge into this one.
     */
    void merge( K3bAudioTrack* trackToMerge, K3bAudioDataSource* sourceAfter = 0 );

    K3bAudioTrack* prev() const { return m_prev; }
    K3bAudioTrack* next() const { return m_next; }

    /**
     * Use with care.
     */
    void setFirstSource( K3bAudioDataSource* source );
    K3bAudioDataSource* firstSource() const { return m_firstSource; }
    K3bAudioDataSource* lastSource() const;
    int numberSources() const;

    /**
     * Append source to the end of the sources list.
     */
    void addSource( K3bAudioDataSource* source );

    bool seek( const K3b::Msf& );

    /**
     * Read data from the track.
     *
     * @return number of read bytes
     */
    int read( char* data, unsigned int max );

    /**
     * called by K3bAudioDataSource because of the lack of signals
     */
    void sourceChanged( K3bAudioDataSource* );

    /**
     * Create a copy of this track containing copies of all the sources
     * but not being part of some list.
     */
    K3bAudioTrack* copy() const;

    /**
     * Split the track at position pos and return the splitted track
     * on success.
     * The new track will be moved after this track.
     *
     * \param pos The position at which to split. \a pos will be the 
     * first frame in the new track.
     */
    K3bAudioTrack* split( const K3b::Msf& pos );

    /**
     * Is this track in a list
     */
    bool inList() const;

    /**
     * Get the source at index.
     * \return the requested source or 0 if index is out
     * of bounds.
     */
    K3bAudioDataSource* getSource( int index ) const;

private:	
    /**
     * Tells the doc that the track has changed
     */
    void emitChanged();

    void debug();

    K3bAudioDoc* m_parent;

    /** copy protection */
    bool m_copy;
    bool m_preEmp;

    K3b::Msf m_index0Offset;

    K3bDevice::TrackCdText m_cdText;

    // list
    K3bAudioTrack* m_prev;
    K3bAudioTrack* m_next;

    K3bAudioDataSource* m_firstSource;


    K3bAudioDataSource* m_currentSource;
    long long m_alreadyReadBytes;

    bool m_currentlyDeleting;

    class Private;
    Private* d;
};


#endif
