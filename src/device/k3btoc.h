#ifndef K3BTOC_H
#define K3BTOC_H

#include <qlist.h>

#include "k3btrack.h"

class QString;

/**
 * A list of K3bTracks that represents the contents
 * of a cd.
 * The Toc deletes all its tracks when it is deleted and
 * deletes removed tracks.
 */
class K3bToc : public QList<K3bTrack>
{
 public:
  K3bToc();
  /** deep copy */
  K3bToc( const K3bToc& );
  /** create empty toc with artist and album info set */
  K3bToc( const QString&, const QString& );
  /** deletes all tracks */
  ~K3bToc();
  /** deep copy */
  K3bToc& operator=( const K3bToc& );

  /**
   * CDDB disc Id
   */
  unsigned int discId() const;

  /**
   * The first track's first sector could differ from the disc's
   * first sector if there is a pregap before index 1
   */
  int firstSector() const;
  int lastSector() const;
  const QString& artist() const { return m_artist; }
  const QString& album() const { return m_album; }
  int length() const;

  void setArtist( const QString& s ) { m_artist = s; }
  void setAlbum( const QString& s ) { m_album = s; }
  void setDiscId( unsigned int id ) { m_discId = id; }
  void setFirstSector( int i ) { m_firstSector = i; }

 private:
  unsigned int m_discId;
  int m_firstSector;
  //  int m_lastSector;
  QString m_artist;
  QString m_album;
};

#endif
