#include "k3btrack.h"


K3bTrack::K3bTrack()
{
  m_firstSector = -1;
  m_lastSector = -1;
  m_type = -1;
}


K3bTrack::K3bTrack( const K3bTrack& track )
  : m_firstSector( track.firstSector() ),
    m_lastSector( track.lastSector() ),
    m_type( track.type() ),
    m_title( track.title() )
{
}


K3bTrack::K3bTrack( int firstSector, int lastSector, int type, const QString& title )
  : m_firstSector( firstSector ), m_lastSector( lastSector ), m_type( type ), m_title( title )
{
}


K3bTrack& K3bTrack::operator=( const K3bTrack& track )
{
  m_firstSector = track.firstSector();
  m_lastSector = track.lastSector();
  m_type = track.type();
  m_title = track.title();
  
  return *this;
}


int K3bTrack::length() const
{
  return m_lastSector - m_firstSector;
}


void K3bTrack::setTitle( const QString& title )
{
  m_title = title;
}


bool K3bTrack::isEmpty() const
{
  return m_lastSector == -1;
}
