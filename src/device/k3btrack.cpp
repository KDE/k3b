#include "k3btrack.h"


K3bCdDevice::Track::Track()
{
  m_firstSector = -1;
  m_lastSector = -1;
  m_type = -1;
  m_mode = -1;
}


K3bCdDevice::Track::Track( const Track& track )
  : m_firstSector( track.firstSector() ),
    m_lastSector( track.lastSector() ),
    m_type( track.type() ),
    m_mode( track.mode() ),
    m_title( track.title() )
{
}


K3bCdDevice::Track::Track( int firstSector, int lastSector, int type, int mode, const QString& title )
  : m_firstSector( firstSector ), m_lastSector( lastSector ), m_type( type ), m_mode( mode ), m_title( title )
{
}


K3bCdDevice::Track& K3bCdDevice::Track::operator=( const K3bTrack& track )
{
  m_firstSector = track.firstSector();
  m_lastSector = track.lastSector();
  m_type = track.type();
  m_mode = track.mode();
  m_title = track.title();

  return *this;
}


int K3bCdDevice::Track::length() const
{
  // +1 since the last sector is included
  return m_lastSector - m_firstSector + 1;
}


void K3bCdDevice::Track::setTitle( const QString& title )
{
  m_title = title;
}


bool K3bCdDevice::Track::isEmpty() const
{
  return m_lastSector == -1;
}
