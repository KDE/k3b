#include "k3btoc.h"

#include <qstring.h>


K3bToc::K3bToc()
  : QValueList<K3bTrack>()
{
  m_discId = 0;
  m_firstSector = -1;
}


K3bToc::K3bToc( const K3bToc& toc )
  : QValueList<K3bTrack>( toc ),
    m_artist( toc.artist() ),
    m_album( toc.album() )
{
  m_firstSector = toc.firstSector();
  m_discId = toc.discId();
}


K3bToc::K3bToc( const QString& artist, const QString& album )
  : QValueList<K3bTrack>(), m_artist( artist ), m_album( album )
{
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
  m_discId = toc.discId();

  QValueList<K3bTrack>::operator=( toc );

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
  if( isEmpty() )
    return 0;
  // the last track's last sector should be the last sector of the entire cd
  return last().lastSector();
}


int K3bToc::length() const
{
  return lastSector() - m_firstSector;
}
