#include "k3btoc.h"

#include <qstring.h>


K3bCdDevice::Toc::Toc()
  : QValueList<K3bCdDevice::Track>()
{
  m_discId = 0;
  m_firstSector = -1;
}


K3bCdDevice::Toc::Toc( const Toc& toc )
  : QValueList<K3bCdDevice::Track>( toc ),
    m_artist( toc.artist() ),
    m_album( toc.album() )
{
  m_firstSector = toc.firstSector();
  m_discId = toc.discId();
}


K3bCdDevice::Toc::Toc( const QString& artist, const QString& album )
  : QValueList<K3bCdDevice::Track>(), m_artist( artist ), m_album( album )
{
}


K3bCdDevice::Toc::~Toc()
{
}


K3bCdDevice::Toc& K3bCdDevice::Toc::operator=( const Toc& toc )
{
  if( &toc == this ) return *this;

  m_artist = toc.artist();
  m_album = toc.album();

  m_firstSector = toc.firstSector();
  m_discId = toc.discId();

  QValueList<K3bCdDevice::Track>::operator=( toc );

  return *this;
}


unsigned int K3bCdDevice::Toc::discId() const
{
  return m_discId;
}


int K3bCdDevice::Toc::firstSector() const
{
  return m_firstSector;
}


int K3bCdDevice::Toc::lastSector() const
{
  if( isEmpty() )
    return 0;
  // the last track's last sector should be the last sector of the entire cd
  return last().lastSector();
}


int K3bCdDevice::Toc::length() const
{
  return lastSector() - m_firstSector;
}
