#include "k3bmixeddoc.h"
#include "k3bmixedjob.h"

#include "k3bmixedview.h"
#include "../data/k3bdatadoc.h"
#include "../audio/k3baudiodoc.h"

#include <klocale.h>


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

  m_mixedType = DATA_FIRST_TRACK;

  // TODO: overwrite default settings with mixed defaults

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
  // TODO: load user defaults
  setDummy(false);
}


#include "k3bmixeddoc.moc"

