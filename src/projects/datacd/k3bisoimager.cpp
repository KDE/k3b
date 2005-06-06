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

#include "k3bisoimager.h"
#include "k3bdiritem.h"
#include "k3bbootitem.h"
#include "k3bdatadoc.h"
#include <k3bexternalbinmanager.h>
#include <k3bdevice.h>
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
#include <qdir.h>

#include <errno.h>
#include <string.h>
#include <cmath>


class K3bIsoImager::Private
{
public:
  QString imagePath;
  QFile imageFile;
};


K3bIsoImager::K3bIsoImager( K3bDataDoc* doc, QObject* parent, const char* name )
  : K3bJob( parent, name ),
    m_pathSpecFile(0),
    m_rrHideFile(0),
    m_jolietHideFile(0),
    m_sortWeightFile(0),
    m_process( 0 ),
    m_processSuspended(false),
    m_processExited(false),
    m_doc( doc ),
    m_noDeepDirectoryRelocation( false ),
    m_importSession( false ),
    m_device(0),
    m_mkisofsPrintSizeResult( 0 ),
    m_fdToWriteTo(-1)
{
  d = new Private;
}


K3bIsoImager::~K3bIsoImager()
{
  delete d;
  cleanup();
}


bool K3bIsoImager::active() const
{
  return !m_processExited;
}


void K3bIsoImager::writeToFd( int fd )
{
  m_fdToWriteTo = fd;
}


void K3bIsoImager::writeToImageFile( const QString& path )
{
  d->imagePath = path;
  m_fdToWriteTo = -1;
}


void K3bIsoImager::slotReceivedStderr( const QString& line )
{
  if( !line.isEmpty() ) {
    emit debuggingOutput( "mkisofs", line );

    if( line.contains( "done, estimate" ) ) {

//       if( m_device )
// 	m_device->close();  // release the device for cdrecord

      int p = parseProgress( line );
      if( p != -1 )
	emit percent( p );
    }
    else if( line.contains( "extents written" ) ) {
      emit percent( 100 );
    }
    else {
      kdDebug() << "(mkisofs) " << line << endl;
    }
  }
}


int K3bIsoImager::parseProgress( const QString& line ) 
{
  //
  // in multisession mode mkisofs' progress does not start at 0 but at (X+Y)/X
  // where X is the data already on the cd and Y the data to create
  // This is not very dramatic but kind or ugly.
  // We just save the first emitted progress value and to some math ;)
  //

  QString perStr = line;
  perStr.truncate( perStr.find('%') );
  bool ok;
  double p = perStr.toDouble( &ok );
  if( !ok ) {
    kdDebug() << "(K3bIsoImager) Parsing did not work for " << perStr << endl;
    return -1;
  }
  else {
    if( m_firstProgressValue < 0 )
      m_firstProgressValue = p;
    
    return( (int)::ceil( (p - m_firstProgressValue)*100.0/(100.0 - m_firstProgressValue) ) );
  }
}


