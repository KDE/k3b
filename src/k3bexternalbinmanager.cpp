#include "k3bexternalbinmanager.h"

#include <kprocess.h>
#include <kconfig.h>

#include <qstring.h>
#include <qregexp.h>
#include <qfile.h>





K3bExternalBin::K3bExternalBin( const QString& name )
 : m_name( name )
{
}


bool K3bExternalBin::isEmpty() const 
{
  return m_name.isEmpty();
}


const QString& K3bExternalBin::name() const
{
  return m_name;
}


K3bExternalBinManager::K3bExternalBinManager( QObject* parent )
  : QObject( parent )
{
  m_process = new KProcess();

  m_binMap.insert("mkisofs", new K3bExternalBin( "mkisofs" ));
  m_binMap.insert("cdrecord", new K3bExternalBin( "cdrecord" ));
  m_binMap.insert("cdrdao", new K3bExternalBin( "cdrdao" ));
//   m_binMap.insert("mpg123", new K3bExternalBin( "mpg123" ));
//   m_binMap.insert("sox", new K3bExternalBin( "sox" ));
}


K3bExternalBinManager::~K3bExternalBinManager()
{ 
  delete m_process;

  QMap<QString, K3bExternalBin*>::Iterator it = m_binMap.begin();
  for( ; it != m_binMap.end(); ++it )
    delete it.data();
}


void K3bExternalBinManager::search()
{
  static const char* searchPaths[] = { "/usr/bin/", "/usr/local/bin/",
				       "/usr/sbin/", "/usr/local/sbin/",
				       "/opt/schily/bin/" };
  static const int NUM_SEARCH_PATHS = 5;



  // search for mkisofs
  for( int i = 0; i < NUM_SEARCH_PATHS; i++ ) {

    QString bin = QString("%1%2").arg(searchPaths[i]).arg("mkisofs");
    if( QFile::exists( bin ) ) {

      m_process->clearArguments();
      m_process->disconnect();
      *m_process << bin << "--version";
      connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	       this, SLOT(slotParseCdrtoolsVersion(KProcess*, char*, int)) );
      m_process->start( KProcess::Block, KProcess::Stdout );
      if( m_binMap["mkisofs"]->version.isEmpty() ) {
	qDebug("(K3bExternalBinManager) " + bin + " seems not to be mkisofs version >= 1.13.");
      }
      else {
	m_binMap["mkisofs"]->path = bin;
	break;  // bin found
      }
    }
  }
  if( m_binMap["mkisofs"]->path.isEmpty() ) {
    qDebug("(K3bExternalBinManager) Could not find mkisofs");
  }



  // search for cdrecord
  for( int i = 0; i < NUM_SEARCH_PATHS; i++ ) {

    QString bin = QString("%1%2").arg(searchPaths[i]).arg("cdrecord");
    if( QFile::exists( bin ) ) {

      m_process->clearArguments();
      m_process->disconnect();
      *m_process << bin << "--version";
      connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	       this, SLOT(slotParseCdrtoolsVersion(KProcess*, char*, int)) );
      m_process->start( KProcess::Block, KProcess::Stdout );
      if( m_binMap["cdrecord"]->version.isEmpty() ) {
	qDebug("(K3bExternalBinManager) " + bin + " seems not to be cdrecord version >= 1.9.");
      }
      else {
	m_binMap["cdrecord"]->path = bin;
	break;  // bin found
      }
    }
  }
  if( m_binMap["cdrecord"]->path.isEmpty() ) {
    qDebug("(K3bExternalBinManager) Could not find cdrecord");
  }



  // search for cdrdao
  for( int i = 0; i < NUM_SEARCH_PATHS; i++ ) {

    QString bin = QString("%1%2").arg(searchPaths[i]).arg("cdrdao");
    if( QFile::exists( bin ) ) {

      m_process->clearArguments();
      m_process->disconnect();
      *m_process << bin;
      connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
	       this, SLOT(slotParseCdrdaoVersion(KProcess*, char*, int)) );
      m_process->start( KProcess::Block, KProcess::Stderr );
      if( m_binMap["cdrdao"]->version.isEmpty() ) {
	qDebug("(K3bExternalBinManager) " + bin + " seems not to be cdrdao version >= 1.1.3.");
      }
      else {
	m_binMap["cdrdao"]->path = bin;
	break;  // bin found
      }
    }
  }
  if( m_binMap["cdrdao"]->path.isEmpty() ) {
    qDebug("(K3bExternalBinManager) Could not find cdrdao");
  }


