#ifndef K3BTRACK_H
#define K3BTRACK_H

#include <qstring.h>

namespace K3bCdDevice
{

  class Track
  {
  public:
    enum TrackType { 
      AUDIO, 
      DATA 
    };

    enum DataMode { 
      MODE1, 
      MODE2, 
      XA_FORM1, 
      XA_FORM2, 
      UNKNOWN
    };

    Track();
    Track( const Track& );
    Track( int firstSector, int lastSector, int type, int mode = UNKNOWN, const QString& = QString::null );
    Track& operator=( const Track& );

    const QString& title() const { return m_title; }
    int type() const { return m_type; }
    int mode() const { return m_mode; }
    int firstSector() const { return m_firstSector; }
    int lastSector() const { return m_lastSector; }
    int length() const;

    void setTitle( const QString& );

    bool isEmpty() const;

  private:
    int m_firstSector;
    int m_lastSector;
    int m_type;
    int m_mode;
    QString m_title;
  };
}

typedef K3bCdDevice::Track K3bTrack;

#endif