void K3bIsoImager::slotProcessExited( KProcess* p )
{
  m_processExited = true;

  if( d->imageFile.isOpen() )
    d->imageFile.close();

//   if( m_device )
//     m_device->close();

  if( !m_canceled ) {

    if( p->normalExit() ) {
      if( p->exitStatus() == 0 ) {
	emit finished( true );
      }
      else  {
	switch( p->exitStatus() ) {
	case 104:
	  // connection reset by peer
	  // This only happens if cdrecord does not finish successfully
	  // so we may leave the error handling to it meaning we handle this
	  // as a known error
	  break;

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
      emit infoMessage( i18n("%1 did not exit cleanly.").arg("mkisofs"), ERROR );
      emit finished( false );
    }
  }

  cleanup();
}


void K3bIsoImager::cleanup()
{
  // remove all temp files
  delete m_pathSpecFile;
  delete m_rrHideFile;
  delete m_jolietHideFile;
  delete m_sortWeightFile;

  // remove boot-images-temp files
  for( QStringList::iterator it = m_tempFiles.begin();
       it != m_tempFiles.end(); ++it )
    QFile::remove( *it );
  m_tempFiles.clear();

  m_pathSpecFile = m_jolietHideFile = m_rrHideFile = m_sortWeightFile = 0;

  delete m_process;
  m_process = 0;
}


void K3bIsoImager::calculateSize()
{
  // determine iso-size
  cleanup();

  m_process = new K3bProcess();
  m_process->setRunPrivileged(true);

  const K3bExternalBin* mkisofsBin = k3bcore->externalBinManager()->binObject( "mkisofs" );
  if( !mkisofsBin ) {
    kdDebug() << "(K3bIsoImager) could not find mkisofs executable" << endl;
    emit infoMessage( i18n("Mkisofs executable not found."), K3bJob::ERROR );
    cleanup();
    emit sizeCalculated( ERROR, 0 );
    return;
  }

  if( !mkisofsBin->copyright.isEmpty() )
    emit infoMessage( i18n("Using %1 %2 - Copyright (C) %3").arg("mkisofs").arg(mkisofsBin->version).arg(mkisofsBin->copyright), INFO );

  *m_process << mkisofsBin;

  // prepare the filenames as written to the image
  m_doc->prepareFilenames();

  if( !prepareMkisofsFiles() || 
      !addMkisofsParameters( true ) ) {
    cleanup();
    emit sizeCalculated( ERROR, 0 );
    return;
  }

  // add empty dummy dir since one path-spec is needed
  *m_process << dummyDir();

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
    emit infoMessage( i18n("Could not start %1.").arg("mkisofs"), K3bJob::ERROR );
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
    emit sizeCalculated( INFO, m_mkisofsPrintSizeResult );
  }
  else {
    m_mkisofsPrintSizeResult = 0;
    kdDebug() << "(K3bIsoImager) Parsing mkisofs -print-size failed: " << m_collectedMkisofsPrintSizeStdout << endl;
    emit infoMessage( i18n("Could not determine size of resulting image file."), ERROR );
    emit sizeCalculated( ERROR, 0 );
  }
}


void K3bIsoImager::init()
{
  m_containsFilesWithMultibleBackslashes = false;
  m_firstProgressValue = -1;
  m_processExited = false;
  m_processSuspended = false;
  m_canceled = false;
}


void K3bIsoImager::start()
{
  emit started();

  cleanup();
  init();

  m_process = new K3bProcess();
  m_process->setRunPrivileged(true);

  const K3bExternalBin* mkisofsBin = k3bcore->externalBinManager()->binObject( "mkisofs" );
  if( !mkisofsBin ) {
    kdDebug() << "(K3bIsoImager) could not find mkisofs executable" << endl;
    emit infoMessage( i18n("Could not find %1 executable.").arg("mkisofs"), K3bJob::ERROR );
    cleanup();
    emit finished( false );
    return;
  }

  if( !mkisofsBin->copyright.isEmpty() )
    emit infoMessage( i18n("Using %1 %2 - Copyright (C) %3").arg("mkisofs").arg(mkisofsBin->version).arg(mkisofsBin->copyright), INFO );


  *m_process << mkisofsBin;

  // prepare the filenames as written to the image
  m_doc->prepareFilenames();

  if( !prepareMkisofsFiles() ||
      !addMkisofsParameters() ) {
    cleanup();
    emit finished( false );
    return;
  }

  // add empty dummy dir since one path-spec is needed
  *m_process << dummyDir();

  connect( m_process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotProcessExited(KProcess*)) );

  connect( m_process, SIGNAL(stderrLine( const QString& )),
	   this, SLOT(slotReceivedStderr( const QString& )) );

  if( m_fdToWriteTo != -1 )
    m_process->writeToFd( m_fdToWriteTo );
  else {
    d->imageFile.setName( d->imagePath );
    if( d->imageFile.open( IO_WriteOnly ) )
      m_process->writeToFd( d->imageFile.handle() );
    else {
      emit infoMessage( i18n("Could not open %1 for writing").arg(d->imagePath), ERROR );
      cleanup();
      emit finished(false);
      return;
    }
  }


  kdDebug() << "***** mkisofs parameters:\n";
  const QValueList<QCString>& args = m_process->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    s += *it + " ";
  }
  kdDebug() << s << endl << flush;
  emit debuggingOutput("mkisofs comand:", s);

  if( m_doc->needToCutFilenames() )
    emit infoMessage( i18n("Some filenames need to be shortened due to the %1 char restriction "
			   "of the Joliet extensions.")
		      .arg( m_doc->isoOptions().jolietLong() ? 103 : 64 ), 
		      WARNING );

  if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput) ) {
    // something went wrong when starting the program
    // it "should" be the executable
    kdDebug() << "(K3bIsoImager) could not start mkisofs" << endl;
    emit infoMessage( i18n("Could not start %1.").arg("mkisofs"), K3bJob::ERROR );
    emit finished( false );
    cleanup();
  }
}


