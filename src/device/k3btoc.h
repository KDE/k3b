#ifndef K3BTOC_H
#define K3BTOC_H

#include <qvaluelist.h>

#include "k3btrack.h"

class QString;

namespace K3bCdDevice
{
  /**
   * A list of K3bTracks that represents the contents
   * of a cd.
   * The Toc deletes all its tracks when it is deleted and
   * deletes removed tracks.
   */
  class Toc : public QValueList<K3bTrack>
  {
  public:
    Toc();
    /** deep copy */
    Toc( const Toc& );
    /** deletes all tracks */
    ~Toc();
    /** deep copy */
    Toc& operator=( const Toc& );

    /**
     * CDDB disc Id
     */
    unsigned int discId() const;
    unsigned int calculateDiscId();

    /**
     * The first track's first sector could differ from the disc's
     * first sector if there is a pregap before index 1
     */
    int firstSector() const;
    int lastSector() const;
    int length() const;

    void setDiscId( unsigned int id ) { m_discId = id; }
    void setFirstSector( int i ) { m_firstSector = i; }

  private:
    unsigned int m_discId;
    int m_firstSector;
    //  int m_lastSector;
  };
};

typedef K3bCdDevice::Toc K3bToc;

#endif
