#include "k3bisoimager.h"

#include <kdebug.h>
#include <kstandarddirs.h>
#include <klocale.h>

#include <qfile.h>
#include <qregexp.h>
#include <qtimer.h>

#include "k3bdatadoc.h"
#include "../tools/k3bexternalbinmanager.h"
#include "../device/k3bdevice.h"
#include "k3bdiritem.h"
#include "../k3bprocess.h"



K3bIsoImager::K3bIsoImager( K3bExternalBinManager* exbm, K3bDataDoc* doc, QObject* parent, const char* name )
  : K3bJob( parent, name ),
    m_externalBinManager( exbm ),
    m_doc( doc ),
    m_noDeepDirectoryRelocation( false ),
    m_importSession( false ),
    m_process( 0 ),
    m_mkisofsPrintSizeResult( 0 )
{
}


K3bIsoImager::~K3bIsoImager()
{
  cleanup();
}


void K3bIsoImager::slotReceivedStdout( KProcess*, char* d, int len )
{
  m_process->suspend();
  emit data( d, len );
}


void K3bIsoImager::resume()
{
  if( m_process )
    if( m_process->isRunning() )
      m_process->resume();
}


void K3bIsoImager::slotReceivedStderr( const QString& line )
{
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


void K3bIsoImager::slotProcessExited( KProcess* p )
{
  if( p->normalExit() ) {
    if( p->exitStatus() == 0 ) {
      emit infoMessage( i18n("mkisofs finished successfully."), STATUS );
      emit finished( true );
    }
    else {
      emit infoMessage( i18n("mkisofs returned error: %1").arg(p->exitStatus()), ERROR );
      emit finished( false );
    }
  }
  else {
    emit infoMessage( i18n("mkisofs exited abnormally."), ERROR );
    emit finished( false );
  }

  cleanup();
}


void K3bIsoImager::cleanup()
{
  // remove all temp files
  if( QFile::exists( m_pathSpecFile ) )
    QFile::remove( m_pathSpecFile );
  if( QFile::exists( m_rrHideFile ) )
    QFile::remove( m_rrHideFile );
  if( QFile::exists( m_jolietHideFile ) )
    QFile::remove( m_jolietHideFile );
  m_pathSpecFile = m_rrHideFile = m_jolietHideFile = QString::null;

  delete m_process;
  m_process = 0;
}


void K3bIsoImager::calculateSize()
{
  if( !prepareMkisofsFiles() ) {
    cleanup();
    emit sizeCalculated( ERROR, 0 );
    return;
  }


  // determine iso-size
  delete m_process;
  m_process = new K3bProcess();

  if( !addMkisofsParameters() ) {
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
  m_collectedMkisofsPrintSizeStderr.append( QString::fromLocal8Bit( data, len ) );
}


void K3bIsoImager::slotCollectMkisofsPrintSizeStdout(KProcess*, char* data, int len )
{
  m_collectedMkisofsPrintSizeStdout.append( QString::fromLocal8Bit( data, len ) );
}


void K3bIsoImager::slotMkisofsPrintSizeFinished()
{
  bool success = true;

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

  if( !prepareMkisofsFiles() ) {
    cleanup();
    emit finished( false );
    return;
  }

  delete m_process;
  m_process = new K3bProcess();

  if( !addMkisofsParameters() ) {
    cleanup();

    emit finished( false );
    return;
  }

  connect( m_process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotProcessExited(KProcess*)) );

//   connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
// 	   this, SLOT(slotReceivedStderr(KProcess*, char*, int)) );

  connect( m_process, SIGNAL(stderrLine( const QString& )),
	   this, SLOT(slotReceivedStderr( const QString& )) );

  connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotReceivedStdout(KProcess*, char*, int)) );

  if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput) )
    {
      // something went wrong when starting the program
      // it "should" be the executable
      kdDebug() << "(K3bIsoImager) could not start mkisofs" << endl;
      emit infoMessage( i18n("Could not start mkisofs."), K3bJob::ERROR );
      emit finished( false );
      cleanup();
    }
}


