#include "k3bexternalbinmanager.h"

#include <kdebug.h>
#include <kprocess.h>
#include <kconfig.h>

#include <qstring.h>
#include <qregexp.h>
#include <qfile.h>
#include <qfileinfo.h>

#include <unistd.h>
#include <sys/stat.h>



static const char* vcdTools[] =  { "vcdxgen",
				   "vcdxbuild",
           "vcdxminfo",
				   0 };

static const char* transcodeTools[] =  { "transcode",
					 "tcprobe",
					 "tccat",
					 "tcscan",
					 "tcextract",
					 "tcdecode",
					 0 };

static const char* binPrograms[] =  { "cdrecord",
				      "cdrdao",
				      "mkisofs",
				      0 };



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


bool K3bExternalBin::hasFeature( const QString& f ) const
{
  return m_features.contains( f );
}


void K3bExternalBin::addFeature( const QString& f )
{
  m_features.append( f );
}


void K3bExternalBin::addUserParameter( const QString& p )
{
  m_userParameters.append( p );
}




// ///////////////////////////////////////////////////////////
// 
// K3BEXTERNALBINMANAGER
// 
// ///////////////////////////////////////////////////////////


K3bExternalBinManager::K3bExternalBinManager()
  : QObject()
{
}


K3bExternalBinManager::~K3bExternalBinManager()
{
  for( QMap<QString, K3bExternalProgram*>::Iterator it = m_programs.begin(); it != m_programs.end(); ++it )
    delete it.data();
}


