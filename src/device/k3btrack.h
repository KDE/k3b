/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */



#ifndef K3BTRACK_H
#define K3BTRACK_H

#include <qcstring.h>
#include <qvaluevector.h>

#include <k3bmsf.h>

namespace K3bCdDevice
{

  class Index
    {
    public:
      /**
       * Invalid index
       */
      Index()
	: m_num(-1) {
      }

      Index( int num, const K3b::Msf& pos )
	: m_position(pos),
	m_num(num) {
      }

      /**
       * Startposition
       */
      const K3b::Msf& position() const { return m_position; }

      /**
       * Index number
       */
      int number() const { return m_num; }

    private:
      K3b::Msf m_position;
      int m_num;
    };


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
      DVD,
      UNKNOWN
    };

    Track();
    Track( const Track& );
    Track( const K3b::Msf& firstSector, 
	   const K3b::Msf& lastSector, 
	   int type, 
	   int mode = UNKNOWN );
    Track& operator=( const Track& );

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

    bool recordedIncremental() const { return m_preEmphasis; }
    bool recordedUninterrupted() const { return !recordedIncremental(); }

    const QCString& isrc() const { return m_isrc; }
    void setIsrc( const QCString& s ) { m_isrc = s; }

    const K3b::Msf& firstSector() const { return m_firstSector; }
    const K3b::Msf& lastSector() const { return m_lastSector; }
    K3b::Msf length() const;

    /**
     * This takes index0 into account
     */
    K3b::Msf realAudioLength() const;

    /**
     * 0 if unknown
     */
    int session() const { return m_session; }

    /**
     * @return number of indices. This does not include index 0.
     */
    int indexCount() const;

    /**
     * @return starting sector (lba) of index @p i or -1 if no index i
     *
     * We use long instead of Msf here to be able to return -1
     */
    long index( int i, bool absolute = false ) const;

    /**
     * Returns the lba value of the position of index 0 if it is there.
     */
    long index0() const;

    const QValueVector<long>& indices() const { return m_indices; }

  private:
    K3b::Msf m_firstSector;
    K3b::Msf m_lastSector;

    int m_type;
    int m_mode;
    bool m_copyPermitted;
    bool m_preEmphasis;

    int m_session;

    QValueVector<long> m_indices;

    QCString m_isrc;
  };
}

typedef K3bCdDevice::Track K3bTrack;

#endif
