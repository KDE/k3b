/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bdatapreparationjob.h"
#include "k3bdatadoc.h"
#include "k3bisooptions.h"

#include <k3bthreadjob.h>
#include <k3bthread.h>
#include <k3bdiritem.h>
#include <k3bfileitem.h>
#include <k3bglobals.h>

#include <klocale.h>
#include <kstringhandler.h>

#include <qfile.h>
#include <qvaluelist.h>


class K3bDataPreparationJob::Private : public K3bThread
{
public:
  Private( K3bDataDoc* doc );

  void run();
  void cancel();

  K3bDataDoc* doc;

  QValueList<K3bDataItem*> nonExistingItems;
  QString listOfRenamedItems;
  QValueList<K3bDataItem*> folderSymLinkItems;

  K3bThreadJob* threadJob;

  bool canceled;
};


K3bDataPreparationJob::Private::Private( K3bDataDoc* _doc )
  : doc(_doc)
{
}


void K3bDataPreparationJob::Private::run()
{
  emitStarted();

  // clean up
  nonExistingItems.clear();
  listOfRenamedItems.truncate(0);
  folderSymLinkItems.clear();

  // initialize filenames in the project
  doc->prepareFilenames();

  // create the message string for the renamed files
  if( doc->needToCutFilenames() ) {
    int maxlines = 10;
    QValueList<K3bDataItem*>::const_iterator it;
    for( it = doc->needToCutFilenameItems().begin(); 
	 maxlines > 0 && it != doc->needToCutFilenameItems().end();
	 ++it, --maxlines ) {
      K3bDataItem* item = *it;
      listOfRenamedItems += i18n("<em>%1</em> renamed to <em>%2</em>")
	.arg( KStringHandler::csqueeze( item->k3bName(), 30 ) )
	.arg( KStringHandler::csqueeze( item->writtenName(), 30 ) );
      listOfRenamedItems += "<br>";
    }
    if( it != doc->needToCutFilenameItems().end() )
      listOfRenamedItems += "...";
  }

  //
  // Check for missing files and folder symlinks
  //
  K3bDataItem* item = doc->root();
  while( (item = item->nextSibling()) ) {

    if( item->isSymLink() ) {
      if( doc->isoOptions().followSymbolicLinks() ) {
	QFileInfo f( K3b::resolveLink( item->localPath() ) );
	if( !f.exists() ) {
	  nonExistingItems.append( item );
	}
	else if( f.isDir() ) {
	  folderSymLinkItems.append( item );
	}
      }
    }
    else if( item->isFile() && !QFile::exists( item->localPath() ) ) {
      nonExistingItems.append( item );
    }

    if( canceled ) {
      emitCanceled();
      emitFinished(false);
      return;
    }
  }


  emitFinished( true );
}


void K3bDataPreparationJob::Private::cancel()
{
  canceled = true;
}




static QString createItemsString( const QValueList<K3bDataItem*>& items, unsigned int max )
{
  QString s;
  unsigned int cnt = 0;
  for( QValueList<K3bDataItem*>::const_iterator it = items.begin();
       it != items.end(); ++it ) {

    s += KStringHandler::csqueeze( (*it)->localPath(), 60 );

    ++cnt;
    if( cnt >= max || it == items.end() )
      break;

    s += "<br>";
  }

  if( items.count() > max )
    s += "...";

  return s;
}


K3bDataPreparationJob::K3bDataPreparationJob( K3bDataDoc* doc, K3bJobHandler* hdl, QObject* parent )
  : K3bJob( hdl, parent )
{
  d = new Private( doc );
  d->threadJob = new K3bThreadJob( d, this, this );
  connectSubJob( d->threadJob, SLOT(slotWorkDone(bool)), K3bJob::DEFAULT_SIGNAL_CONNECTION );
}


K3bDataPreparationJob::~K3bDataPreparationJob()
{
  delete d;
}


