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
class K3bDirItem;
class K3bFileItem;

class K3bView;
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
	
	K3bDirItem* root() const { return m_root; }

	/** reimplemented from K3bDoc */
	K3bView* newView( QWidget* parent );
	/** reimplemented from K3bDoc */
	void addView(K3bView* view);

	bool newDocument();	
	int size();
	
	const QString& name() const { return m_name; }
	
	/** Informs all views that an item has been removed */
	void removeItem( K3bDataItem* item );
	
public slots:
	/** add urls to the compilation.
	  * @param dir the directory where to add the urls, by default this is the root directory.
	  **/
	void slotAddURLs( const QStringList&, K3bDirItem* dir = 0 );
//	void slotRemoveFile( K3bFileItem* );
//	void slotRemoveDir( K3bDirItem* );

signals:
	void newFile( K3bFileItem* );
	void newDir( K3bDirItem* );
	// TODO: remove files ???

	void signalAddDirectory( const QString& url, K3bDirItem* dir );
	void itemRemoved( K3bDataItem* );
	
private slots:
	void slotAddDirectory( const QString& url, K3bDirItem* dir );

protected:
 	/** reimplemented from K3bDoc */
 	bool loadDocumentData( QFile& f );
 	/** reimplemented from K3bDoc */
 	bool saveDocumentData( QFile& f );
	
private:
	K3bDirItem* m_root;
	QString m_name;
};

#endif
