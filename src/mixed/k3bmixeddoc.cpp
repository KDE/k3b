#include "k3bmixeddoc.h"
#include "k3bmixedjob.h"

#include "k3bmixedview.h"
#include "../data/k3bdatadoc.h"
#include "../audio/k3baudiodoc.h"

#include <qfileinfo.h>

#include <klocale.h>
#include <kconfig.h>
#include <kapplication.h>


K3bMixedDoc::K3bMixedDoc( QObject* parent )
  : K3bDoc( parent )
{
  m_dataDoc = new K3bDataDoc( this );
  m_audioDoc = new K3bAudioDoc( this );
}


K3bMixedDoc::~K3bMixedDoc()
{
}


bool K3bMixedDoc::newDocument()
{
  m_dataDoc->newDocument();
  m_dataDoc->isoOptions().setVolumeID( i18n("Project name", "Mixed") );
  m_audioDoc->newDocument();

  return K3bDoc::newDocument();
}


unsigned long long K3bMixedDoc::size() const
{
  return m_dataDoc->size() + m_audioDoc->size();
}

unsigned long long K3bMixedDoc::length() const
{
  return m_dataDoc->length() + m_audioDoc->length();
}


K3bView* K3bMixedDoc::newView( QWidget* parent )
{
  return new K3bMixedView( this, parent );
}


int K3bMixedDoc::numOfTracks() const
{
  return m_audioDoc->numOfTracks() + 1;
}


K3bBurnJob* K3bMixedDoc::newBurnJob()
{
  return new K3bMixedJob( this );
}


void K3bMixedDoc::addUrl( const KURL& url )
{

}


void K3bMixedDoc::addUrls( const KURL::List& urls )
{

}


bool K3bMixedDoc::loadDocumentData( QDomDocument* )
{
}


bool K3bMixedDoc::saveDocumentData( QDomDocument* )
{
}

  
void K3bMixedDoc::loadDefaultSettings()
{
  KConfig* c = kapp->config();
  c->setGroup( "default mixed settings" );

  setDummy( c->readBoolEntry( "dummy_mode", false ) );
  setDao( c->readBoolEntry( "dao", true ) );
  setOnTheFly( c->readBoolEntry( "on_the_fly", true ) );
  setBurnproof( c->readBoolEntry( "burnproof", true ) );
  setRemoveBufferFiles( c->readBoolEntry( "remove_buffer_files", true ) );

  // TODO: load mixed type

  K3bIsoOptions o = K3bIsoOptions::load( c );
  dataDoc()->isoOptions() = o;
}


void K3bMixedDoc::setImagePath( const QString& path )
{
  // check if it's a file and if so just take the dir
  QFileInfo info( path );
  m_imagePath = info.dirPath(true);
}


#include "k3bmixeddoc.moc"