void K3bIsoImager::cancel()
{
  m_canceled = true;

  if( m_process )
    if( !m_processExited ) {
      disconnect(m_process);
      m_process->kill();
//       if( m_device )
// 	m_device->close();
    }

  if( !m_processExited ) {
    emit canceled();
    emit finished(false);
  }
}


void K3bIsoImager::setMultiSessionInfo( const QString& info, K3bDevice* dev )
{
  m_multiSessionInfo = info;
  m_device = dev;
}


bool K3bIsoImager::addMkisofsParameters( bool printSize )
{
  // add multisession info
  if( !m_multiSessionInfo.isEmpty() ) {
    *m_process << "-cdrecord-params" << m_multiSessionInfo;
    if( m_device ) {
      *m_process << "-prev-session" << m_device->blockDeviceName();
    }
  }

  // add the arguments
  *m_process << "-gui";
  *m_process << "-graft-points";

  if( printSize )
    *m_process << "-print-size" << "-quiet";

  if( !m_doc->isoOptions().volumeID().isEmpty() ) {
    QString s = m_doc->isoOptions().volumeID();
    s.truncate(32);  // ensure max length
    *m_process << "-volid" << s;
  }
  else {
    emit infoMessage( i18n("No volume id specified. Using default."), WARNING );
    *m_process << "-volid" << "CDROM";
  }

  QString s = m_doc->isoOptions().volumeSetId();
  s.truncate(128);  // ensure max length
  *m_process << "-volset" << s;

  s = m_doc->isoOptions().applicationID();
  s.truncate(128);  // ensure max length
  *m_process << "-appid" << s;

  s = m_doc->isoOptions().publisher();
  s.truncate(128);  // ensure max length
  *m_process << "-publisher" << s;

  s = m_doc->isoOptions().preparer();
  s.truncate(128);  // ensure max length
  *m_process << "-preparer" << s;

  s = m_doc->isoOptions().systemId();
  s.truncate(32);  // ensure max length
  *m_process << "-sysid" << s;

  *m_process << "-volset-size" << QString::number(m_doc->isoOptions().volumeSetSize());
  *m_process << "-volset-seqno" << QString::number(m_doc->isoOptions().volumeSetNumber());

  if( m_sortWeightFile ) {
    *m_process << "-sort" << m_sortWeightFile->name();
  }

  if( m_doc->isoOptions().createRockRidge() ) {
    if( m_doc->isoOptions().preserveFilePermissions() )
      *m_process << "-rock";
    else
      *m_process << "-rational-rock";
    if( m_rrHideFile )
      *m_process << "-hide-list" << m_rrHideFile->name();
  }

  if( m_doc->isoOptions().createJoliet() ) {
    *m_process << "-joliet";
    if( m_doc->isoOptions().jolietLong() )
      *m_process << "-joliet-long";
    if( m_jolietHideFile )
      *m_process << "-hide-joliet-list" << m_jolietHideFile->name();
  }

  if( m_doc->isoOptions().createUdf() )
    *m_process << "-udf";

  if( m_doc->isoOptions().ISOuntranslatedFilenames()  ) {
    *m_process << "-untranslated-filenames";
  }
  else {
    if( m_doc->isoOptions().ISOallowPeriodAtBegin()  )
      *m_process << "-allow-leading-dots";
    if( m_doc->isoOptions().ISOallow31charFilenames()  )
      *m_process << "-full-iso9660-filenames";
    if( m_doc->isoOptions().ISOomitVersionNumbers() && !m_doc->isoOptions().ISOmaxFilenameLength() )
      *m_process << "-omit-version-number";
    if( m_doc->isoOptions().ISOrelaxedFilenames()  )
      *m_process << "-relaxed-filenames";
    if( m_doc->isoOptions().ISOallowLowercase()  )
      *m_process << "-allow-lowercase";
    if( m_doc->isoOptions().ISOnoIsoTranslate()  )
      *m_process << "-no-iso-translate";
    if( m_doc->isoOptions().ISOallowMultiDot()  )
      *m_process << "-allow-multidot";
    if( m_doc->isoOptions().ISOomitTrailingPeriod() )
      *m_process << "-omit-period";
  }

  if( m_doc->isoOptions().ISOmaxFilenameLength()  )
    *m_process << "-max-iso9660-filenames";

  if( m_noDeepDirectoryRelocation  )
    *m_process << "-disable-deep-relocation";

  if( m_doc->isoOptions().followSymbolicLinks() )
    *m_process << "-follow-links";

  if( m_doc->isoOptions().createTRANS_TBL()  )
    *m_process << "-translation-table";
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

      *m_process << "-eltorito-boot";
      *m_process << bootItem->writtenPath();

      if( bootItem->imageType() == K3bBootItem::HARDDISK ) {
	*m_process << "-hard-disk-boot";
      }
      else if( bootItem->imageType() == K3bBootItem::NONE ) {
	*m_process << "-no-emul-boot";
	if( bootItem->loadSegment() > 0 )
	  *m_process << "-boot-load-seg" << QString::number(bootItem->loadSegment());
	if( bootItem->loadSize() > 0 )
	  *m_process << "-boot-load-size" << QString::number(bootItem->loadSize());
      }

      if( bootItem->imageType() != K3bBootItem::NONE && bootItem->noBoot() )
	*m_process << "-no-boot";
      if( bootItem->bootInfoTable() )
	*m_process << "-boot-info-table";

      first = false;
    }

    *m_process << "-eltorito-catalog" << m_doc->bootCataloge()->writtenPath();
  }


  // additional parameters from config
  const QStringList& params = k3bcore->externalBinManager()->binObject( "mkisofs" )->userParameters();
  for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
    *m_process << *it;

  return true;
}


