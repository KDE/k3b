/***************************************************************************
                          k3bdoc.cpp  -  description
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

// include files for Qt
#include <qdir.h>
#include <qfileinfo.h>
#include <qwidget.h>
#include <qstring.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qtimer.h>

// include files for KDE
#include <klocale.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <kprocess.h>
#include <kapp.h>
#include <kstddirs.h>
#include <klocale.h>

// application specific includes
#include "k3b.h"
#include "k3bview.h"
#include "k3bdoc.h"
#include "k3bglobals.h"

K3bDoc::K3bDoc( QObject* parent )
	: QObject( parent )
{
	pViewList = new QList<K3bView>;
	pViewList->setAutoDelete(false);

	m_process = new KProcess();
	m_burner = 0;
	m_dao = true;
	m_error = K3b::NOT_STARTED;
	m_speed = 1;
}


K3bDoc::~K3bDoc()
{
	delete m_process;
	delete pViewList;
}

void K3bDoc::setDao( bool b )
{
	m_dao = b;
}

void K3bDoc::setDummy( bool b )
{
	m_dummy = b;
}

void K3bDoc::setSpeed( int speed )
{
	m_speed = speed;
}

void K3bDoc::setBurner( K3bDevice* dev )
{
	m_burner = dev;
	if( dev )
		qDebug( QString("(K3bDoc) Setting writer to %1 %2").arg( dev->device()).arg(dev->description) );
}


void K3bDoc::addView(K3bView *view)
{
	pViewList->append(view);
	changedViewList();
}

void K3bDoc::removeView(K3bView *view)
{
	  pViewList->remove(view);
	  if(!pViewList->isEmpty())
			changedViewList();
		else
			deleteContents();
}

void K3bDoc::changedViewList(){	
	
	K3bView *w;
	if((int)pViewList->count() == 1){
  	w=pViewList->first();
  	w->setCaption(URL().fileName());
	}
	else{	
		int i;
    for( i=1,w=pViewList->first(); w!=0; i++, w=pViewList->next())
  		w->setCaption(QString(URL().fileName()+":%1").arg(i));	
	}
}

bool K3bDoc::isLastView() {
  return ((int) pViewList->count() == 1);
}


void K3bDoc::updateAllViews(K3bView *sender)
{
  K3bView *w;
  for(w=pViewList->first(); w!=0; w=pViewList->next())
  {
     w->update(sender);
  }

}

void K3bDoc::setURL(const KURL &url)
{
  doc_url=url;
}

const KURL& K3bDoc::URL() const
{
  return doc_url;
}

void K3bDoc::closeDocument()
{
  K3bView *w;
  if(!isLastView())
  {
    for(w=pViewList->first(); w!=0; w=pViewList->next())
    {
   	 	if(!w->close())
 				break;
    }
	}
  if(isLastView())
  {
  	w=pViewList->first();
  	w->close();
  }
}

bool K3bDoc::newDocument()
{
	m_dummy = false;
	m_dao = true;
	m_onTheFly = false;
	
	modified=false;
	return true;
}

bool K3bDoc::openDocument(const KURL& url, const char* )
{
  QString tmpfile;
  KIO::NetAccess::download( url, tmpfile );

  /////////////////////////////////////////////////
	QFile f( tmpfile );
	if ( !f.open( IO_ReadOnly ) )
		return false;
	
	// load the data into the document	
	bool success = loadDocumentData( f );
	
	f.close();

  /////////////////////////////////////////////////
  KIO::NetAccess::removeTempFile( tmpfile );
  doc_url=url;
	
  modified=false;
  return success;
}

bool K3bDoc::saveDocument(const KURL&, const char* )
{
//  KTempFile( locateLocal( "tmp", "k3b"
//	QFile f( filename );
//	if ( !f.open( IO_WriteOnly ) )
//		return false;
//
//  bool success = saveDocumentData( f );
//
//  f.close();
//
//  modified=false;
//	m_filename=filename;
//	m_title=QFileInfo(f).fileName();
//  return success;
  return true;
}

void K3bDoc::deleteContents()
{
  /////////////////////////////////////////////////
  // TODO: Add implementation to delete the document contents
  /////////////////////////////////////////////////

}

bool K3bDoc::canCloseFrame(K3bView* pFrame)
{
	if(!isLastView())
		return true;
		
	bool ret=false;
  if(isModified())
  {
		KURL saveURL;
  	switch(KMessageBox::warningYesNoCancel(pFrame, i18n("The current file has been modified.\n"
                          "Do you want to save it?"),URL().fileName()))
    {
			case KMessageBox::Yes:
				if(URL().fileName().contains(i18n("Untitled")))
				{
					saveURL=KFileDialog::getSaveURL(QDir::currentDirPath(),
                      i18n("*|All files"), pFrame, i18n("Save as..."));
          if(saveURL.isEmpty())
          	return false;
				}
				else
						saveURL=URL();
					
				if(!saveDocument(saveURL))
				{
 					switch(KMessageBox::warningYesNo(pFrame,i18n("Could not save the current document !\n"
																												"Close anyway ?"), i18n("I/O Error !")))
 					{
 						case KMessageBox::Yes:
 							ret=true;
 						case KMessageBox::No:
 							ret=false;
 					}	        			
				}
				else
					ret=true;
				break;
			case KMessageBox::No:
				ret=true;
				break;
			case KMessageBox::Cancel:
			default:
				ret=false; 				
				break;
		}
	}
	else
		ret=true;
		
	return ret;
}
