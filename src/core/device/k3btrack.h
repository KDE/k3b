/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */



#ifndef K3BTRACK_H
#define K3BTRACK_H

#include <qstring.h>

#include <k3bmsf.h>

namespace K3bCdDevice
{

  class Track
  {
    friend class CdDevice;

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
    Track( const K3b::Msf& firstSector, 
	   const K3b::Msf& lastSector, 
	   int type, 
	   int mode = UNKNOWN, 
	   const QString& = QString::null );
    Track& operator=( const Track& );

    const QString& title() const { return m_title; }
    int type() const { return m_type; }

    /**
     * Invalid for DVDs and Audio CDs
     */
    int mode() const { return m_mode; }

    /**
     * Invalid for DVDs
     */
    bool copyPermitted() const { return m_copyPermitted; }

    /**
     * Only valid for audio tracks
     */
    bool preEmphasis() const { return m_preEmphasis; }

    const K3b::Msf& firstSector() const { return m_firstSector; }
    const K3b::Msf& lastSector() const { return m_lastSector; }
    K3b::Msf length() const;

    /**
     * This takes index0 into account
     */
    K3b::Msf realAudioLength() const;

    long index0() const { return m_index0; }
    
    /**
     * 0 if unknown
     */
    int session() const { return m_session; }

    void setTitle( const QString& );

  private:
    K3b::Msf m_firstSector;
    K3b::Msf m_lastSector;

    long m_index0;

    int m_type;
    int m_mode;
    bool m_copyPermitted;
    bool m_preEmphasis;

    int m_session;

    QString m_title;
  };
}

typedef K3bCdDevice::Track K3bTrack;

#endif