int K3bIsoImager::writePathSpec()
{
  delete m_pathSpecFile;
  m_pathSpecFile = new KTempFile();
  m_pathSpecFile->setAutoDelete(true);

  if( QTextStream* t = m_pathSpecFile->textStream() ) {
    // recursive path spec writing
    int num = writePathSpecForDir( m_doc->root(), *t );

    m_pathSpecFile->close();
    return num;
  }
  else
    return -1;
}


int K3bIsoImager::writePathSpecForDir( K3bDirItem* dirItem, QTextStream& stream )
{
  if( dirItem->depth() > 7 ) {
    kdDebug() << "(K3bIsoImager) found directory depth > 7. Enabling no deep directory relocation." << endl;
    m_noDeepDirectoryRelocation = true;
  }

  // now create the graft points
  int num = 0;
  for( QPtrListIterator<K3bDataItem> it( dirItem->children() ); it.current(); ++it ) {
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

      QFileInfo f( item->localPath() );
      // QFile::exists returns false for broken symlinks which is not what we want
      if( item->isDir() || f.isSymLink() || f.isFile() ) {
	num++;

	// some versions of mkisofs seem to have a bug that prevents to use filenames
	// that contain one or more backslashes
	if( item->writtenPath().contains("\\") )
	  m_containsFilesWithMultibleBackslashes = true;
	
	stream << escapeGraftPoint( item->writtenPath() )
	       << "=";

	if( m_doc->bootImages().containsRef( dynamic_cast<K3bBootItem*>(item) ) ) { // boot-image-backup-hack
	  
	  // create temp file
	  KTempFile temp;
	  QString tempPath = temp.name();
	  temp.unlink();
	  
	  if( !KIO::NetAccess::copy( item->localPath(), tempPath ) ) {
	    emit infoMessage( i18n("Could not write to temporary file %1").arg(tempPath), ERROR );
	    return -1;
	  }
	  
	  static_cast<K3bBootItem*>(item)->setTempPath( tempPath );

	  m_tempFiles.append(tempPath);
	  stream << escapeGraftPoint( tempPath ) << endl;
	}
	else if( item->isDir() )
	  stream << dummyDir( item->sortWeight() ) << endl;
	else
	  stream << escapeGraftPoint( item->localPath() ) << endl;
      }
      else
	emit infoMessage( i18n("Could not find %1. Skipping...").arg(item->localPath()), WARNING );
    }

    // recursively write graft points for all subdirs
    if( item->isDir() ) {
      int x = writePathSpecForDir( dynamic_cast<K3bDirItem*>(item), stream );
      if( x >= 0 )
	num += x;
      else
	return -1;
    }
  }

  return num;
}



