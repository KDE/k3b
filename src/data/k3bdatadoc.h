/***************************************************************************
                          k3bdatadoc.h  -  description
                             -------------------
    begin                : Sun Apr 22 2001
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

#ifndef K3BDATADOC_H
#define K3BDATADOC_H

#include "../k3bdoc.h"

class K3bDataItem;
class K3bRootItem;
class K3bDirItem;
class K3bFileItem;
class K3bJob;

class K3bView;
class QString;
class QStringList;
class QWidget;

/**
  *@author Sebastian Trueg
  */

class K3bDataDoc : public K3bDoc
{
	Q_OBJECT

public:
	K3bDataDoc( QObject* parent );
	~K3bDataDoc();
	
	K3bRootItem* root() const { return m_root; }

	/** reimplemented from K3bDoc */
	K3bView* newView( QWidget* parent );
	/** reimplemented from K3bDoc */
	void addView(K3bView* view);

	bool newDocument();	
	int size();
	
	const QString& name() const { return m_name; }
	
	void removeItem( K3bDataItem* item );
	
	/** writes a mkisofs-path-spec file with graft-points **/
	QString writePathSpec( const QString& fileName );
	
	/** returns an empty dummy dir for use with K3bDirItems.
		Creates one if nessessary.
		The dummy dir is used to create empty dirs on the iso-filesystem! */
	const QString& dummyDir();
	const QString& isoImage() const { return m_isoImage; }
	void setIsoImage( const QString& s ) { m_isoImage = s; }
	
	bool createRockRidge() const { return m_createRockRidge; }
	bool createJoliet() const { return m_createJoliet; }

	void setCreateRockRidge( bool b ) { m_createRockRidge = b; }
	void setCreateJoliet( bool b ) { m_createJoliet = b; }
		
	K3bBurnJob* newBurnJob();
	
public slots:
	/** add urls to the compilation.
	  * @param dir the directory where to add the urls, by default this is the root directory.
	  **/
	void slotAddURLs( const QStringList&, K3bDirItem* dir = 0 );

signals:
	void newFile( K3bFileItem* );
	void newDir( K3bDirItem* );
	void itemRemoved( K3bDataItem* );
	
private slots:
	void slotAddDirectory( const QString& url, K3bDirItem* dir );

protected:
 	/** reimplemented from K3bDoc */
 	bool loadDocumentData( QFile& f );
 	/** reimplemented from K3bDoc */
 	bool saveDocumentData( QFile& f );
	
private:
	K3bRootItem* m_root;
	QString m_name;
	QString m_dummyDir;
	QString m_isoImage;
			
	bool m_createRockRidge;    // -r or -R
	bool m_createJoliet;             // -J
	bool m_ISOallowLowercase;   // -allow-lowercase
	bool m_ISOallowPeriodAtBegin;   // -L
	bool m_ISOallow31charFilenames;  // -I
	bool m_ISOomitVersionNumbers;   // -N
	bool m_ISOmaxFilenameLength;     // -max-iso9660-filenames (forces -N)
	bool m_ISOrelaxedFilenames;      // -relaxed-filenames
	bool m_ISOnoIsoTranslate;        // -no-iso-translate
	bool m_ISOallowMultiDot;          // -allow-multidot
	bool m_ISOuntranslatedFilenames;   // -U (forces -d, -I, -L, -N, -relaxed-filenames, -allow-lowercase, -allow-multidot, -no-iso-translate)
	bool m_noDeepDirectoryRelocation;   // -D
	bool m_followSymbolicLinks;       // -f
	bool m_hideRR_MOVED;  // -hide-rr-moved
	bool m_createTRANS_TBL;    // -T
	bool m_hideTRANS_TBL;    // -hide-joliet-trans-tbl
	bool m_padding;           // -pad
};

#endif
