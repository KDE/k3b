#ifndef K3BTRACK_H
#define K3BTRACK_H

#include <qstring.h>


class K3bTrack
{
 public:
  K3bTrack();
  K3bTrack( const K3bTrack& );
  K3bTrack( int, int, int, const QString& = QString::null );
  K3bTrack& operator=( const K3bTrack& );

  enum track_type { AUDIO, DATA };

  const QString& title() const { return m_title; }
  int type() const { return m_type; }
  int firstSector() const { return m_firstSector; }
  int lastSector() const { return m_lastSector; }
  int length() const;

  void setTitle( const QString& );

  bool isEmpty() const;

 private:
  int m_firstSector;
  int m_lastSector;
  int m_type;
  QString m_title;
};

#endif
