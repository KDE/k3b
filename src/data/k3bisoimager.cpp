/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bisoimager.h"
#include "k3bdiritem.h"
#include "k3bbootitem.h"
#include "k3bdatadoc.h"
#include <k3bexternalbinmanager.h>
#include <device/k3bdevice.h>
#include <k3bprocess.h>
#include <k3bcore.h>
#include <k3bversion.h>

#include <kdebug.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <ktempfile.h>
#include <kio/netaccess.h>
#include <kio/global.h>
#include <kio/job.h>

#include <qfile.h>
#include <qregexp.h>
#include <qtimer.h>

#include <errno.h>
#include <string.h>


K3bIsoImager::K3bIsoImager( K3bDataDoc* doc, QObject* parent, const char* name )
  : K3bJob( parent, name ),
    m_doc( doc ),
    m_pathSpecFile(0),
    m_rrHideFile(0),
    m_jolietHideFile(0),
    m_noDeepDirectoryRelocation( false ),
    m_importSession( false ),
    m_process( 0 ),
    m_processSuspended(false),
    m_processExited(false),
    m_lastOutput(0),
    m_mkisofsPrintSizeResult( 0 ),
    m_fdToWriteTo(-1)
{
}


K3bIsoImager::~K3bIsoImager()
{
  cleanup();
}


void K3bIsoImager::writeToFd( int fd )
{
  m_fdToWriteTo = fd;
}


void K3bIsoImager::slotReceivedStdout( KProcess*, char* d, int len )
{
  // We need to deep copy the data since we cannot trust on KProcess to not emit any
  // output after being suspended
  // it seems to me that KProcess is not perfect here but who am I to blame ;)

  QByteArray* buf = new QByteArray;
  buf->duplicate( d, len );
  m_data.enqueue( buf );

  if( !m_processSuspended ) {
    m_process->suspend();
    m_processSuspended = true;

    outputData();
  }
}


void K3bIsoImager::outputData()
{
  // we do not need the old data any longer since it has been written
  if( m_lastOutput ) {
    delete m_lastOutput;
    m_lastOutput = 0;
  }
  
  // we need to keep the data until we are resumed since the data will mostly be written to 
  // a KProcess (see KProcess::writeStdin)
  m_lastOutput = m_data.dequeue();
  emit data( m_lastOutput->data(), m_lastOutput->size() );
}


void K3bIsoImager::resume()
{
  // if mkisofs is writing directly to another fd the
  // process never gets suspended since we are not connected to it's 
  // (non active anyway) stdout signal and m_data is always empty

  if( m_fdToWriteTo == -1 ) {
    // check if there is data left
    if( m_data.count() > 0 ) {
      outputData();
    }
    else {
      if( m_processExited ) {
	slotProcessExited( m_process );
      }
      else {
	m_processSuspended = false;
	m_process->resume();
      }
    }
  }
}


void K3bIsoImager::slotReceivedStderr( const QString& line )
{
  if( !line.isEmpty() ) {
    emit debuggingOutput( "mkisofs", line );

    if( line.contains( "done, estimate" ) ) {

      QString perStr = line;
      perStr.truncate( perStr.find('%') );
      bool ok;
      double p = perStr.toDouble( &ok );
      if( !ok ) {
	kdDebug() << "(K3bIsoImager) Parsing did not work for " << perStr << endl;
      }
      else {
	emit percent( (int)p );
      }
    }
    else if( line.contains( "extents written" ) ) {
      emit percent( 100 );
    }
    else {
      kdDebug() << "(mkisofs) " << line << endl;
    }
  }
}