//   // search for mpg123
//   for( int i = 0; i < NUM_SEARCH_PATHS; i++ ) {

//     QString bin = QString("%1%2").arg(searchPaths[i]).arg("mpg123");
//     if( QFile::exists( bin ) ) {

//       m_process->clearArguments();
//       m_process->disconnect();
//       *m_process << bin;
//       connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
// 	       this, SLOT(slotParseMpg123Version(KProcess*, char*, int)) );
//       m_process->start( KProcess::Block, KProcess::Stderr );
//       if( m_binMap["mpg123"]->version.isEmpty() ) {
// 	qDebug("(K3bExternalBinManager) " + bin + " seems not to be mpg123.");
//       }
//       else {
// 	m_binMap["mpg123"]->path = bin;
// 	break;  // bin found
//       }
//     }
//   }
//   if( m_binMap["mpg123"]->path.isEmpty() ) {
//     qDebug("(K3bExternalBinManager) Could not find mpg123");
//   }


//   // search for sox
//   for( int i = 0; i < NUM_SEARCH_PATHS; i++ ) {

//     QString bin = QString("%1%2").arg(searchPaths[i]).arg("sox");
//     if( QFile::exists( bin ) ) {

//       m_process->clearArguments();
//       m_process->disconnect();
//       *m_process << bin << "-h";
//       connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
// 	       this, SLOT(slotParseSoxVersion(KProcess*, char*, int)) );
//       m_process->start( KProcess::Block, KProcess::Stderr );
//       if( m_binMap["sox"]->version.isEmpty() ) {
// 	qDebug("(K3bExternalBinManager) " + bin + " seems not to be sox.");
//       }
//       else {
// 	m_binMap["sox"]->path = bin;
// 	break;  // bin found
//       }
//     }
//   }
//   if( m_binMap["sox"]->path.isEmpty() ) {
//     qDebug("(K3bExternalBinManager) Could not find sox");
//   }
}


void K3bExternalBinManager::checkVersions()
{
  // search for mkisofs
  K3bExternalBin* binO = binObject("mkisofs");
  if( binO ) {
    binO->version = QString::null;
    if( QFile::exists( binO->path ) ) {

      m_process->clearArguments();
      m_process->disconnect();
      *m_process << binO->path << "--version";
      connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	       this, SLOT(slotParseCdrtoolsVersion(KProcess*, char*, int)) );
      m_process->start( KProcess::Block, KProcess::Stdout );
      if( m_binMap["mkisofs"]->version.isEmpty() ) {
	qDebug("(K3bExternalBinManager) " + binO->path + " seems not to be mkisofs.");
      }
    }
  }

  binO = binObject("cdrecord");
  if( binO ) {
    binO->version = QString::null;
    if( QFile::exists( binO->path ) ) {

      m_process->clearArguments();
      m_process->disconnect();
      *m_process << binO->path << "--version";
      connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	       this, SLOT(slotParseCdrtoolsVersion(KProcess*, char*, int)) );
      m_process->start( KProcess::Block, KProcess::Stdout );
      if( m_binMap["cdrecord"]->version.isEmpty() ) {
	qDebug("(K3bExternalBinManager) " + binO->path + " seems not to be cdrecord.");
      }
    }
  }

  binO = binObject("cdrdao");
  if( binO ) {
    binO->version = QString::null;
    if( QFile::exists( binO->path ) ) {

      m_process->clearArguments();
      m_process->disconnect();
      *m_process << binO->path;
      connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
	       this, SLOT(slotParseCdrdaoVersion(KProcess*, char*, int)) );
      m_process->start( KProcess::Block, KProcess::Stderr );
      if( m_binMap["cdrdao"]->version.isEmpty() ) {
	qDebug("(K3bExternalBinManager) " + binO->path + " seems not to be cdrdao.");
      }
    }
  }

