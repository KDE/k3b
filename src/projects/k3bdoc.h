/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BDOC_H
#define K3BDOC_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// include files for QT
#include <qobject.h>
#include <qstring.h>
#include <qptrlist.h>


// include files for KDE
#include <kurl.h>
#include <kio/global.h>


// forward declaration of the K3b classes
class K3bView;
class QTimer;
class KTempFile;
class K3bBurnJob;
class K3bProjectBurnDialog;
class QDomDocument;
class QDomElement;
class KConfig;
class KActionCollection;


namespace K3bCdDevice {
  class CdDevice;
}
namespace K3b {
  class Msf;
}

/**
 * K3bDoc is the base document class.
 * It takes care of writing to a KoStore and handles some general settings.
 */
class K3bDoc : public QObject
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
    MOVIX_DVD,
    DVD,
    VIDEODVD 
  };

  virtual KActionCollection* actionCollection() const { return m_actionCollection; }

  virtual int docType() const { return m_docType; }

  /** 
   * Create a new view
   * This should only be called once.
   */
  K3bView* createView( QWidget* parent = 0, const char* name = 0 );

  /** 
   * returns the K3bView created by newView() or null if none has been created
   */
  K3bView* view() const { return m_view; }

  /** 
   * sets the modified flag for the document after a modifying action on the view connected to the document.
   */
  void setModified( bool m = true) { m_modified = m; }

  /** 
   * returns if the document is modified or not. Use this to determine 
   * if your document needs saving by the user on closing.
   */
  virtual bool isModified() const { return m_modified; }

  /**
   * Subclasses should call this when reimplementing.
   * Sets some defaults.
   */
  virtual bool newDocument();

  /**
   * Since we have several types of K3b projects we cannot follow the standard and
   * allow opening an URL in an already created doc. So this static method checks
   * the type of the saved project and creates a appropriate project object or 0
   * if no K3b project has been found.
   */
  static K3bDoc* openDocument(const KURL &url);

  /**
   * saves the document under filename and format.
   */
  bool saveDocument(const KURL &url);

  /** returns the KURL of the document */
  const KURL& URL() const;
  /** sets the URL of the document */
  void setURL(const KURL& url);

  int writingMode() const { return m_writingMode; }
  bool dummy() const { return m_dummy; }
  bool onTheFly() const { return m_onTheFly; }
  bool burnproof() const { return m_burnproof; }
  bool overburn() const { return m_overburn; }
  bool removeImages() const { return m_removeImages; }
  bool onlyCreateImages() const { return m_onlyCreateImages; }

  int speed() const { return m_speed; }
  K3bCdDevice::CdDevice* burner() const { return m_burner; }
  virtual KIO::filesize_t size() const = 0;
  virtual K3b::Msf length() const = 0;

  const QString& tempDir() const { return m_tempDir; }

  virtual int numOfTracks() const { return 1; }

  virtual K3bBurnJob* newBurnJob() = 0;

  int writingApp() const { return m_writingApp; }
  void setWritingApp( int a ) { m_writingApp = a; }

  /**
   * @return true if the document has successfully been saved to a file
   */
  bool saved() const { return m_saved; }

  /**
   * Should return the name of the document type.
   * This is used for saving the contents in a XML file
   * and naming the config group to store the default settings in.
   */
  virtual QString documentType() const = 0;

/*   virtual K3bProjectInterface* dcopInterface(); */
/*   QCString dcopId(); */

 signals:
  void changed();

 public slots:
  /**
   * Default impl. brings up the burnDialog via newBurnDialog() with writing
   */
  virtual void slotBurn();

  /**
   * Default impl. brings up the burnDialog via newBurnDialog() without writing
   */
  virtual void slotProperties();

  void setDummy( bool d );
  void setWritingMode( int m ) { m_writingMode = m; }
  void setOnTheFly( bool b ) { m_onTheFly = b; }
  void setOverburn( bool b ) { m_overburn = b; }
  void setSpeed( int speed );
  void setBurner( K3bCdDevice::CdDevice* dev );
  void setBurnproof( bool b ) { m_burnproof = b; }
  void setTempDir( const QString& dir ) { m_tempDir = dir; }
  void setRemoveImages( bool b ) { m_removeImages = b; }
  void setOnlyCreateImages( bool b ) { m_onlyCreateImages = b; }

  /**
   * the default implementation just calls addUrls with
   * list containing the url
   */
  virtual void addUrl( const KURL& url );
  virtual void addUrls( const KURL::List& urls ) = 0;

  /**
   * load the default project settings from the app configuration
   * the default implementation opens the correct group and loads
   * the following settings:
   * <ul>
   *   <li>Writing mode</li>
   *   <li>Simulate</li>
   *   <li>on the fly</li>
   *   <li>burnfree</li>
   *   <li>remove images</li>
   *   <li>only create images</li>
   *   <li>writer</li>
   *   <li>writing speed</li>
   * </ul>
   * New implementations should call this before doing anything else.
   * After that there is no need to change the config group.
   */
  virtual void loadDefaultSettings( KConfig* );

 protected:
  /**
   * when deriving from K3bDoc this method really opens the document since
   * openDocument only opens a tempfile and calls this method.
   */
  virtual bool loadDocumentData( QDomElement* root ) = 0;

  /**
   * when deriving from K3bDoc this method really saves the document since
   * saveDocument only opens the file and calls this method.
   * Append all child elements to docElem.
   * XML header was already created
   */
  virtual bool saveDocumentData( QDomElement* docElem ) = 0;

  bool saveGeneralDocumentData( QDomElement* );
  bool readGeneralDocumentData( const QDomElement& );

  int m_docType;

  //  K3bProjectInterface* m_dcopInterface;

  /**
   * Protected since the BurnDialog is not part of the API.
   */
  virtual K3bProjectBurnDialog* newBurnDialog( QWidget* = 0, const char* = 0 ) = 0;
  virtual K3bView* newView( QWidget* parent = 0 ) = 0;

 private:
  /** the modified flag of the current document */
  bool m_modified;
  KURL doc_url;

  K3bView* m_view;
  QString m_tempDir;
  K3bCdDevice::CdDevice* m_burner;
  bool m_dummy;
  bool m_onTheFly;
  bool m_burnproof;
  bool m_overburn;
  bool m_removeImages;
  bool m_onlyCreateImages;
  int  m_speed;

  /** see k3bglobals.h */
  int m_writingApp;

  int m_writingMode;

  bool m_saved;

  KActionCollection* m_actionCollection;
};

#endif // K3BDOC_H
