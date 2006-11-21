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


#ifndef K3B_CDPARANOIA_LIB_H
#define K3B_CDPARANOIA_LIB_H

// from cdda_interface.h
#define CD_FRAMESIZE_RAW 2352


#include <qstring.h>

#include <sys/types.h>
#include "k3b_export.h"

namespace K3bDevice {
  class Device;
  class Toc;
}


/**
 * K3bCdparanoiaLib is a convenience wrapper around libcdda_interface
 * and libcdda_paranoia.
 *
 * It uses four paranoia levels 0-3 which can be set via setParanoiaMode
 * and are used the same way as in cdrdao:
 * \li 0: No checking, data is copied directly from the drive.
 * \li 1: Perform overlapped reading to avoid jitter.
 * \li 2: Like 1 but with additional checks of the read audio data.
 * \li 3: Like 2 but with additional scratch detection and repair.
 *
 * K3bCdparanoiaLib is based on a shared data approach which makes sure
 * that each device can only be opened once. This is necessary since 
 * libcdda_interface opens the device exclusively on most distributions.
 *
 * However, it is perfectly possible to have two instances of K3bCdparanoiaLib
 * accessing the same device at the same time. K3bCdparanoiaLib will take care
 * of the syncing and seeking issues automatically.
 *
 * K3bCdparanoiaLib is thread-safe.
 *
 * Usage:
 * <pre>
 * K3bCdparanoiaLib lib;
 * lib.initParanoia( mydevice );
 * lib.initReading( tracknumber );
 * while( char* data = lib.read() )
 *   dosomethingcoolwithdata( data );
 * </pre>
 */
class LIBK3B_EXPORT K3bCdparanoiaLib
{
 public:
  ~K3bCdparanoiaLib();

  /** default: 1 */
  void setParanoiaMode( int );
  void setNeverSkip( bool b );

  /** default: 5 */
  void setMaxRetries( int );

  /**
   * This will read the Toc and initialize some stuff.
   * It will also call paranoiaInit( const QString& )
   */
  bool initParanoia( K3bDevice::Device* dev );

  /**
   * Use for faster initialization without reading the toc
   */
  bool initParanoia( K3bDevice::Device* dev, const K3bDevice::Toc& );

  void close();

  /**
   * Call this after initParanoia to set the data to rip.
   *
   * Rip all audio tracks.
   */
  bool initReading();

  /**
   * Call this after initParanoia to set the data to rip.
   */
  bool initReading( unsigned int track );

  /**
   * Call this after initParanoia to set the data to rip.
   */
  bool initReading( long startSector, long endSector );

  /**
   * Read data.
   * \param statusCode If not 0 will be set.
   * \param track the tracknumer the data belongs to
   *
   * This method takes care of swapping the byte-order depending on the
   * machine type.
   *
   * \return The read sector data or 0 if all data within the specified range
   *         has been read or an error has occured.
   */
  char* read( int* statusCode = 0, unsigned int* track = 0, bool littleEndian = true );

  /**
   * This only is valid after a call to read()
   */
  int status() const;

  enum Status {
    S_OK,
    S_ERROR
    // to be extended with Jitter and stuff...
  };

  /**
   * Only valid after a call to initParanoia()
   */
  const K3bDevice::Toc& toc() const;

  long rippedDataLength() const;

  /**
   * returns 0 if the cdparanoialib could not
   * be found on the system.
   * Otherwise you have to take care of
   * deleting.
   */
  static K3bCdparanoiaLib* create();

 private:
  void cleanup();

  K3bCdparanoiaLib();
  bool load();

  class Private;
  Private* d;

  static void* s_libInterface;
  static void* s_libParanoia;
  static int s_counter;
};


#endif