//   binO = binObject("mpg123");
//   if( binO ) {
//     binO->version = QString::null;
//     if( QFile::exists( binO->path ) ) {

//       m_process->clearArguments();
//       m_process->disconnect();
//       *m_process << binO->path;
//       connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
// 	       this, SLOT(slotParseMpg123Version(KProcess*, char*, int)) );
//       m_process->start( KProcess::Block, KProcess::Stderr );
//       if( m_binMap["mpg123"]->version.isEmpty() ) {
// 	qDebug("(K3bExternalBinManager) " + binO->path + " seems not to be mpg123.");
//       }
//     }
//   }

//   binO = binObject("sox");
//   if( binO ) {
//     binO->version = QString::null;
//     if( QFile::exists( binO->path ) ) {

//       m_process->clearArguments();
//       m_process->disconnect();
//       *m_process << binO->path << "-h";
//       connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
// 	       this, SLOT(slotParseSoxVersion(KProcess*, char*, int)) );
//       m_process->start( KProcess::Block, KProcess::Stderr );
//       if( m_binMap["sox"]->version.isEmpty() ) {
// 	qDebug("(K3bExternalBinManager) " + binO->path + " seems not to be sox.");
//       }
//     }
//   }
}


void K3bExternalBinManager::slotParseCdrtoolsVersion( KProcess*, char* data, int len )
{
  QString buffer = QString::fromLatin1( data, len );
  int start = buffer.find( QRegExp("[0-9]") );
  int findStart = ( start > -1 ? start : 0 );
  int end   = buffer.find( ' ', findStart );

  if( start > -1 && end > -1 ) {
    if( buffer.contains( "mkisofs" ) ) {
      if( m_binMap.contains( "mkisofs" ) ) {
	m_binMap["mkisofs"]->version = buffer.mid( start, end-start );
      }
    }
    else if( buffer.contains( "Cdrecord" ) ) {
      if( m_binMap.contains( "cdrecord" ) ) {
	m_binMap["cdrecord"]->version = buffer.mid( start, end-start );
      }
    }
  }
}


void K3bExternalBinManager::slotParseCdrdaoVersion( KProcess*, char* data, int len )
{
  if( m_binMap.contains( "cdrdao" ) ) {
    QString buffer = QString::fromLatin1( data, len );
    QStringList lines = QStringList::split( "\n", buffer );
    
    for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ ) {
      if( (*str).startsWith( "Cdrdao version" ) ) {
	int start = (*str).find( "version" ) + 8;
	int findStart = ( start > -1 ? start : 0 );
	int end   = (*str).find( ' ', findStart );
	
	if( start > -1 && end > -1 ) {
	  m_binMap["cdrdao"]->version = (*str).mid( start, end-start );
	  break;   // version found
	}
      }
    }
  }
}


// void K3bExternalBinManager::slotParseMpg123Version( KProcess*, char* data, int len )
// {
//   if( m_binMap.contains( "mpg123" ) ) {
//     QString buffer = QString::fromLatin1( data, len );
//     QStringList lines = QStringList::split( "\n", buffer );
    
//     for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ ) {
//       if( (*str).contains( "Version" ) ) {
// 	int start = (*str).find( "Version" ) + 8;
// 	int findStart = ( start > -1 ? start : 0 );
// 	int end   = (*str).find( ' ', findStart );
	
