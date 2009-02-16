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


#ifndef K3BDOC_H
#define K3BDOC_H

#include "config-k3b.h"
#include "k3bglobals.h"

// include files for QT
#include <qobject.h>
#include <qstring.h>


// include files for KDE
#include <kurl.h>
#include <kio/global.h>
#include "k3b_export.h"

// forward declaration of the K3b classes
class K3bBurnJob;
class QDomElement;
class K3bJobHandler;


namespace K3bDevice {
    class Device;
}
namespace K3b {
    class Msf;
}

/**
 * K3bDoc is the base document class.
 * It handles some general settings.
 */
class LIBK3B_EXPORT K3bDoc : public QObject
{
    Q_OBJECT

public:
    K3bDoc( QObject* = 0 );
    virtual ~K3bDoc();

    enum DocType { 
        AUDIO = 1, 
        DATA, 
        MIXED, 
        VCD, 
        MOVIX,
        VIDEODVD 
    };

    virtual int type() const { return m_docType; }

    /**
     * \return A name for the project which might for example be used as a suggestion for a file name
     *         when saving. The default implementation extracts a name from the URL.
     */
    virtual QString name() const;

    /**
     * \return A string representation of the document type.
     */
    virtual QString typeString() const = 0;

    /**
     * The media types that are supported by this project type.
     * The default implementation returns all writable media types.
     * This should NOT take into accout settings like the writing mode
     * or anything that can be changed in the burn dialog.
     */
    virtual int supportedMediaTypes() const;

    /** 
     * returns the view widget set with setView() or null if none has been set.
     */
    QWidget* view() const { return m_view; }

    /**
     * Just for convenience to make an easy mapping from doc to GUI possible.
     */
    void setView( QWidget* v ) { m_view = v; }

    /** 
     * sets the modified flag for the document after a modifying action on the view connected to the document.
     */
    virtual void setModified( bool m = true );

    /** 
     * returns if the document is modified or not. Use this to determine 
     * if your document needs saving by the user on closing.
     */
    virtual bool isModified() const { return m_modified; }

    /**
     * Subclasses should call this when reimplementing.
     * Sets some defaults.
     * FIXME: this method is completely useless. Just do it all in the constructor
     */
    virtual bool newDocument();

    /**
     * Clear project, i.e. remove all data that has ben added
     */
    virtual void clear() = 0;

    /**
     * Load a project from an xml stream.
     *
     * This is used to load/save k3b projects. 
     */
    virtual bool loadDocumentData( QDomElement* root ) = 0;

    /**
     * Save a project to an xml stream.
     *
     * This is used to load/save k3b projects. 
     */
    virtual bool saveDocumentData( QDomElement* docElem ) = 0;

    /** returns the KUrl of the document */
    const KUrl& URL() const;
    /** sets the URL of the document */
    virtual void setURL( const KUrl& url );

    K3b::WritingMode writingMode() const { return m_writingMode; }
    bool dummy() const { return m_dummy; }
    bool onTheFly() const { return m_onTheFly; }
    bool removeImages() const { return m_removeImages; }
    bool onlyCreateImages() const { return m_onlyCreateImages; }
    int copies() const { return m_copies; }
    int speed() const { return m_speed; }
    K3bDevice::Device* burner() const { return m_burner; }

    /**
     * \return the size that will actually be burnt to the medium.
     * This only differs from size() for multisession projects.
     */
    virtual KIO::filesize_t burningSize() const;
    virtual KIO::filesize_t size() const = 0;
    virtual K3b::Msf length() const = 0;

    // FIXME: rename this to something like imagePath
    const QString& tempDir() const { return m_tempDir; }

    virtual int numOfTracks() const { return 1; }

    /**
     * Create a new BurnJob to burn this project. It is not mandatory to use this
     * method. You may also just create the BurnJob you need manually. It is just 
     * easier this way since you don't need to distinguish between the different
     * project types.
     */
    virtual K3bBurnJob* newBurnJob( K3bJobHandler*, QObject* parent = 0 ) = 0;

    K3b::WritingApp writingApp() const { return m_writingApp; }
    void setWritingApp( K3b::WritingApp a ) { m_writingApp = a; }

    /**
     * @return true if the document has successfully been saved to a file
     */
    bool isSaved() const { return m_saved; }

    /**
     * Used for session management. Use with care.
     */
    void setSaved( bool s ) { m_saved = s; }

Q_SIGNALS:
    void changed();
    void changed( K3bDoc* );

public Q_SLOTS:
    void setDummy( bool d );
    void setWritingMode( K3b::WritingMode m ) { m_writingMode = m; }
    void setOnTheFly( bool b ) { m_onTheFly = b; }
    void setSpeed( int speed );
    void setBurner( K3bDevice::Device* dev );
    void setTempDir( const QString& dir ) { m_tempDir = dir; }
    void setRemoveImages( bool b ) { m_removeImages = b; }
    void setOnlyCreateImages( bool b ) { m_onlyCreateImages = b; }
    void setCopies( int c ) { m_copies = c; }

    /**
     * the default implementation just calls addUrls with
     * list containing the url
     */
    virtual void addUrl( const KUrl& url );
    virtual void addUrls( const KUrl::List& urls ) = 0;

protected:
    int m_docType;

    bool saveGeneralDocumentData( QDomElement* );

    bool readGeneralDocumentData( const QDomElement& );

private Q_SLOTS:
    void slotChanged();

private:
    /** the modified flag of the current document */
    bool m_modified;
    KUrl doc_url;

    QWidget* m_view;

    QString m_tempDir;
    K3bDevice::Device* m_burner;
    bool m_dummy;
    bool m_onTheFly;
    bool m_removeImages;
    bool m_onlyCreateImages;
    int  m_speed;

    /** see k3bglobals.h */
    K3b::WritingApp m_writingApp;

    K3b::WritingMode m_writingMode;

    int m_copies;

    bool m_saved;
};

#endif // K3BDOC_H