void K3bIsoImager::cancel()
{
  m_process->kill();
}


void K3bIsoImager::setMultiSessionInfo( const QString& info )
{
  m_multiSessionInfo = info;
  m_importSession = true;
}


bool K3bIsoImager::addMkisofsParameters()
{
  if( !m_externalBinManager->foundBin( "mkisofs" ) ) {
    kdDebug() << "(K3bIsoImager) could not find mkisofs executable" << endl;
    emit infoMessage( i18n("Mkisofs executable not found."), K3bJob::ERROR );
    return false;
  }

  *m_process << m_externalBinManager->binPath( "mkisofs" );

  // add multisession info
  if( m_importSession ) {

    // it has to be the device we are writing to cause only this makes sense
    *m_process << "-M" << m_doc->burner()->busTargetLun();
    *m_process << "-C" << m_multiSessionInfo;
  }

  // add the arguments
  *m_process << "-gui";
  *m_process << "-graft-points";

  if( !m_doc->isoOptions().volumeID().isEmpty() ) {
    QString s = m_doc->isoOptions().volumeID();
    *m_process << "-V" << s.replace( QRegExp("\\s"), "_" );
  }
  if( !m_doc->isoOptions().volumeSetId().isEmpty() ) {
    QString s = m_doc->isoOptions().volumeSetId();
    *m_process << "-volset" << s.replace( QRegExp("\\s"), "_" );;
  }
  if( !m_doc->isoOptions().applicationID().isEmpty() ) {
    QString s = m_doc->isoOptions().applicationID();
    *m_process << "-A" << s.replace( QRegExp("\\s"), "_" );;
  }
  if( !m_doc->isoOptions().publisher().isEmpty() ) {
    QString s = m_doc->isoOptions().publisher();
    *m_process << "-P" << s.replace( QRegExp("\\s"), "_" );;
  }
  if( !m_doc->isoOptions().preparer().isEmpty() ) {
    QString s = m_doc->isoOptions().preparer();
    *m_process << "-p" << s.replace( QRegExp("\\s"), "_" );;
  }
  if( !m_doc->isoOptions().systemId().isEmpty() ) {
    QString s = m_doc->isoOptions().systemId();
    *m_process << "-sysid" << s.replace( QRegExp("\\s"), "_" );;
  }

  if( m_doc->isoOptions().createRockRidge() ) {
    if( m_doc->isoOptions().preserveFilePermissions() )
      *m_process << "-R";
    else
      *m_process << "-r";
    if( QFile::exists( m_rrHideFile ) )
      *m_process << "-hide-list" << m_rrHideFile;
  }

  if( m_doc->isoOptions().createJoliet() ) {
    *m_process << "-J";
    if( QFile::exists( m_jolietHideFile ) )
      *m_process << "-hide-joliet-list" << m_jolietHideFile;
  }

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

  *m_process << "-path-list" << QFile::encodeName(m_pathSpecFile);


  // additional parameters from config
  const QStringList& params = m_externalBinManager->binObject( "mkisofs" )->userParameters();
  for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
    *m_process << *it;

  return true;
}


bool K3bIsoImager::writePathSpec( const QString& filename )
{
  QFile file( filename );
  if( !file.open( IO_WriteOnly ) ) {
    return false;
  }

  QTextStream t(&file);

  // recursive path spec writing
  writePathSpecForDir( m_doc->root(), t );

  file.close();

  return true;
}