// 	if( start > -1 && end > -1 ) {
// 	  m_binMap["mpg123"]->version = (*str).mid( start, end-start );
// 	  break;   // version found
// 	}
//       }
//     }
//   }
// }


// void K3bExternalBinManager::slotParseSoxVersion( KProcess*, char* data, int len )
// {
//   if( m_binMap.contains( "sox" ) ) {
//     QString buffer = QString::fromLatin1( data, len );
//     QStringList lines = QStringList::split( "\n", buffer );
    
//     for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ ) {
//       if( (*str).contains( "Version" ) ) {
// 	int start = (*str).find( "Version" ) + 8;
// 	if( start > -1 ) {
// 	  m_binMap["sox"]->version = (*str).mid( start );
// 	  break;   // version found
// 	}
//       }
//     }
//   }
// }


bool K3bExternalBinManager::readConfig( KConfig* c )
{
  if( c->hasKey( "cdrecord path" ) ) {
    QString path = c->readEntry( "cdrecord path" );
    if( QFile::exists( path ) ) {
      m_binMap["cdrecord"]->path = path;
    }
    else {
      qDebug( "(K3bExternalBinManager) config contains invalid cdrecord path");
    }
  }
  if( c->hasKey( "mkisofs path" ) ) {
    QString path = c->readEntry( "mkisofs path" );
    if( QFile::exists( path ) ) {
      m_binMap["mkisofs"]->path = path;
    }
    else {
      qDebug( "(K3bExternalBinManager) config contains invalid mkisofs path");
    }
  }
  if( c->hasKey( "cdrdao path" ) ) {
    QString path = c->readEntry( "cdrdao path" );
    if( QFile::exists( path ) ) {
      m_binMap["cdrdao"]->path = path;
    }
    else {
      qDebug( "(K3bExternalBinManager) config contains invalid cdrdao path");
    }
  }
//   if( c->hasKey( "mpg123 path" ) ) {
//     QString path = c->readEntry( "mpg123 path" );
//     if( QFile::exists( path ) ) {
//       m_binMap["mpg123"]->path = path;
//     }
//     else {
//       qDebug( "(K3bExternalBinManager) config contains invalid mpg123 path");
//     }
//   }
//   if( c->hasKey( "sox path" ) ) {
//     QString path = c->readEntry( "sox path" );
//     if( QFile::exists( path ) ) {
//       m_binMap["sox"]->path = path;
//     }
//     else {
//       qDebug( "(K3bExternalBinManager) config contains invalid sox path");
//     }
//   }

  return true;
}


bool K3bExternalBinManager::saveConfig( KConfig* c )
{
  if( QFile::exists( m_binMap["cdrecord"]->path ) )
    c->writeEntry( "cdrecord path", m_binMap["cdrecord"]->path );

  if( QFile::exists( m_binMap["mkisofs"]->path ) )
    c->writeEntry( "mkisofs path", m_binMap["mkisofs"]->path );

  if( QFile::exists( m_binMap["cdrdao"]->path ) )
    c->writeEntry( "cdrdao path", m_binMap["cdrdao"]->path );

//   if( QFile::exists( m_binMap["mpg123"]->path ) )
//     c->writeEntry( "mpg123 path", m_binMap["mpg123"]->path );

//   if( QFile::exists( m_binMap["sox"]->path ) )
//     c->writeEntry( "sox path", m_binMap["sox"]->path );


  return true;
}


bool K3bExternalBinManager::foundBin( const QString& name )
{
  if( !binObject( name ) )
    return false;
  else
    return ( !binObject( name )->version.isEmpty() );
}


const QString& K3bExternalBinManager::binPath( const QString& name )
{
  K3bExternalBin* extBin = binObject( name );
  if( extBin != 0 )
    return extBin->path;

  return m_noPath;
}


K3bExternalBin* K3bExternalBinManager::binObject( const QString& name )
{
  if( m_binMap.contains( name ) )
    return m_binMap[name];
  else
    return 0;
}


#include "k3bexternalbinmanager.moc"