K3bExternalBin* K3bExternalBinManager::probeCdrecord( const QString& path )
{
  if( !QFile::exists( path ) )
    return 0;

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  vp << path << "-version";
  connect( &vp, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
  connect( &vp, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
  m_gatheredOutput = "";
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = m_gatheredOutput.find( "Cdrecord" );
    if( pos < 0 )
      return 0;

    pos = m_gatheredOutput.find( QRegExp("[0-9]"), pos );
    if( pos < 0 )
      return 0;

    int endPos = m_gatheredOutput.find( ' ', pos+1 );
    if( endPos < 0 )
      return 0;

    bin = new K3bExternalBin( "cdrecord" );
    bin->path = path;
    bin->version = m_gatheredOutput.mid( pos, endPos-pos );
  }
  else {
    kdDebug() << "(K3bExternalBinManager) could not start " << path << endl;
    return 0;
  }



  // probe features
  KProcess fp;
  fp << path << "-help";
  connect( &fp, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
  connect( &fp, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
  m_gatheredOutput = "";
  if( fp.start( KProcess::Block, KProcess::AllOutput ) ) {
    if( m_gatheredOutput.contains( "gracetime" ) )
      bin->addFeature( "gracetime" );
    if( m_gatheredOutput.contains( "-overburn" ) )
      bin->addFeature( "overburn" );
    if( m_gatheredOutput.contains( "-text" ) )
      bin->addFeature( "cdtext" );
    if( m_gatheredOutput.contains( "-clone" ) )  // cdrecord ProDVD
      bin->addFeature( "clone" );

    // check if we run cdrecord as root
    if( !getuid() )
      bin->addFeature( "suidroot" );
    else {
      struct stat s;
      if( !::stat( path.latin1(), &s ) ) {
	if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
	  bin->addFeature( "suidroot" );
      }
    }
  }
  else {
    kdDebug() << "(K3bExternalBinManager) could not start " << bin->path << endl;
    delete bin;
    return 0;
  }

  return bin;
}


K3bExternalBin* K3bExternalBinManager::probeMkisofs( const QString& path )
{
  if( !QFile::exists( path ) )
    return 0;

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  vp << path << "-version";
  connect( &vp, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
  connect( &vp, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
  m_gatheredOutput = "";
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = m_gatheredOutput.find( "mkisofs" );
    if( pos < 0 )
      return 0;

    pos = m_gatheredOutput.find( QRegExp("[0-9]"), pos );
    if( pos < 0 )
      return 0;

    int endPos = m_gatheredOutput.find( ' ', pos+1 );
    if( endPos < 0 )
      return 0;

    bin = new K3bExternalBin( "mkisofs" );
    bin->path = path;
    bin->version = m_gatheredOutput.mid( pos, endPos-pos );
  }
  else {
    kdDebug() << "(K3bExternalBinManager) could not start " << path << endl;
    return 0;
  }



  // probe features
  KProcess fp;
  fp << path << "-help";
  connect( &fp, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
  connect( &fp, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
  m_gatheredOutput = "";
  if( fp.start( KProcess::Block, KProcess::AllOutput ) ) {
    if( m_gatheredOutput.contains( "-udf" ) )
      bin->addFeature( "udf" );
    if( m_gatheredOutput.contains( "-dvd-video" ) )
      bin->addFeature( "dvd-video" );
    if( m_gatheredOutput.contains( "-joliet-long" ) )
      bin->addFeature( "joliet-long" );
  }
  else {
    kdDebug() << "(K3bExternalBinManager) could not start " << bin->path << endl;
    delete bin;
    return 0;
  }

  return bin;
}


K3bExternalBin* K3bExternalBinManager::probeCdrdao( const QString& path )
{
  if( !QFile::exists( path ) )
    return 0;

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  vp << path ;
  connect( &vp, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
  connect( &vp, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
  m_gatheredOutput = "";
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = m_gatheredOutput.find( "Cdrdao version" );
    if( pos < 0 )
      return 0;

    pos = m_gatheredOutput.find( QRegExp("[0-9]"), pos );
    if( pos < 0 )
      return 0;

    int endPos = m_gatheredOutput.find( ' ', pos+1 );
    if( endPos < 0 )
      return 0;

    bin = new K3bExternalBin( "cdrdao" );
    bin->path = path;
    bin->version = m_gatheredOutput.mid( pos, endPos-pos );
  }
  else {
    kdDebug() << "(K3bExternalBinManager) could not start " << path << endl;
    return 0;
  }



  // probe features
  KProcess fp;
  fp << path << "write" << "-h";
  connect( &fp, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
  connect( &fp, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
  m_gatheredOutput = "";
  if( fp.start( KProcess::Block, KProcess::AllOutput ) ) {
    if( m_gatheredOutput.contains( "--overburn" ) )
      bin->addFeature( "overburn" );
    if( m_gatheredOutput.contains( "--multi" ) )
      bin->addFeature( "multisession" );

    // check if we run cdrdao as root
    if( !getuid() )
      bin->addFeature( "suidroot" );
    else {
      struct stat s;
      if( !::stat( path.latin1(), &s ) ) {
	if( (s.st_mode & S_ISUID) && s.st_uid == 0 )
	  bin->addFeature( "suidroot" );
      }
    }
  }
  else {
    kdDebug() << "(K3bExternalBinManager) could not start " << bin->path << endl;
    delete bin;
    return 0;
  }

  return bin;
}



K3bExternalBin* K3bExternalBinManager::probeTranscode( const QString& path )
{
  if( !QFile::exists( path ) )
    return 0;

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  vp << path ;
  connect( &vp, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
  connect( &vp, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
  m_gatheredOutput = "";
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = m_gatheredOutput.find( "transcode v" );
    if( pos < 0 )
      return 0;

    pos += 11;

    int endPos = m_gatheredOutput.find( QRegExp("[\\s\\)]"), pos+1 );
    if( endPos < 0 )
      return 0;

    bin = new K3bExternalBin( "transcode" );
    bin->path = path;
    bin->version = m_gatheredOutput.mid( pos, endPos-pos );
  }
  else {
    kdDebug() << "(K3bExternalBinManager) could not start " << path << endl;
    return 0;
  }

  return bin;
}


K3bExternalBin* K3bExternalBinManager::probeMovix( const QString& path )
{
  // we need the following files:
  // isolinux/initrd.gz
  // isolinux/iso.sort
  // isolinux/isolinux.bin
  // isolinux/isolinux.cfg
  // isolinux/movix.lss
  // isolinux/mphelp.txt
  // isolinux/mxhelp.txt
  // isolinux/trblst.txt
  // isolinux/credits.txt
  // isolinux/kernel/vmlinuz

  return 0;
}


K3bExternalBin* K3bExternalBinManager::probeMovix2( const QString& )
{
  return 0;
}


K3bExternalBin* K3bExternalBinManager::probeVcd( const QString& path )
{
  if( !QFile::exists( path ) )
    return 0;

  K3bExternalBin* bin = 0;

  // probe version
  KProcess vp;
  vp << path << "-V";
  connect( &vp, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
  connect( &vp, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(gatherOutput(KProcess*, char*, int)) );
  m_gatheredOutput = "";
  if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
    int pos = m_gatheredOutput.find( "GNU VCDImager" );
    if( pos < 0 )
      return 0;

    pos += 14;

    int endPos = m_gatheredOutput.find( QRegExp("[\\n\\)]"), pos+1 );
    if( endPos < 0 )
      return 0;

    bin = new K3bExternalBin( "vcdxgen" );
    bin->path = path;
    bin->version = m_gatheredOutput.mid( pos, endPos-pos ).stripWhiteSpace();
  }
  else {
    kdDebug() << "(K3bExternalBinManager) could not start " << path << endl;
    return 0;
  }

  return bin;
}



void K3bExternalBinManager::gatherOutput( KProcess*, char* data, int len )
{
  m_gatheredOutput.append( QString::fromLatin1( data, len ) );
}


bool K3bExternalBinManager::readConfig( KConfig* c )
{
  loadDefaultSearchPath();  

  if( c->hasKey( "search path" ) )
    setSearchPath( c->readListEntry( "search path" ) );

  search();

  for ( QMap<QString, K3bExternalProgram*>::iterator it = m_programs.begin(); it != m_programs.end(); ++it ) {
    K3bExternalProgram* p = it.data();
    if( c->hasKey( p->name() + " default" ) ) {
      p->setDefault( c->readEntry( p->name() + " default" ) );
    }
    if( c->hasKey( p->name() + " user parameters" ) ) {
      QStringList list = c->readListEntry( p->name() + " user parameters" );
      for( QStringList::iterator strIt = list.begin(); strIt != list.end(); ++strIt )
	p->addUserParameter( *strIt );
    }
  }

  return true;
}

bool K3bExternalBinManager::saveConfig( KConfig* c )
{
  c->writeEntry( "search path", m_searchPath );

  for ( QMap<QString, K3bExternalProgram*>::iterator it = m_programs.begin(); it != m_programs.end(); ++it ) {
    K3bExternalProgram* p = it.data();
    if( p->defaultBin() )
      c->writeEntry( p->name() + " default", p->defaultBin()->path );

    c->writeEntry( p->name() + " user parameters", p->userParameters() );
  }

  return true;
}


bool K3bExternalBinManager::foundBin( const QString& name )
{
  if( m_programs.find( name ) == m_programs.end() )
    return false;
  else
    return (m_programs[name]->defaultBin() != 0);
}


const QString& K3bExternalBinManager::binPath( const QString& name )
{
  if( m_programs.find( name ) == m_programs.end() )
    return m_noPath;

  if( m_programs[name]->defaultBin() != 0 )
    return m_programs[name]->defaultBin()->path;
  else
    return m_noPath;
}


const K3bExternalBin* K3bExternalBinManager::binObject( const QString& name )
{
  if( m_programs.find( name ) == m_programs.end() )
    return 0;

  return m_programs[name]->defaultBin();
}


void K3bExternalBinManager::createProgramContainer()
{
  for( int i = 0; binPrograms[i]; ++i ) {
    if( m_programs.find( binPrograms[i] ) == m_programs.end() )
      m_programs.insert( binPrograms[i], new K3bExternalProgram( binPrograms[i] ) );
  }  
  for( int i = 0; transcodeTools[i]; ++i ) {
    if( m_programs.find( transcodeTools[i] ) == m_programs.end() )
      m_programs.insert( transcodeTools[i], new K3bExternalProgram( transcodeTools[i] ) );
  }  
  for( int i = 0; vcdTools[i]; ++i ) {
    if( m_programs.find( vcdTools[i] ) == m_programs.end() )
      m_programs.insert( vcdTools[i], new K3bExternalProgram( vcdTools[i] ) );
  }

  if( m_programs.find( "movix" ) == m_programs.end() )
    m_programs.insert( "movix", new K3bExternalProgram( "movix" ) );
  if( m_programs.find( "movix2" ) == m_programs.end() )
    m_programs.insert( "movix2", new K3bExternalProgram( "movix2" ) );
}


void K3bExternalBinManager::search()
{
  if( m_searchPath.isEmpty() )
    loadDefaultSearchPath();

  createProgramContainer();

  for( QMap<QString, K3bExternalProgram*>::iterator it = m_programs.begin(); it != m_programs.end(); ++it ) {
    it.data()->clear();
  }

  for( QStringList::const_iterator it = m_searchPath.begin(); it != m_searchPath.end(); ++it ) {
    QString path = *it;
    if( path[path.length()-1] == '/' )
      path.truncate( path.length()-1 );

    QFileInfo fi( path );
    if( !fi.exists() )
      continue;

    K3bExternalBin* cdrecordBin = 0;
    K3bExternalBin* mkisofsBin = 0;
    K3bExternalBin* cdrdaoBin = 0;

    if( fi.isDir() ) {
       cdrecordBin = probeCdrecord( path + "/cdrecord" );
       mkisofsBin = probeMkisofs( path + "/mkisofs" );
       cdrdaoBin = probeCdrdao( path + "/cdrdao" );

       for( int i = 0; transcodeTools[i]; ++i ) {
         K3bExternalBin* bin = probeTranscode( path + "/" + QString::fromLatin1(transcodeTools[i]) );
         if( bin )
          m_programs[ transcodeTools[i] ]->addBin( bin );
       }

       for( int i = 0; vcdTools[i]; ++i ) {
         K3bExternalBin* bin = probeVcd( path + "/" + QString::fromLatin1(vcdTools[i]) );
         if( bin )
          m_programs[ vcdTools[i] ]->addBin( bin );
       }

       K3bExternalBin* movixBin = probeMovix( path + "/movix" );
       if( movixBin )
	 m_programs[ "movix" ]->addBin( movixBin );
    }
    else {
       cdrecordBin = probeCdrecord( path );
       mkisofsBin = probeMkisofs( path );
       cdrdaoBin = probeCdrdao( path );

       // TODO: is there any way to test the other tools?
    }
    if( cdrecordBin )
      m_programs["cdrecord"]->addBin( cdrecordBin );
    if( mkisofsBin )
      m_programs["mkisofs"]->addBin( mkisofsBin );
    if( cdrdaoBin )
      m_programs["cdrdao"]->addBin( cdrdaoBin );
  }



  // TESTING
  // /////////////////////////
  const K3bExternalBin* bin = program("cdrecord")->defaultBin();

  if( !bin ) {
    kdDebug() << "(K3bExternalBinManager) Probing cdrecord failed" << endl;
  }
  else {
    kdDebug() << "(K3bExternalBinManager) Cdrecord " << bin->version << " features: " 
	      << bin->features().join( ", " ) << endl;
    
    if( bin->version >= "1.11a02" )
      kdDebug() << "(K3bExternalBinManager) seems to be cdrecord version >= 1.11a02, using burnfree instead of burnproof" << endl;
    if( bin->version >= "1.11a31" )
      kdDebug() << "(K3bExternalBinManager) seems to be cdrecord version >= 1.11a31, support for Just Link via burnfree "
		<< "driveroption" << endl;
  }
}


K3bExternalProgram* K3bExternalBinManager::program( const QString& name ) const
{
  if( m_programs.find( name ) == m_programs.end() )
    return 0;
  else
    return m_programs[name];
}


void K3bExternalBinManager::loadDefaultSearchPath()
{
  static const char* defaultSearchPaths[] = { "/usr/bin/",
					      "/usr/local/bin/",
					      "/usr/sbin/",
					      "/usr/local/sbin/",
					      "/opt/schily/bin/",
					      0 };

  m_searchPath.clear();
  for( int i = 0; defaultSearchPaths[i]; ++i ) {
    m_searchPath.append( defaultSearchPaths[i] );
  }
}


void K3bExternalBinManager::setSearchPath( const QStringList& list )
{
  loadDefaultSearchPath();

  for( QStringList::const_iterator it = list.begin(); it != list.end(); ++it ) {
    if( !m_searchPath.contains( *it ) )
      m_searchPath.append( *it );
  }
}


void K3bExternalBinManager::addSearchPath( const QString& path )
{
  if( !m_searchPath.contains( path ) )
    m_searchPath.append( path );
}


// ///////////////////////////////////////////////////////////
// 
// K3BEXTERNALPROGRAM
// 
// ///////////////////////////////////////////////////////////


K3bExternalProgram::K3bExternalProgram( const QString& name )
  : m_name( name )
{
  m_bins.setAutoDelete( true );
}


K3bExternalProgram::~K3bExternalProgram()
{
}

void K3bExternalProgram::addBin( K3bExternalBin* bin )
{
  if( !m_bins.contains( bin ) )
    m_bins.append( bin );
}

void K3bExternalProgram::setDefault( K3bExternalBin* bin )
{
  if( m_bins.contains( bin ) )
    m_bins.take( m_bins.find( bin ) );

  m_bins.insert( 0, bin );
}


void K3bExternalProgram::setDefault( const QString& path )
{
  for( QPtrListIterator<K3bExternalBin> it( m_bins ); it.current(); ++it ) {
    if( it.current()->path == path ) {
      setDefault( it.current() );
      return;
    }
  }
}


void K3bExternalProgram::addUserParameter( const QString& p )
{
  if( !m_userParameters.contains( p ) )
    m_userParameters.append(p);
}


K3bExternalBinManager* K3bExternalBinManager::self()
{
  static K3bExternalBinManager* instance = 0;
  if( !instance )
    instance = new K3bExternalBinManager();
  return instance;
}


#include "k3bexternalbinmanager.moc"

