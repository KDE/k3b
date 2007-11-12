/* 
 *
 * $Id$
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */



#ifndef K3BTRACK_H
#define K3BTRACK_H

#include <q3cstring.h>
#include <q3valuevector.h>

#include <k3bmsf.h>
#include "k3bdevice_export.h"

namespace K3bDevice
{
  class LIBK3BDEVICE_EXPORT Track
  {
    friend class Device;

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
    void setCopyPermitted( bool b ) { m_copyPermitted = b; }

    /**
     * Only valid for audio tracks
     */
    bool preEmphasis() const { return m_preEmphasis; }
    void setPreEmphasis( bool b ) { m_preEmphasis = b; }

    bool recordedIncremental() const { return m_preEmphasis; }
    bool recordedUninterrupted() const { return !recordedIncremental(); }

    const Q3CString& isrc() const { return m_isrc; }
    void setIsrc( const Q3CString& s ) { m_isrc = s; }

    const K3b::Msf& firstSector() const { return m_firstSector; }
    const K3b::Msf& lastSector() const { return m_lastSector; }
    void setFirstSector( const K3b::Msf& msf ) { m_firstSector = msf; }
    void setLastSector( const K3b::Msf& msf ) { m_lastSector = msf; }

    const K3b::Msf& nextWritableAddress() const { return m_nextWritableAddress; }
    const K3b::Msf& freeBlocks() const { return m_freeBlocks; }

    K3b::Msf length() const;

    /**
     * This takes index0 into account
     */
    K3b::Msf realAudioLength() const;

    /**
     * 0 if unknown
     */
    int session() const { return m_session; }
    void setSession( int s ) { m_session = s; }

    /**
     * @return number of indices. This does not include index 0.
     */
    int indexCount() const;

    /**
     * Returns the index relative to the track's start.
     * If it is zero there is no index0.
     */
    const K3b::Msf& index0() const { return m_index0; }

    /**
     * Set the track's index0 value.
     * @param msf offset relative to track start.
     */
    void setIndex0( const K3b::Msf& msf );

    /**
     * All indices. Normally this list is empty as indices are rarely used.
     * Starts with index 2 (since index 1 are all other sectors FIXME)
     */
    const Q3ValueVector<K3b::Msf>& indices() const { return m_indices; }

    bool operator==( const Track& ) const;
    bool operator!=( const Track& ) const;

  private:
    K3b::Msf m_firstSector;
    K3b::Msf m_lastSector;
    K3b::Msf m_index0;

    K3b::Msf m_nextWritableAddress;
    K3b::Msf m_freeBlocks;

    int m_type;
    int m_mode;
    bool m_copyPermitted;
    bool m_preEmphasis;

    int m_session;

    Q3ValueVector<K3b::Msf> m_indices;

    Q3CString m_isrc;
  };
}

typedef K3bDevice::Track K3bTrack;

#endif
