/***************************************************************************
                          k3bdoc.h  -  description
                             -------------------
    begin                : Mon Mar 26 15:30:59 CEST 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BDOC_H
#define K3B_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

// include files for QT
#include <qobject.h>
#include <qstring.h>
#include <qlist.h>
#include <qfile.h>

#include "k3bdevicemanager.h"

// include files for KDE
#include <kurl.h>

// forward declaration of the K3b classes
class K3bView;
class QTimer;
class KTempFile;
class K3bDevice;
class KProcess;
class K3bApp;

/**	K3bDoc provides a document object for a document-view model.
  *
  * The K3bDoc class provides a document object that can be used in conjunction with the classes
  * K3bApp and K3bView to create a document-view model for MDI (Multiple Document Interface)
  * KDE 2 applications based on KApplication and KTMainWindow as main classes and QWorkspace as MDI manager widget.
  * Thereby, the document object is created by the K3bApp instance (and kept in a document list) and contains
  * the document structure with the according methods for manipulating the document
  * data by K3bView objects. Also, K3bDoc contains the methods for serialization of the document data
  * from and to files.
  * @author Source Framework Automatically Generated by KDevelop, (c) The KDevelop Team. 	
  * @version KDevelop version 1.3 code generation
  */
class K3bDoc : public QObject
{
  Q_OBJECT

  friend K3bView;

public:
    /** Constructor for the fileclass of the application */
    K3bDoc( QObject* );
    /** Destructor for the fileclass of the application */
    ~K3bDoc();

	enum DocType { AUDIO = 1, DATA = 2 };

    /** adds a view to the document which represents the document contents. Usually this is your main view. */
    virtual void addView(K3bView *view);
    /** removes a view from the list of currently connected views */
    void removeView(K3bView *view);
	/** gets called if a view is removed or added */
	void changedViewList();
    /** returns the first view instance */
	K3bView* firstView(){ return pViewList->first(); };
	/** returns true, if the requested view is the last view of the document */
    bool isLastView();
    /** This method gets called when the user is about to close a frame window. It checks, if more than one view
    	* is connected to the document (then the frame can be closed), if pFrame is the last view and the document is
    	* modified, the user gets asked if he wants to save the document.
    	*/
	bool canCloseFrame(K3bView* pFrame);
    /** sets the modified flag for the document after a modifying action on the view connected to the document.*/
    void setModified(bool _m=true){ modified=_m; };
    /** returns if the document is modified or not. Use this to determine if your document needs saving by the user on closing.*/
    bool isModified(){ return modified; };
    /** deletes the document's contents */
    virtual void deleteContents();
    /** this virtual version only sets the modified flag */
    virtual bool newDocument();
    /** closes the acutal document */
    void closeDocument();
    /** loads the document by filename and format and emits the updateViews() signal */
    bool openDocument(const KURL &url, const char *format=0);
    /** saves the document under filename and format.*/	
    bool saveDocument(const KURL &url, const char *format=0);
    /** returns the KURL of the document */
    const KURL& URL() const;
    /** sets the URL of the document */
	void setURL(const KURL& url);
	
	  /** Create a new view */
  	virtual K3bView* newView( QWidget* parent ) = 0;

 	/**
	 * For writing "on the fly "
	 * Connect to the signals (at least result())
	 * which will be deleted after the writing finished. To get the result
	 * use error().
	 **/
	virtual void write() = 0;

	/**
	 * Writing an image file to cdr. To create an image use
	 * @p writeImage.
	 **/
	void write( const QString& imageFile, bool deleteImage = true );
	virtual void writeImage( const QString& filename ) = 0;
	// vielleicht sollte man die 2. write-fkt einfach
	// iso-images schreiben lassen. Braucht man dann
	// noch ein TOC-file und kann man images auch track-at-once
	// schreiben?

	const QString& projectName() const { return m_projectName; }
	bool dao() const { return m_dao; }
	bool dummy() const { return m_dummy; }
	bool eject() const { return m_eject; }
	int speed() const { return m_speed; }
	K3bDevice* burner() const { return m_burner; }
	virtual int size() = 0;

	/**
	 * After result() has been emitted this returns the error-code
	 * to check the result.
	 **/
	int error() const;
	QString errorString() const;

	bool workInProgress() const;

public slots:
    /** calls repaint() on all views connected to the document object and is called by the view by which the document has been changed.
     * As this view normally repaints itself, it is excluded from the paintEvent.
     */
    void updateAllViews(K3bView *sender);
	void setDummy( bool d );
	void setDao( bool d );
	void setEject( bool e );
	void setSpeed( int speed );
	void setBurner( K3bDevice* dev );
	
	virtual void showBurnDialog() = 0;
	/** in the default implementation no canceled signal is emmited! */
	virtual void cancel();

protected slots:
	virtual void startRecording() = 0;
	virtual void parseCdrecordOutput( KProcess*, char* output, int len ) = 0;
	virtual void cdrecordFinished() = 0;

signals:
	void infoMessage( const QString& );
	void canceled();
	void result();
	void percent( int percent );
	void processedSize( unsigned long processed, unsigned long size );
	void timeLeft( const QTime& );
	void bufferStatus( int percent );
	void startWriting();
	void writingLeadOut();
	 	
protected:
  	/** when deriving from K3bDoc this method really opens the document since
	      openDocument only opens a tempfile and calls this method. */
	virtual bool loadDocumentData( QFile& f ) = 0;
	
	/** when deriving from K3bDoc this method really saves the document since
	    saveDocument only opens the file and calls this method. */
  	virtual bool saveDocumentData( QFile& f ) = 0;

	void emitResult();
	void emitCanceled();
	virtual void emitProgress( unsigned long size, unsigned long processed, int speed = 0 );
	void emitMessage( const QString& msg );

	QString findTempFile( const QString& ending );

	QTimer* m_timer;
	KProcess* m_process;
	
	int  m_error;
	
private:
    /** the modified flag of the current document */
    bool modified;
    KURL doc_url;
    /** the list of the views currently connected to the document */
    QList<K3bView> *pViewList;	
	QString m_projectName;
	K3bDevice* m_burner;
	bool m_dao;
	bool m_dummy;
	bool m_eject;
	int  m_speed;
};

#endif // K3BDOC_H
