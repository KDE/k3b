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
  m_binMap.clear();

  // search for mkisofs
  m_binMap.insert("mkisofs", new K3bExternalBin( "mkisofs" ));
  if( QFile::exists( "/usr/bin/mkisofs" ) ) {
    m_binMap["mkisofs"]->path = "/usr/bin/mkisofs";
  }
  else if( QFile::exists( "/usr/local/bin/mkisofs" ) ) {
    m_binMap["mkisofs"]->path = "/usr/local/bin/mkisofs";
  }
  if( !m_binMap["mkisofs"]->path.isEmpty() ) {
    m_process->clearArguments();
    m_process->disconnect();
    *m_process << m_binMap["mkisofs"]->path << "--version";
    connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	     this, SLOT(slotParseCdrtoolsVersion(KProcess*, char*, int)) );
    m_process->start( KProcess::Block, KProcess::Stdout );
    if( m_binMap["mkisofs"]->version.isEmpty() ) {
      qDebug("(K3bExternalBinManager) " + m_binMap["mkisofs"]->path + " seems not to be mkisofs version >= 1.13. removing path.");
      m_binMap["mkisofs"]->path = QString::null;
    }
  }
  else {
    qDebug("(K3bExternalBinManager) Could not find mkisofs");
  }


  // search for cdrecord
  m_binMap.insert("cdrecord", new K3bExternalBin( "cdrecord" ));
  if( QFile::exists( "/usr/bin/cdrecord" ) ) {
    m_binMap["cdrecord"]->path = "/usr/bin/cdrecord";
  }
  else if( QFile::exists( "/usr/local/bin/cdrecord" ) ) {
    m_binMap["cdrecord"]->path = "/usr/local/bin/cdrecord";
  }
  if( !m_binMap["cdrecord"]->path.isEmpty() ) {
    m_process->clearArguments();
    m_process->disconnect();
    *m_process << m_binMap["cdrecord"]->path << "--version";
    connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	     this, SLOT(slotParseCdrtoolsVersion(KProcess*, char*, int)) );
    m_process->start( KProcess::Block, KProcess::Stdout );
    if( m_binMap["cdrecord"]->version.isEmpty() ) {
      qDebug("(K3bExternalBinManager) " + m_binMap["cdrecord"]->path + " seems not to be cdrecord version >= 1.9. removing path.");
      m_binMap["cdrecord"]->path = QString::null;
    }
  }
  else {
    qDebug("(K3bExternalBinManager) Could not find cdrecord");
  }


  // search for cdrdao
  m_binMap.insert("cdrdao", new K3bExternalBin( "cdrdao" ));
  if( QFile::exists( "/usr/bin/cdrdao" ) ) {
    m_binMap["cdrdao"]->path = "/usr/bin/cdrdao";
  }
  else if( QFile::exists( "/usr/local/bin/cdrdao" ) ) {
    m_binMap["cdrdao"]->path = "/usr/local/bin/cdrdao";
  }
  if( !m_binMap["cdrdao"]->path.isEmpty() ) {
    m_process->clearArguments();
    m_process->disconnect();
    *m_process << m_binMap["cdrdao"]->path;
    connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
	     this, SLOT(slotParseCdrtoolsVersion(KProcess*, char*, int)) );
    m_process->start( KProcess::Block, KProcess::Stderr );
    if( m_binMap["cdrdao"]->version.isEmpty() ) {
      qDebug("(K3bExternalBinManager) " + m_binMap["cdrdao"]->path + " seems not to be cdrdao version >= 1.1.3. removing path.");
      m_binMap["cdrdao"]->path = QString::null;
    }
  }
  else {
    qDebug("(K3bExternalBinManager) Could not find cdrdao");
  }


  // search for mpg123
  // will be removed in 0.6

  // search for sox
  // will *perhaps* also be removed in 0.6
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
      if( (*str).contains( "version" ) ) {
	int start = buffer.find( QRegExp("[0-9]") );
	int findStart = ( start > -1 ? start : 0 );
	int end   = buffer.find( ' ', findStart );
	
	if( start > -1 && end > -1 ) {
	  m_binMap["cdrdao"]->version = (*str).mid( start, end-start );
	  break;   // version found
	}
      }
    }
  }
}


bool K3bExternalBinManager::readConfig( KConfig* c )
{
  // TODO: read configuration and overwrite existing entries
  return true;
}


bool K3bExternalBinManager::saveConfig( KConfig* c )
{
  return true;
}


bool K3bExternalBinManager::foundBin( const QString& name )
{
  return ( !binObject( name )->path.isEmpty() );
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