void K3bIsoImager::writePathSpecForDir( K3bDirItem* dirItem, QTextStream& stream )
{
  if( dirItem->depth() > 7 ) {
    kdDebug() << "(K3bIsoImager) found directory depth > 7. Enabling no deep directory relocation." << endl;
    m_noDeepDirectoryRelocation = true;
  }

  // if joliet is enabled we need to cut long names since mkisofs is not able to do it

  if( m_doc->isoOptions().createJoliet() ) {
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

	kdDebug() << "K3bIsoImager) found " << sameNameCount << " files with same joliet name" << endl;

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

	begin += sameNameCount;
      }
      else {
	sortedChildren.at(begin)->setJolietName( sortedChildren.at(begin)->k3bName() );
	begin++;
      }
    }

    // now create the graft points
    for( QPtrListIterator<K3bDataItem> it( *dirItem->children() ); it.current(); ++it ) {
      K3bDataItem* item = it.current();
      if( m_doc->isoOptions().discardSymlinks() && item->isSymLink() )
	continue;

      stream << escapeGraftPoint( m_doc->treatWhitespace(item->jolietPath()) )
	     << "=" << escapeGraftPoint( item->localPath() ) << "\n";
    }
  }
  else {
    // use k3bPath as normal for graftpoints
    // if rr is enabled all will be cool
    // if neither rr nor joliet are enabled we get very awful names but mkisofs
    // takes care of it
    for( QPtrListIterator<K3bDataItem> it( *dirItem->children() ); it.current(); ++it ) {
      K3bDataItem* item = it.current();
      if( m_doc->isoOptions().discardSymlinks() && item->isSymLink() )
	continue;

      stream << escapeGraftPoint( m_doc->treatWhitespace(item->k3bPath()) )
	     << "=" << escapeGraftPoint( item->localPath() ) << "\n";
    }
  }

  // recursively write graft points for all subdirs
  for( QPtrListIterator<K3bDataItem> it( *dirItem->children() ); it.current(); ++it ) {
    if( K3bDirItem* item = dynamic_cast<K3bDirItem*>(it.current()) )
      writePathSpecForDir( item, stream );
  }
}



bool K3bIsoImager::writeRRHideFile( const QString& filename )
{
  QFile file( filename );
  if( !file.open( IO_WriteOnly ) )
    return false;

  QTextStream stream( &file );

  K3bDataItem* item = m_doc->root();
  while( item ) {
    if( item->hideOnRockRidge() ) {
      if( !item->isDir() )  // hiding directories does not work (all dirs point to the dummy-dir)
	stream << escapeGraftPoint( item->localPath() ) << "\n";
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

  file.close();
  return true;
}


bool K3bIsoImager::writeJolietHideFile( const QString& filename )
{
  QFile file( filename );
  if( !file.open( IO_WriteOnly ) )
    return false;

  QTextStream stream( &file );

  K3bDataItem* item = m_doc->root();
  while( item ) {
    if( item->hideOnRockRidge() ) {
      if( !item->isDir() )  // hiding directories does not work (all dirs point to the dummy-dir)
	stream << escapeGraftPoint( item->localPath() ) << "\n";
    }
    item = item->nextSibling();
  }

  file.close();
  return true;
}


QString K3bIsoImager::escapeGraftPoint( const QString& str )
{
  QString newStr( str );

  newStr.replace( QRegExp( "\\\\\\\\" ), "\\\\\\\\\\\\\\\\" );
  newStr.replace( QRegExp( "=" ), "\\=" );

  return newStr;
}


bool K3bIsoImager::prepareMkisofsFiles()
{
  // write path spec file
  // ----------------------------------------------------
  m_pathSpecFile = locateLocal( "appdata", "temp/path_spec.mkisofs" );
  if( !writePathSpec( m_pathSpecFile ) ) {
    emit infoMessage( i18n("Could not write to temporary file %1").arg( m_pathSpecFile ), K3bJob::ERROR );
    return false;
  }

  if( m_doc->isoOptions().createRockRidge() ) {
    m_rrHideFile = locateLocal( "appdata", "temp/rr_hide.mkisofs" );
    if( !writeRRHideFile( m_rrHideFile ) ) {
      emit infoMessage( i18n("Could not write to temporary file %1").arg( m_rrHideFile ), K3bJob::ERROR );
      return false;
    }
  }

  if( m_doc->isoOptions().createJoliet() ) {
    m_jolietHideFile = locateLocal( "appdata", "temp/joliet_hide.mkisofs" );
    if( !writeJolietHideFile( m_jolietHideFile ) ) {
      emit infoMessage( i18n("Could not write to temporary file %1").arg( m_rrHideFile ), K3bJob::ERROR );
      return false ;
    }
  }

  return true;
}


#include "k3bisoimager.moc"