bool K3bIsoImager::writeRRHideFile()
{
  delete m_rrHideFile;
  m_rrHideFile = new KTempFile();
  m_rrHideFile->setAutoDelete(true);

  if( QTextStream* t = m_rrHideFile->textStream() ) {

    K3bDataItem* item = m_doc->root();
    while( item ) {
      if( item->hideOnRockRidge() ) {
	if( !item->isDir() )  // hiding directories does not work (all dirs point to the dummy-dir)
	  *t << escapeGraftPoint( item->localPath() ) << endl;
      }
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
  delete m_jolietHideFile;
  m_jolietHideFile = new KTempFile();
  m_jolietHideFile->setAutoDelete(true);

  if( QTextStream* t = m_jolietHideFile->textStream() ) {

    K3bDataItem* item = m_doc->root();
    while( item ) {
      if( item->hideOnRockRidge() ) {
	if( !item->isDir() )  // hiding directories does not work (all dirs point to the dummy-dir but we could introduce a second hidden dummy dir)
	  *t << escapeGraftPoint( item->localPath() ) << endl;
      }
      item = item->nextSibling();
    }

    m_jolietHideFile->close();
    return true;
  }
  else
    return false;
}


bool K3bIsoImager::writeSortWeightFile()
{
  delete m_sortWeightFile;
  m_sortWeightFile = new KTempFile();
  m_sortWeightFile->setAutoDelete(true);

  if( QTextStream* t = m_sortWeightFile->textStream() ) {
    //
    // We need to write the local path in combination with the sort weight
    // mkisofs will take care of multible entries for one local file and always
    // use the highest weight
    //
    K3bDataItem* item = m_doc->root();
    while( (item = item->nextSibling()) ) {  // we skip the root here
      if( item->sortWeight() != 0 ) {
	if( m_doc->bootImages().containsRef( dynamic_cast<K3bBootItem*>(item) ) ) { // boot-image-backup-hack
	  *t << escapeGraftPoint( static_cast<K3bBootItem*>(item)->tempPath() ) << " " << item->sortWeight() << endl;
	}
	else if( item->isDir() ) {
	  // 
	  // Since we use dummy dirs for all directories in the filesystem and mkisofs uses the local path 
	  // for sorting we need to create a different dummy dir for every sort weight value.
	  //
	  *t << escapeGraftPoint( dummyDir( item->sortWeight() ) ) << " " << item->sortWeight() << endl;
	}
	else
	  *t << escapeGraftPoint( item->localPath() ) << " " << item->sortWeight() << endl;
      }
    }

    m_sortWeightFile->close();
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
  //
  // in case the doc contains files > 2gb we need to enable udf!
  // since iso9660 does only support file sizes up to 2 gb
  //
  if( !m_doc->isoOptions().createUdf() ) {
    K3bDataItem* item = m_doc->root();
    while( (item = item->nextSibling()) ) {
      if( item->isFile() && item->k3bSize() > (KIO::filesize_t)(2*1024*1024*1024) ) {
	emit infoMessage( i18n("Enabled UDF extensions to support files bigger than 2 GB."), WARNING );
	m_doc->isoOptions().setCreateUdf( true );
	break;
      }
    }
  }


  // write path spec file
  // ----------------------------------------------------
  int num = writePathSpec();
  if( num < 0 ) {
    emit infoMessage( i18n("Could not write temporary file"), K3bJob::ERROR );
    return false;
  }
  else if( num == 0 ) {
    emit infoMessage( i18n("No files to be written."), K3bJob::ERROR );
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

  if( !writeSortWeightFile() ) {
    emit infoMessage( i18n("Could not write temporary file"), K3bJob::ERROR );
    return false;
  }

  return true;
}


QString K3bIsoImager::dummyDir( int weight )
{
  QDir _appDir( locateLocal( "appdata", "temp/" ) );
  if( !_appDir.cd( QString("dummydir%1").arg(weight) ) ) {
    _appDir.mkdir( QString("dummydir%1").arg(weight) );
    _appDir.cd( QString("dummydir%1").arg(weight) );
  }

  return _appDir.absPath() + "/";
}

#include "k3bisoimager.moc"
