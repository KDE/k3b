/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_AUDIO_DATA_SOURCE_H_
#define _K3B_AUDIO_DATA_SOURCE_H_

#include <k3bmsf.h>

class K3bAudioTrack;
class K3bAudioDoc;


class K3bAudioDataSource 
{
  friend class K3bAudioTrack;

 public:
  K3bAudioDataSource();
  virtual ~K3bAudioDataSource();

  virtual K3b::Msf length() const = 0;

  /** 
   * @return The raw size in pcm samples (16bit, 44800 kHz, stereo) 
   */
  KIO::filesize_t size() const { return length().audioBytes(); }

  virtual bool seek( const K3b::Msf& ) = 0;

  /**
   * Read data from the source.
   */
  virtual int read( char* data, unsigned int max ) = 0;

  /**
   * Type of the data in readable form.
   */
  virtual QString type() const = 0;

  /**
   * The source in readable form (this is the filename for files)
   */
  virtual QString sourceComment() const = 0;

  /**
   * Used in case an error occured. For now this is used if the
   * decoder was not able to decode an audiofile
   */
  virtual bool isValid() const { return true; }

  /**
   * For internal reasons this does never change.
   */
  K3bAudioDoc* doc() const;
  K3bAudioTrack* track() const { return m_track; }

  K3bAudioDataSource* prev() const { return m_prev; }
  K3bAudioDataSource* next() const { return m_next; }

  K3bAudioDataSource* take();

  void moveAfter( K3bAudioDataSource* track );
  void moveAhead( K3bAudioDataSource* track );

  /**
   * Create a copy of this source which is not part of a list
   * but has the same doc.
   */
  virtual K3bAudioDataSource* copy() const = 0;

  /**
   * Split the source at position pos and return the splitted source
   * on success.
   * The new source will be moved after this source.
   */
  virtual K3bAudioDataSource* split( const K3b::Msf& pos ) = 0;

 protected:
  /**
   * Informs the parent track about changes.
   */
  void emitChange();

 private:
  K3bAudioDoc* m_doc;
  K3bAudioTrack* m_track;
  K3bAudioDataSource* m_prev;
  K3bAudioDataSource* m_next;
};

#endif
