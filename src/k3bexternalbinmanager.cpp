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

  // TODO:delete all K3bExternalBin objects
}


void K3bExternalBinManager::search()
{
  m_binMap.clear();

  // search for mkisofs


  // search for cdrecord


  // search for cdrdao


  // search for mpg123


  // search for sox

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
  return ( binObject( name ) != 0 );
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
