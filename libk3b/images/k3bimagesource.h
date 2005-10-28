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

#ifndef _K3B_IMAGE_SOURCE_H_
#define _K3B_IMAGE_SOURCE_H_

#include <k3bjob.h>
#include <k3btoc.h>
#include <k3btrack.h>
#include <k3bcdtext.h>
#include <k3bglobals.h>

#include <k3b_export.h>

#include <kio/global.h>


/**
 * Base class for jobs that create a CD or DVD image.
 *
 * Reimplement start() and cancel() from K3bJob and determineToc in case
 * calculating the toc takes some time.
 *
 * It is highly recommended to stream data into the fd using a thread different
 * from the GUI thread to make burning of the data as fast as possible. That way
 * the data stream does not depend on the main event loop and is not interrupted
 * by events like GUI updates.
 *
 * There is generally no need for an image source to emit progress information.
 * This is done by the image sink using this source.
 *
 * An image source is intended to provide big endian, 16bit, stereo audio samples
 * for audio tracks.
 */
class LIBK3B_EXPORT K3bImageSource : public K3bJob
{
  Q_OBJECT

 public:
  virtual ~K3bImageSource();

  /**
   * The file descriptor the image should write the data to.
   */
  virtual int fdToWriteTo() const { return m_fd; }

  virtual int session() const { return m_session; }

  /**
   * <b>Call determineToc() before using this method.</b>
   *
   * Retrieve the toc of this image.
   *
   * Every track is supposed to have a sector size based on the track type and
   * it's data mode as follows:
   *
   * <table>
   *  <tr><th>Track type</th><th>Data mode</th><th>Sector size</th></tr>
   *  <tr><td>Audio</td><td>-</td><td>2352</td></tr>
   *  <tr><td>Data</td><td>MODE1</td><td>2048 (user data only)</td></tr>
   *  <tr><td>Data</td><td>XA_FORM1</td><td>2048 (user data only) or 2056 (user data + subheader)</td></tr>
   *  <tr><td>Data</td><td>XA_FORM2</td><td>2324 (user data only) or 2332 (user data + subheader)</td></tr>
   * </table>
   *
   * \see K3bDevice::Track::TrackType
   * \see K3bDevice::Track::DataMode
   * \see K3b::SectorSize
   * \see sectorSize()
   */
  virtual K3bDevice::Toc toc() const { return m_toc; }

  /**
   * <b>Call determineToc() before using this method.</b>
   *
   * \return The sector size used for the track's data.
   *
   * \see toc()
   */
  virtual K3b::SectorSize sectorSize( unsigned int track ) const;

  /**
   * <b>Call determineToc() before using this method.</b>
   *
   * The CD Text object in case the image source provides an audio CD
   * image with CD Text.
   */
  virtual K3bDevice::CdText cdText() const { return m_cdText; }

  /**
   * <b>Call determineToc() before using this method.</b>
   *
   * Read from the source.
   *
   * This is a blocking call. Depending on the implementation it might
   * block the GUI for some time (if no data is ready yet).
   *
   * Using a K3bImageSource through the read() interface means to use a K3bJob
   * in a unusual fashion. Instead of waiting for the finished signal one simply
   * stops when the read() method returns a value <= 0.
   *
   * The default implementation simply returns -1.
   */
  virtual long read( char* data, long maxLen );

  /**
   * <b>Call determineToc() before using this method.</b>
   *
   * \return The size of the data provided for the track. This is based on the track's length
   *         and it's sector size.
   */
  KIO::filesize_t trackSize( unsigned int track ) const;

  /**
   * <b>Call determineToc() before using this method.</b>
   *
   * \return The size of the data provided for the toc. This is based on the tracks' length
   *         and their sector sizes.
   */
  KIO::filesize_t tocSize() const;

 signals:
  void tocReady( bool success );

 public slots:
  /**
   * Determine the Table of contents of the resulting image.
   * Emit tocReady() when done.
   * 
   * The default implementation simply emits the signal through a timer.
   */
  virtual void determineToc();

  /**
   * Set the session to write. Defaults to 1.
   */
  virtual void setSession( int i ) { m_session = i; }

  /**
   * When writing to an fd this will start the streaming. Otherwise start()
   * may do some initialization.
   *
   * Always call start() even if you intend to use read()
   */
  virtual void start() = 0;

  /**
   * Start the source and set the session to write in one call.
   */
  void start( int session );

  /**
   * Be aware that this only makes sense before starting the job.
   *
   * Set the fd to -1 (the default) to make use of the read method.
   *
   * One should also take into accout that letting the source write to an fd
   * might result in a blocking write call which means that the image sink
   * has to run in another thread or process. Otherwise the application will hang.
   */
  virtual void writeToFd( int fd );

 protected:
  K3bImageSource( K3bJobHandler* hdl, QObject* parent = 0 );

  /**
   * Simply uses OS IO to write to the fd set via writeToFd()
   */
  long writeToFd( char* data, long len );

  /**
   * Use this to set the size when reimplementing determineToc() instead of toc().
   */
  void setToc( const K3bDevice::Toc& s ) { m_toc = s; }
  void setCdText( const K3bDevice::CdText& t ) { m_cdText = t; }

  /**
   * Subclasses may use this to set the tracks' sector sizes in case sectorSize()
   * does not get reimplemented.
   */
  void setSectorSize( unsigned int track, K3b::SectorSize size );

 private slots:
  void slotEmitTocReady();

 private:
  int m_session;
  int m_fd;
  K3bDevice::Toc m_toc;
  K3bDevice::CdText m_cdText;

  class Private;
  Private* d;
};

#endif
