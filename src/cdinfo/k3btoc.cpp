#include "k3btoc.h"

#include <qstring.h>


K3bToc::K3bToc()
{
  m_discId = 0;
  m_firstSector = -1;
  m_lastSector = -1;

  setAutoDelete( true );
}


K3bToc::K3bToc( const K3bToc& toc )
  : m_artist( toc.artist() ),
    m_album( toc.album() )
{
  m_firstSector = toc.firstSector();
  m_lastSector = toc.lastSector();
  m_discId = toc.discId();

  QListIterator<K3bTrack> it( toc );
  for( ; it.current(); ++it ) {
    append( new K3bTrack( *it.current() ) );
  }

  setAutoDelete( true );
}


K3bToc::K3bToc( const QString& artist, const QString& album )
  : m_artist( artist ), m_album( album )
{
  m_discId = 0;
  m_firstSector = -1;
  m_lastSector = -1;

  setAutoDelete( true );
}


K3bToc::~K3bToc()
{
}


K3bToc& K3bToc::operator=( const K3bToc& toc )
{
  if( &toc == this ) return *this;

  m_artist = toc.artist();
  m_album = toc.album();

  m_firstSector = toc.firstSector();
  m_lastSector = toc.lastSector();
  m_discId = toc.discId();

  clear();
  QListIterator<K3bTrack> it( toc );
  for( ; it.current(); ++it ) {
    append( new K3bTrack( *it.current() ) );
  }

  return *this;
}


unsigned int K3bToc::discId() const
{
  return m_discId;
}


int K3bToc::firstSector() const
{
  return m_firstSector;
}


int K3bToc::lastSector() const
{
  // the last track's last sector should be the last sector of the entire cd
  return getLast()->lastSector();
}


int K3bToc::length() const
{
  return m_lastSector - m_firstSector;
}
