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


#ifndef _K3B_DISKINFO_H_
#define _K3B_DISKINFO_H_


#include <k3btoc.h>
#include <k3bmsf.h>


#include <qstring.h>


class kdbgstream;


namespace K3bCdDevice
{
  enum State { 
    STATE_UNKNOWN = 256,
    STATE_NO_MEDIA = 0,
    STATE_COMPLETE = 1, 
    STATE_INCOMPLETE = 2,
    STATE_EMPTY = 4
  };

  enum BackGroundFormattingState {
    BG_FORMAT_NONE = 0,
    BG_FORMAT_INCOMPLETE = 1,
    BG_FORMAT_IN_PROGRESS = 2,
    BG_FORMAT_COMPLETE = 3
  };

  /**
   * Defines the different media types as retured by 
   * K3bDevice::mediaType()
   */
  enum MediaType { MEDIA_NONE = 0,
		   MEDIA_DVD_ROM = 1,
		   MEDIA_DVD_R = 2,
		   MEDIA_DVD_R_SEQ = 4,
		   MEDIA_DVD_RAM = 8,
		   MEDIA_DVD_RW = 16,
		   MEDIA_DVD_RW_OVWR = 32,
		   MEDIA_DVD_RW_SEQ = 64,
		   MEDIA_DVD_PLUS_RW = 128,
		   MEDIA_DVD_PLUS_R = 256,
		   MEDIA_DVD_PLUS_R_DL = 4096,
		   MEDIA_CD_ROM = 512,
		   MEDIA_CD_R = 1024,
		   MEDIA_CD_RW = 2048,
		   MEDIA_WRITABLE_CD = MEDIA_CD_R | 
		                       MEDIA_CD_RW,
		   MEDIA_WRITABLE_DVD = MEDIA_DVD_R | 
		                        MEDIA_DVD_R_SEQ | 
		                        MEDIA_DVD_RW |
		                        MEDIA_DVD_RW_OVWR |
		                        MEDIA_DVD_RW_SEQ |
		                        MEDIA_DVD_PLUS_RW |
		                        MEDIA_DVD_PLUS_R |
		                        MEDIA_DVD_PLUS_R_DL,
		   MEDIA_UNKNOWN = 32768
  };

  inline bool isDvdMedia( int mediaType ) {
    return ( mediaType == MEDIA_DVD_ROM || 
	     mediaType == MEDIA_DVD_R || 
	     mediaType == MEDIA_DVD_R_SEQ || 
	     mediaType == MEDIA_DVD_RW || 
	     mediaType == MEDIA_DVD_RW_OVWR || 
	     mediaType == MEDIA_DVD_RW_SEQ || 
	     mediaType == MEDIA_DVD_PLUS_RW || 
	     mediaType == MEDIA_DVD_PLUS_R ||
	     mediaType == MEDIA_DVD_PLUS_R_DL );
  }

  inline bool isRewritableMedia( int mediaType ) {
    return ( mediaType == MEDIA_DVD_RW ||
	     mediaType == MEDIA_DVD_RW_OVWR || 
	     mediaType == MEDIA_DVD_RW_SEQ || 
	     mediaType == MEDIA_DVD_PLUS_RW || 
	     mediaType == MEDIA_CD_RW );
  }


  class CdDevice;

  class DiskInfo
  {
  public:
    DiskInfo();

    enum type { UNKNOWN, NODISC, AUDIO, DATA, MIXED, DVD };

    int mediaType;

    Toc toc;
    QString mediumManufactor;
    QString mediumType;

    bool empty;
    bool cdrw;
    bool appendable;
    bool noDisk;
    bool isVideoDvd;
    bool isVCD;

    K3b::Msf size;
    K3b::Msf remaining;

    int speed;
    int sessions;
    int tocType;

    // iso stuff
    QString isoId;
    QString isoSystemId;
    QString isoVolumeId;
    QString isoVolumeSetId;
    QString isoPublisherId;
    QString isoPreparerId;
    QString isoApplicationId;
    int     isoSize;

    bool valid;

    CdDevice* device;
  };


  /**
   * This class is directly accociated to a strcuture from 
   * the MMC draft READ_DISK_INFO.
   * It also holds some additional data.
   * This class' data will be retrieved by K3bCdDevice::CdDevice.
   *
   * Before using any values one should check diskState != STATE_UNKNOWN or
   * diskState == STATE_NO_MEDIA.
   * That may mean that no disk is in the drive or an error occurred.
   * Writers should never give the STATE_UNKNOWN state. CD-ROM or DVD-ROM
   * drives on the other hand may have trouble determining the state of the disk.
   */
  class NextGenerationDiskInfo
    {
    public:
      NextGenerationDiskInfo();
      ~NextGenerationDiskInfo();

      /**
       * Returnes the state of the disk.
       * See enum State.
       */
      int diskState() const;

      /**
       * Returnes the state of the last session.
       * See enum State.
       */
      int lastSessionState() const;

      /**
       * Returnes the state of the background formatting. This does
       * only make sense for DVD+RW (and MRW which is not yet supported)
       */
      int bgFormatState() const;

      /**
       * returnes true if diskState() == STATE_EMPTY
       */
      bool empty() const;

      /**
       * Is this a rewritable media (e.g. a CD-RW, DVD-RW, or DVD+RW)
       */
      bool rewritable() const;

      /**
       * Is this disk appendable
       * returnes true if diskState() == STATE_INCOMPLETE
       */
      bool appendable() const;

      /**
       * The type of the disk:
       * CD-ROM, CD-R, CD-RW, DVD-ROM, DVD-R(W), DVD+R(W)
       */
      int mediaType() const;

      /**
       * This is the current profile of the drive. That means it may differ
       * from drive to drive.
       * -1 means no info.
       * Mainly interesting for the distiction of DVD-R(W) modes:
       * Sequential and Restricted Overwrite.
       */
      int currentProfile() const { return m_currentProfile; }

      /**
       * Just for easy implementation since there are so many
       * different DVD formats.
       */
      bool isDvdMedia() const;

      /**
       * The number of sessions on the disk.
       * This does not include any leadout or the last empty session
       * on a DVD+-R(W)
       */
      int numSessions() const;

      int numTracks() const;

      /**
       * Number of layers on a DVD media. For CD media this is always 1.
       */
      int numLayers() const;

      /**
       * Does only make sense for appendable disks.
       */
      K3b::Msf remainingSize() const;

      /**
       * The capacity of the disk.
       * For complete disks this may be the same as size()
       */
      K3b::Msf capacity() const;

      /**
       * Returns the size of the used part.
       * For appendable media this equals capacity() - remainingSize()
       */
      K3b::Msf size() const;

      /**
       * Returns the size of Data area in the first layer for DL DVD media.
       * Otherwise size() is returned.
       *
       * This does not specify the layer change sector as the data area on DVD media does
       * not start at sector 0 but at sector 30000h or 31000h depending on the type.
       */
      K3b::Msf firstLayerSize() const;

      void debug() const;

    private:
      int m_mediaType;
      int m_currentProfile;

      int m_diskState;
      int m_lastSessionState;
      int m_bgFormatState;
      int m_numSessions;
      int m_numTracks;
      int m_numLayers;  // only for DVD media
      int m_rewritable;

      K3b::Msf m_capacity;
      K3b::Msf m_usedCapacity;
      K3b::Msf m_firstLayerSize;

      friend class CdDevice;
    };

  //  kdbgstream& operator<<( kdbgstream& s, const NextGenerationDiskInfo& ngInf );
}


typedef K3bCdDevice::DiskInfo K3bDiskInfo;

#endif
