#include "k3btoc.h"

#include <qstring.h>


K3bCdDevice::Toc::Toc()
  : QValueList<K3bCdDevice::Track>()
{
  m_discId = 0;
  m_firstSector = 0;
}


K3bCdDevice::Toc::Toc( const Toc& toc )
  : QValueList<K3bCdDevice::Track>( toc )
{
  m_firstSector = toc.firstSector();
  m_discId = toc.discId();
}


K3bCdDevice::Toc::~Toc()
{
}


K3bCdDevice::Toc& K3bCdDevice::Toc::operator=( const Toc& toc )
{
  if( &toc == this ) return *this;

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
  // +1 since the last sector is included
  return lastSector() - m_firstSector + 1;
}


unsigned int K3bCdDevice::Toc::calculateDiscId()
{
  // calculate cddb-id
  unsigned int id = 0;
  for( K3bToc::iterator it = begin(); it != end(); ++it ) {
    unsigned int n = (*it).firstSector() + 150;
    n /= 75;
    while( n > 0 ) {
      id += n % 10;
      n /= 10;
    }
  }
  unsigned int l = length();
  l /= 75;
  id = ( ( id % 0xff ) << 24 ) | ( l << 8 ) | count();

  setDiscId( id );

  return discId();
}


int K3bCdDevice::Toc::contentType() const
{
  int audioCnt = 0, dataCnt = 0;
  for( K3bToc::const_iterator it = begin(); it != end(); ++it ) {
    if( (*it).type() == K3bCdDevice::Track::AUDIO )
      audioCnt++;
    else
      dataCnt++;
  }

  if( audioCnt + dataCnt == 0 )
    return K3bCdDevice::NONE;
  if( audioCnt == 0 )
    return K3bCdDevice::DATA;
  if( dataCnt == 0 )
    return K3bCdDevice::AUDIO;
  return K3bCdDevice::MIXED;
}