void K3bIsoImager::slotProcessExited( KProcess* p )
{
  m_processExited = true;

  if( m_canceled )
    return;

  if( m_data.count() <= 0 ) {
    if( p->normalExit() ) {
      if( p->exitStatus() == 0 ) {
	emit finished( true );
      }
      else  {
	switch( p->exitStatus() ) {
	case 2:
	  // mkisofs seems to have a bug that prevents to use filenames 
	  // that contain one or more backslashes
	  // mkisofs 1.14 has the bug, 1.15a40 not
	  // TODO: find out the version that fixed the bug
	  if( m_containsFilesWithMultibleBackslashes &&
	      k3bcore->externalBinManager()->binObject( "mkisofs" )->version < K3bVersion( 1, 15, -1, "a40" ) ) {
	    emit infoMessage( i18n("Due to a bug in mkisofs <= 1.15a40, K3b is unable to handle "
				   "filenames that contain more than one backslash:"), ERROR );

	    break;
	  }
	  // otherwise just fall through
	default:
	  emit infoMessage( i18n("%1 returned an unknown error (code %2).").arg("mkisofs").arg(p->exitStatus()), 
			    K3bJob::ERROR );
	  emit infoMessage( strerror(p->exitStatus()), K3bJob::ERROR );
	  emit infoMessage( i18n("Please send me an email with the last output."), K3bJob::ERROR );
	}

	emit finished( false );
      }
    }
    else {
      emit infoMessage( i18n("mkisofs exited abnormally."), ERROR );
      emit finished( false );
    }

    cleanup();
  }
}


void K3bIsoImager::cleanup()
{
  // remove all temp files
  if( m_pathSpecFile ) delete m_pathSpecFile;
  if( m_rrHideFile ) delete m_rrHideFile;
  if( m_jolietHideFile ) delete m_jolietHideFile;

  // remove boot-images-temp files
  for( QStringList::iterator it = m_tempFiles.begin();
       it != m_tempFiles.end(); ++it )
    QFile::remove( *it );
  m_tempFiles.clear();

  m_pathSpecFile = m_jolietHideFile = m_rrHideFile = 0;

  delete m_process;
  m_process = 0;
}


void K3bIsoImager::calculateSize()
{
  // determine iso-size
  cleanup();

  m_process = new K3bProcess();
  m_process->setRunPrivileged(true);

  if( !prepareMkisofsFiles() || 
      !addMkisofsParameters() ) {
    cleanup();
    emit sizeCalculated( ERROR, 0 );
    return;
  }

  *m_process << "-print-size" << "-quiet";
  // add empty dummy dir since one path-spec is needed
  *m_process << m_doc->dummyDir();

  kdDebug() << "***** mkisofs parameters:\n";
  const QValueList<QCString>& args = m_process->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    s += *it + " ";
  }
  kdDebug() << s << endl << flush;


  // since output changed during mkisofs version changes we grab both
  // stdout and stderr

  // mkisofs version >= 1.15 (don't know about 1.14!)
  // the extends on stdout (as lonely number)
  // and error and warning messages on stderr

  // mkisofs >= 1.13
  // everything is written to stderr
  // last line is: "Total extents scheduled to be written = XXXXX"

  connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotCollectMkisofsPrintSizeStderr(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotCollectMkisofsPrintSizeStdout(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotMkisofsPrintSizeFinished()) );

  m_collectedMkisofsPrintSizeStdout = QString::null;
  m_collectedMkisofsPrintSizeStderr = QString::null;
  m_mkisofsPrintSizeResult = 0;

  if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
    emit infoMessage( i18n("Could not start mkisofs!"), K3bJob::ERROR );
    cleanup();

    emit sizeCalculated( ERROR, 0 );
    return;
  }
}


void K3bIsoImager::slotCollectMkisofsPrintSizeStderr(KProcess*, char* data , int len)
{
  emit debuggingOutput( "mkisofs", QString::fromLocal8Bit( data, len ) );
  m_collectedMkisofsPrintSizeStderr.append( QString::fromLocal8Bit( data, len ) );
}


void K3bIsoImager::slotCollectMkisofsPrintSizeStdout(KProcess*, char* data, int len )
{
  emit debuggingOutput( "mkisofs", QString::fromLocal8Bit( data, len ) );
  m_collectedMkisofsPrintSizeStdout.append( QString::fromLocal8Bit( data, len ) );
}


