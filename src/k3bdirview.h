/***************************************************************************
                          k3bdirview.h  -  description
                             -------------------
    begin                : Mon Mar 26 2001
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

#ifndef K3BDIRVIEW_H
#define K3BDIRVIEW_H

#include <qsplitter.h>
#include <qfile.h>
#include <qstring.h>

#include <klistview.h>
#include <kdiroperator.h>
#include <kfiledetailview.h>

class KFileViewItem;
class QDragObject;

/**
  *@author Sebastian Trueg
  */


class K3bDirView : public QSplitter  {
   Q_OBJECT

public:
	K3bDirView(QWidget *parent=0, const char *name=0);
	~K3bDirView();

public slots:
	void slotFolderSelected( const QString& );
	void slotFileItemSelected( const KFileViewItem* );
	
private:     	
	class PrivateFileView : public KFileDetailView
     {
     public:
     	PrivateFileView( QWidget* parent, const char* name );
     	
     protected:
     	QDragObject* dragObject() const;
     }; // class PrivateFileView

    class PrivateDirItem : public QListViewItem
    {
    public:
        PrivateDirItem( QListView* parent, const QString& filename, const QString& altName = QString::null );
        PrivateDirItem( PrivateDirItem* parent, const QString& filename, const QString& altName = QString::null );

        QString text( int ) const;

        QString absPath();

        void setOpen( bool );
        void setup();

        const QPixmap* pixmap( int i ) const;
        void setPixmap( QPixmap *p );

    private:
    	/** Reads all subdirectories **/
    	void expandDirItem( );
    	
    	QString m_altName;
        QFile f;
        K3bDirView::PrivateDirItem* p;
        bool readable;
        QPixmap *pix;
    }; // class PrivateDirItem

    class PrivateDirView : public KListView
    {
        Q_OBJECT

    public:
        PrivateDirView( QWidget *parent = 0, const char *name = 0 );

    public slots:
        void setDir( const QString & );
    		//void reload();
    		
    signals:
        void folderSelected( const QString & );

    protected slots:
        void slotFolderSelected( QListViewItem* );
    }; // class PrivateDirView

	PrivateDirView* m_dirView;
	KDirOperator* m_fileView;
};

#endif
