/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_IMAGE_H_
#define _K3B_IMAGE_H_

#include <k3btoc.h>
#include <k3bcdtext.h>
#include <k3bglobals.h>

#include <kio/global.h>

#include <qdom.h>



/**
 * This class represents a K3b image file (with a tar backend)
 *
 * K3bImage splits tracks bigger than 1GB into 1GB chunks to avoid problems
 * with file sizes which break the 32 bit barrier.
 */
class K3bImage
{
 public:
  K3bImage();
  K3bImage( const QString& filename, int mode = IO_ReadOnly );
  ~K3bImage();

  const QString& filename() const { return m_filename; }

  /**
   * Open an image file for writing or reading.
   *
   * \param mode has to be either IO_ReadOnly or IO_WriteOnly.
   *             If IO_WriteOnly a new image will be created. For now images
   *             can not be modified.
   */
  bool open( const QString& filename, int mode = IO_ReadOnly );
  bool isOpen() const;

  /**
   * Closes the image and writes back the toc.
   */
  void close();

  /**
   * The table of contents of the image.
   */
  const K3bDevice::Toc& toc() const { return m_toc; }
  const K3bDevice::CdText& cdText() const { return m_cdText; }

  /**
   * \li 2352 for Audio tracks or raw data
   * \li 2048 mode1
   * \li 2056 mode2 form1
   * \li 2332 mode2 form2
   *
   * \see K3b::SectorSize
   */
  K3b::SectorSize sectorSize( unsigned int track ) const;

  /**
   * \return A read-only device to access the track data. Ownership of the device is being transferred
   *         to the caller, who will have to delete it.
   */
  QIODevice* readTrack( unsigned int track );

  /**
   * Clear the image. Remove all data.
   *
   * Does nothing if the image has been opened read-only.
   */
  void clear();

  /**
   * This will initialize a new toc.
   * Afterwards the track data has to be written for all tracks.
   *
   * Does nothing if the image has been opened read-only.
   *
   * Be aware that this will not delete previously written track data!
   */
  void setToc( const K3bDevice::Toc& toc );

  /**
   * Does nothing if the image has been opened read-only.
   */
  void setCdText( const K3bDevice::CdText& );

  /**
   * Prepare the writing of a track's data. Write the data using 
   * writeTrackData(). It is not possible to start a new track until
   * all data has been written.
   *
   * \param track Number of the track to be written. Starts at 1.
   * \size The sector size used for this track's data.
   */
  bool writeTrack( unsigned int track, K3b::SectorSize size );
  bool writeTrackData( const char*, unsigned int len );

  /**
   * FIXME: not implemented yet.
   *
   * \return a device to write the track data to or null if \ref track
   *         is out of range or the image has been opened ReadOnly.
   *         Ownership of the device is being transferred
   *         to the caller, who will have to delete it.
   */
  QIODevice* writingDevice();

 private:
  bool readToc();
  bool readFilename( QDomElement& e, 
		     QString& filename, 
		     KIO::filesize_t& startOffset, 
		     KIO::filesize_t& fileSize );
  bool saveToc();
  bool saveCdText();

  QString m_filename;
  K3bDevice::Toc m_toc;
  K3bDevice::CdText m_cdText;

  class Private;
  Private* d;
};

#endif