void K3bIsoImager::slotMkisofsPrintSizeFinished()
{
  bool success = true;

  kdDebug() << "(K3bIsoImager) iso size: " << m_collectedMkisofsPrintSizeStdout << endl;

  // if m_collectedMkisofsPrintSizeStdout is not empty we have a recent version of
  // mkisofs and parsing is very easy (s.o.)
  if( !m_collectedMkisofsPrintSizeStdout.isEmpty() ) {
    m_mkisofsPrintSizeResult = m_collectedMkisofsPrintSizeStdout.toInt( &success );
  }
  else {
    // parse the stderr output
    // I hope parsing the last line is enough!
    int pos = m_collectedMkisofsPrintSizeStderr.findRev( "extents scheduled to be written" );

    if( pos == -1 )
      success = false;
    else
      m_mkisofsPrintSizeResult = m_collectedMkisofsPrintSizeStderr.mid( pos+33 ).toInt( &success );
  }



  if( success ) {
    emit sizeCalculated( PROCESS, m_mkisofsPrintSizeResult );
  }
  else {
    m_mkisofsPrintSizeResult = 0;
    kdDebug() << "(K3bIsoImager) Parsing mkisofs -print-size failed: " << m_collectedMkisofsPrintSizeStdout << endl;
    emit infoMessage( i18n("Could not determine size of resulting image file."), ERROR );
    emit sizeCalculated( ERROR, 0 );
  }
}



void K3bIsoImager::start()
{
  emit started();

  cleanup();

  m_containsFilesWithMultibleBackslashes = false;

  m_process = new K3bProcess();

  if( !prepareMkisofsFiles() || 
      !addMkisofsParameters() ) {
    cleanup();
    emit finished( false );
    return;
  }

  connect( m_process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotProcessExited(KProcess*)) );

  connect( m_process, SIGNAL(stderrLine( const QString& )),
	   this, SLOT(slotReceivedStderr( const QString& )) );

  if( m_fdToWriteTo == -1 )
    connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	     this, SLOT(slotReceivedStdout(KProcess*, char*, int)) );
  else
    m_process->dupStdout( m_fdToWriteTo );


  kdDebug() << "***** mkisofs parameters:\n";
  const QValueList<QCString>& args = m_process->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    s += *it + " ";
  }
  kdDebug() << s << endl << flush;
  emit debuggingOutput("mkisofs comand:", s);

  if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput) ) {
    // something went wrong when starting the program
    // it "should" be the executable
    kdDebug() << "(K3bIsoImager) could not start mkisofs" << endl;
    emit infoMessage( i18n("Could not start mkisofs."), K3bJob::ERROR );
    emit finished( false );
    cleanup();
  }
  else {
    m_processExited = false;
    m_processSuspended = false;
    m_canceled = false;
  }
}


void K3bIsoImager::cancel()
{
  m_canceled = true;

  if( m_process )
    if( !m_processExited ) {
      disconnect(m_process);
      m_process->kill();
    }

  if( !m_processExited || m_data.count() > 0 ) {
    emit canceled();
    emit finished(false);
  }
}


void K3bIsoImager::setMultiSessionInfo( const QString& info, K3bDevice* dev )
{
  m_multiSessionInfo = info;
  m_device = dev;
}