void K3bDataPreparationJob::start()
{
  if( !active() ) {
    d->canceled = false;
    jobStarted();
    d->threadJob->start();
  }
}


void K3bDataPreparationJob::slotWorkDone( bool success )
{
  if( success ) {
    if( !d->listOfRenamedItems.isEmpty() ) {
      if( !questionYesNo( "<p>" + i18n("Some filenames need to be shortened due to the %1 char restriction "
				       "of the Joliet extensions. If the Joliet extensions are disabled filenames "
				       "do not have to be shortened but long filenames will not be available on "
				       "Windows systems.")
			  .arg( d->doc->isoOptions().jolietLong() ? 103 : 64 )
			  + "<p>" + d->listOfRenamedItems,
			  i18n("Warning"),
			  i18n("Shorten Filenames"),
			  i18n("Disable Joliet extensions") ) ) {
	// No -> disable joliet
	// for now we enable RockRidge to be sure we did not lie above (keep long filenames)
	K3bIsoOptions op = d->doc->isoOptions();
	op.setCreateJoliet( false );
	op.setCreateRockRidge( true );
	d->doc->setIsoOptions( op );
	d->doc->prepareFilenames();
      }
    }

    //
    // The joliet extension encodes the volume desc in UCS-2, i.e. uses 16 bit for each char.
    // Thus, the max length here is 16.
    //
    if( d->doc->isoOptions().createJoliet() &&
	d->doc->isoOptions().volumeID().length() > 16 ) {
      if( !questionYesNo( "<p>" + i18n("The Joliet extensions (which are needed for long filenames on Windows systems) "
				       "restrict the length of the volume descriptior (the name of the filesystem) "
				       "to %1 characters. The selected descriptor '%2' is longer than that. Do you "
				       "want it to be cut or do you want to go back and change it manually?")
			  .arg( 16 ).arg( d->doc->isoOptions().volumeID() ),
			  i18n("Warning"),
			  i18n("Cut volume descriptor in the Joliet tree"),
			  i18n("Cancel and go back") ) ) {
	d->canceled = true;
	emit canceled();
	jobFinished( false );
	return;
      }
    }

    //
    // Check for missing files
    //
    if( !d->nonExistingItems.isEmpty() ) {
      if( questionYesNo( "<p>" + i18n("The following files could not be found. Do you want to remove them from the "
				      "project and continue without adding them to the image?") + 
			 "<p>" + createItemsString( d->nonExistingItems, 10 ),
			 i18n("Warning"),
			 i18n("Remove missing files and continue"),
			 i18n("Cancel and go back") ) ) {
	for( QValueList<K3bDataItem*>::const_iterator it = d->nonExistingItems.begin();
	     it != d->nonExistingItems.end(); ++it ) {
	  delete *it;
	}
      }
      else {
	d->canceled = true;
	emit canceled();
	jobFinished(false);
	return;
      }
    }

    //
    // Warn about symlinks to folders
    //
    if( d->doc->isoOptions().followSymbolicLinks() && !d->folderSymLinkItems.isEmpty() ) {
      if( !questionYesNo( "<p>" + i18n("K3b is not able to follow symbolic links to folders after they have been added "
				       "to the project. Do you want to continue "
				       "without writing the symbolic links to the image?") +
			  "<p>" + createItemsString( d->folderSymLinkItems, 10 ),
			  i18n("Warning"),
			  i18n("Discard symbolic links to folders"),
			  i18n("Cancel and go back") ) ) {
	d->canceled = true;
	emit canceled();
	jobFinished(false);
	return;
      }
    }

    jobFinished( true );
  }
  else {
    if( d->canceled )
      emit canceled();
    jobFinished(false);
  }
}


void K3bDataPreparationJob::cancel()
{
  d->cancel();
}


bool K3bDataPreparationJob::hasBeenCanceled() const
{
  return d->canceled;
}

#include "k3bdatapreparationjob.moc"