bool K3bIsoImager::addMkisofsParameters()
{
  if( !k3bcore->externalBinManager()->foundBin( "mkisofs" ) ) {
    kdDebug() << "(K3bIsoImager) could not find mkisofs executable" << endl;
    emit infoMessage( i18n("Mkisofs executable not found."), K3bJob::ERROR );
    return false;
  }

  *m_process << k3bcore->externalBinManager()->binPath( "mkisofs" );

  // add multisession info
  if( !m_multiSessionInfo.isEmpty() ) {
    *m_process << "-C" << m_multiSessionInfo;
    if( m_device )
      *m_process << "-M" << m_device->busTargetLun();
  }

  // add the arguments
  *m_process << "-gui";
  *m_process << "-graft-points";

  if( !m_doc->isoOptions().volumeID().isEmpty() ) {
    QString s = m_doc->isoOptions().volumeID();
    s.truncate(32);  // ensure max length
    *m_process << "-V" << s;
  }
  else {
    emit infoMessage( i18n("No volume id specified. Using default."), INFO );
    *m_process << "-V" << "CDROM";
  }

  QString s = m_doc->isoOptions().volumeSetId();
  s.truncate(128);  // ensure max length
  *m_process << "-volset" << s;
  
  s = m_doc->isoOptions().applicationID();
  s.truncate(128);  // ensure max length
  *m_process << "-A" << s;
  
  s = m_doc->isoOptions().publisher();
  s.truncate(128);  // ensure max length
  *m_process << "-P" << s;
  
  s = m_doc->isoOptions().preparer();
  s.truncate(128);  // ensure max length
  *m_process << "-p" << s;
  
  s = m_doc->isoOptions().systemId();
  s.truncate(32);  // ensure max length
  *m_process << "-sysid" << s;
  
  *m_process << "-volset-size" << QString::number(m_doc->isoOptions().volumeSetSize());
  *m_process << "-volset-seqno" << QString::number(m_doc->isoOptions().volumeSetNumber());
  
  if( m_doc->isoOptions().createRockRidge() ) {
    if( m_doc->isoOptions().preserveFilePermissions() )
      *m_process << "-R";
    else
      *m_process << "-r";
    if( m_rrHideFile )
      *m_process << "-hide-list" << m_rrHideFile->name();
  }

  if( m_doc->isoOptions().createJoliet() ) {
    *m_process << "-J";
    if( m_jolietHideFile )
      *m_process << "-hide-joliet-list" << m_jolietHideFile->name();
  }

  if( m_doc->isoOptions().createUdf() )
    *m_process << "-udf";

  if( m_doc->isoOptions().ISOuntranslatedFilenames()  ) {
    *m_process << "-U";
  }
  else {
    if( m_doc->isoOptions().ISOallowPeriodAtBegin()  )
      *m_process << "-L";
    if( m_doc->isoOptions().ISOallow31charFilenames()  )
      *m_process << "-l";
    if( m_doc->isoOptions().ISOomitVersionNumbers() && !m_doc->isoOptions().ISOmaxFilenameLength() )
      *m_process << "-N";
    if( m_doc->isoOptions().ISOrelaxedFilenames()  )
      *m_process << "-relaxed-filenames";
    if( m_doc->isoOptions().ISOallowLowercase()  )
      *m_process << "-allow-lowercase";
    if( m_doc->isoOptions().ISOnoIsoTranslate()  )
      *m_process << "-no-iso-translate";
    if( m_doc->isoOptions().ISOallowMultiDot()  )
      *m_process << "-allow-multidot";
    if( m_doc->isoOptions().ISOomitTrailingPeriod() )
      *m_process << "-d";
  }

  if( m_doc->isoOptions().ISOmaxFilenameLength()  )
    *m_process << "-max-iso9660-filenames";

  if( m_noDeepDirectoryRelocation  )
    *m_process << "-D";

  if( m_doc->isoOptions().followSymbolicLinks() )
    *m_process << "-f";

  if( m_doc->isoOptions().createTRANS_TBL()  )
    *m_process << "-T";
  if( m_doc->isoOptions().hideTRANS_TBL()  )
    *m_process << "-hide-joliet-trans-tbl";

  *m_process << "-iso-level" << QString::number(m_doc->isoOptions().ISOLevel());

  if( m_doc->isoOptions().forceInputCharset() )
    *m_process << "-input-charset" << m_doc->isoOptions().inputCharset();

  *m_process << "-path-list" << QFile::encodeName(m_pathSpecFile->name());


  // boot stuff
  if( !m_doc->bootImages().isEmpty() ) {
    bool first = true;
    for( QPtrListIterator<K3bBootItem> it( m_doc->bootImages() );
	 *it; ++it ) {
      if( !first )
	*m_process << "-eltorito-alt-boot";

      K3bBootItem* bootItem = *it;

      *m_process << "-b";
      if( m_doc->isoOptions().createJoliet() )
	*m_process << m_doc->treatWhitespace( bootItem->jolietPath() );
      else
	*m_process << m_doc->treatWhitespace( bootItem->k3bPath() );

      if( bootItem->imageType() == K3bBootItem::HARDDISK ) {
	*m_process << "-hard-disk-boot";
      }
      else if( bootItem->imageType() == K3bBootItem::NONE ) {
	*m_process << "-no-emul-boot";
	if( bootItem->loadSegment() > 0 )
	  *m_process << "-boot-load-seg" << bootItem->loadSegment() ;
	if( bootItem->loadSegment() > 0 )
	  *m_process << "-boot-load-size" << bootItem->loadSize() ;
      }

      if( bootItem->imageType() != K3bBootItem::NONE && bootItem->noBoot() )
	*m_process << "-no-boot";
      if( bootItem->bootInfoTable() )
	*m_process << "-boot-info-table";

      first = false;
    }

    *m_process << "-c" << m_doc->bootCataloge()->k3bPath();
  }


  // additional parameters from config
  const QStringList& params = k3bcore->externalBinManager()->binObject( "mkisofs" )->userParameters();
  for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
    *m_process << *it;

  return true;
}


bool K3bIsoImager::writePathSpec()
{
  if( m_pathSpecFile )
    delete m_pathSpecFile;
  m_pathSpecFile = new KTempFile();
  m_pathSpecFile->setAutoDelete(true);

  if( QTextStream* t = m_pathSpecFile->textStream() ) {
    // recursive path spec writing
    bool success = writePathSpecForDir( m_doc->root(), *t );
    
    m_pathSpecFile->close();
    return success;
  }
  else
    return false;
}


bool K3bIsoImager::writePathSpecForDir( K3bDirItem* dirItem, QTextStream& stream )
{
  if( dirItem->depth() > 7 ) {
    kdDebug() << "(K3bIsoImager) found directory depth > 7. Enabling no deep directory relocation." << endl;
    m_noDeepDirectoryRelocation = true;
  }

  // if joliet is enabled we need to cut long names since mkisofs is not able to do it
  if( m_doc->isoOptions().createJoliet() ) {
    createJolietFilenames( dirItem );
  }

  // now create the graft points
  for( QPtrListIterator<K3bDataItem> it( *dirItem->children() ); it.current(); ++it ) {
    K3bDataItem* item = it.current();
    if( 
       item->writeToCd()
       &&
       !( item->isSymLink()
	  && 
	  ( m_doc->isoOptions().discardSymlinks() 
	    ||
	    ( m_doc->isoOptions().discardBrokenSymlinks() 
	      && !item->isValid() )
	    )
	  )
       ) {
	
      // some versions of mkisofs seem to have a bug that prevents to use filenames 
      // that contain one or more backslashes
      if( item->k3bPath().contains("\\") )
	m_containsFilesWithMultibleBackslashes = true;

      if( m_doc->isoOptions().createJoliet() )
	stream << escapeGraftPoint( m_doc->treatWhitespace(item->jolietPath()) );
      else
	stream << escapeGraftPoint( m_doc->treatWhitespace(item->k3bPath()) ); 
      stream << "=";
      if( m_doc->bootImages().containsRef( dynamic_cast<K3bBootItem*>(item) ) ) { // boot-image-backup-hack

	// create temp file
	KTempFile temp;
	QString tempPath = temp.name();
	temp.unlink();

	if( !KIO::NetAccess::copy( it.current()->localPath(), tempPath ) ) {
	  emit infoMessage( i18n("Could not write to temporary file %1").arg(tempPath), ERROR );
	  return false;
	}

	m_tempFiles.append(tempPath);
	stream << escapeGraftPoint( tempPath ) << "\n";
      }
      else
	stream << escapeGraftPoint( item->localPath() ) << "\n";
    }
  }

  bool success = true;

  // recursively write graft points for all subdirs
  for( QPtrListIterator<K3bDataItem> it( *dirItem->children() ); it.current(); ++it ) {
    if( K3bDirItem* item = dynamic_cast<K3bDirItem*>(it.current()) )
      success = success && writePathSpecForDir( item, stream );
  }

  return success;
}



bool K3bIsoImager::writeRRHideFile()
{
  if( m_rrHideFile )
    delete m_rrHideFile;
  m_rrHideFile = new KTempFile();
  m_rrHideFile->setAutoDelete(true);

  if( QTextStream* t = m_rrHideFile->textStream() ) {

    K3bDataItem* item = m_doc->root();
    while( item ) {
      if( item->hideOnRockRidge() ) {
	if( !item->isDir() )  // hiding directories does not work (all dirs point to the dummy-dir)
	  *t << escapeGraftPoint( item->localPath() ) << "\n";
	//       if( item->isDir() ) {
	// 	K3bDirItem* parent = item->parent();
	// 	if( parent )
	// 	  item = parent->nextChild( item );
	// 	else
	// 	  item = 0;
	//       }
	//       else
	//	item = item->nextSibling();
      }
      //    else
      item = item->nextSibling();
    }
    
    m_rrHideFile->close();
    return true;
  }
  else
    return false;
}


bool K3bIsoImager::writeJolietHideFile()
{
  if( m_jolietHideFile )
    delete m_jolietHideFile;
  m_jolietHideFile = new KTempFile();
  m_jolietHideFile->setAutoDelete(true);

  if( QTextStream* t = m_jolietHideFile->textStream() ) {

    K3bDataItem* item = m_doc->root();
    while( item ) {
      if( item->hideOnRockRidge() ) {
	if( !item->isDir() )  // hiding directories does not work (all dirs point to the dummy-dir)
	  *t << escapeGraftPoint( item->localPath() ) << "\n";
      }
      item = item->nextSibling();
    }

    m_jolietHideFile->close();
    return true;
  }
  else
    return false;
}


QString K3bIsoImager::escapeGraftPoint( const QString& str )
{
  QString newStr( str );

  newStr.replace( "\\\\", "\\\\\\\\" );
  newStr.replace( "=", "\\=" );

  return newStr;
}


bool K3bIsoImager::prepareMkisofsFiles()
{
  // write path spec file
  // ----------------------------------------------------
  if( !writePathSpec() ) {
    emit infoMessage( i18n("Could not write temporary file"), K3bJob::ERROR );
    return false;
  }

  if( m_doc->isoOptions().createRockRidge() ) {
    if( !writeRRHideFile() ) {
      emit infoMessage( i18n("Could not write temporary file"), K3bJob::ERROR );
      return false;
    }
  }

  if( m_doc->isoOptions().createJoliet() ) {
    if( !writeJolietHideFile() ) {
      emit infoMessage( i18n("Could not write temporary file"), K3bJob::ERROR );
      return false ;
    }
  }

  return true;
}


void K3bIsoImager::createJolietFilenames( K3bDirItem* dirItem )
{
  // create new joliet names and use jolietPath for graftpoints
  // sort dirItem->children entries and rename all to fit joliet
  // which is about x characters

  kdDebug() << "(K3bIsoImager) creating joliet names for directory: " << dirItem->k3bName() << endl;

  QPtrList<K3bDataItem> sortedChildren;

  // insertion sort
  for( QPtrListIterator<K3bDataItem> it( *dirItem->children() ); it.current(); ++it ) {
    K3bDataItem* item = it.current();

    unsigned int i = 0;
    while( i < sortedChildren.count() && item->k3bName() > sortedChildren.at(i)->k3bName() )
      ++i;

    sortedChildren.insert( i, item );
  }

  unsigned int begin = 0;
  unsigned int sameNameCount = 0;
  unsigned int jolietMaxLength = 64;
  while( begin < sortedChildren.count() ) {
    if( sortedChildren.at(begin)->k3bName().length() > jolietMaxLength ) {
      kdDebug() << "(K3bIsoImager) filename to long for joliet: "
		<< sortedChildren.at(begin)->k3bName() << endl;
      sameNameCount = 1;

      while( begin + sameNameCount < sortedChildren.count() &&
	     sortedChildren.at( begin + sameNameCount )->k3bName().left(jolietMaxLength)
	     == sortedChildren.at(begin)->k3bName().left(jolietMaxLength) )
	sameNameCount++;

      kdDebug() << "(K3bIsoImager) found " << sameNameCount << " files with same joliet name" << endl;

      if( sameNameCount > 1 ) {
	kdDebug() << "(K3bIsoImager) cutting filenames." << endl;

	unsigned int charsForNumber = QString::number(sameNameCount).length();
	for( unsigned int i = begin; i < begin + sameNameCount; i++ ) {
	  // we always reserve 5 chars for the extension
	  QString extension = sortedChildren.at(i)->k3bName().right(5);
	  if( !extension.contains(".") )
	    extension = "";
	  else
	    extension = extension.mid( extension.find(".") );
	  QString jolietName = sortedChildren.at(i)->k3bName().left(jolietMaxLength-charsForNumber-extension.length()-1);
	  jolietName.append( " " );
	  jolietName.append( QString::number( i-begin ).rightJustify( charsForNumber, '0') );
	  jolietName.append( extension );
	  sortedChildren.at(i)->setJolietName( jolietName );
	    
	  kdDebug() << "(K3bIsoImager) set joliet name for "
		    << sortedChildren.at(i)->k3bName() << " to "
		    << jolietName << endl;
	}
      }
      else {
	kdDebug() << "(K3bIsoImager) single file -> no change." << endl;
	sortedChildren.at(begin)->setJolietName( sortedChildren.at(begin)->k3bName() );
      }

      begin += sameNameCount;
    }
    else {
      sortedChildren.at(begin)->setJolietName( sortedChildren.at(begin)->k3bName() );
      begin++;
    }
  }
}


#include "k3bisoimager.moc"
