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
#include "k3b_export.h"
class K3bAudioTrack;
class K3bAudioDoc;


/**
 * An AudioDataSource has an original length which represents the maximum amount of audio
 * sectors this source can provide (in special cases this is not true, see K3bAudioZeroData).
 *
 * It is possible to just use a portion of that data by changing the startOffset and endOffset.
 * This will change the actual length of the data provided by this source through the read method.
 *
 * Sources are part of a list which can be traversed via the prev() and next() methods. This list 
 * is part of a K3bAudioTrack which in turn is part of a list which is owned by a K3bAudioDoc.
 *
 * The list may be modified with the take(), moveAfter(), and moveAhead() methods. The source takes
 * care of fixing the list and notifying the track about the change (It is also possible to move sources
 * from one track to the other).
 *
 * When a source is deleted it automatically removes itself from it's list.
 */
class LIBK3B_EXPORT K3bAudioDataSource 
{
  friend class K3bAudioTrack;

 public:
  K3bAudioDataSource();

  /**
   * Create en identical copy except that the copy will not be in any list.
   */
  K3bAudioDataSource( const K3bAudioDataSource& );
  virtual ~K3bAudioDataSource();

  /**
   * The original length of the source is the maximum data which is available
   * when startOffset is 0 this is the max for endOffset
   *
   * Be aware that this may change (see K3bAudioZeroData)
   */
  virtual K3b::Msf originalLength() const = 0;

  /**
   * The default implementation returns the originalLength modified by startOffset and endOffset
   */
  virtual K3b::Msf length() const;

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
   * Used in case an error occurred. For now this is used if the
   * decoder was not able to decode an audiofile
   */
  virtual bool isValid() const { return true; }

  /**
   * The doc the source is currently a part of or null.
   */
  K3bAudioDoc* doc() const;
  K3bAudioTrack* track() const { return m_track; }

  K3bAudioDataSource* prev() const { return m_prev; }
  K3bAudioDataSource* next() const { return m_next; }

  K3bAudioDataSource* take();

  void moveAfter( K3bAudioDataSource* track );
  void moveAhead( K3bAudioDataSource* track );

  /**
   * Set the start offset from the beginning of the source's originalLength.
   */
  virtual void setStartOffset( const K3b::Msf& );

  /**
   * Set the end offset from the beginning of the file. The endOffset sector
   * is not included in the data.
   * The maximum value is originalLength() which means to use all data.
   * 0 means the same as originalLength().
   * This has to be bigger than the start offset.
   */
  virtual void setEndOffset( const K3b::Msf& );

  virtual const K3b::Msf& startOffset() const { return m_startOffset; }

  /**
   * The end offset. It is the first sector not included in the data.
   * If 0 the last sector is determined by the originalLength
   */
  virtual const K3b::Msf& endOffset() const { return m_endOffset; }

  /**
   * Get the last used sector in the source.
   * The default implementation uses originalLength() and endOffset()
   */
  virtual K3b::Msf lastSector() const;

  /**
   * Create a copy of this source which is not part of a list
   */
  virtual K3bAudioDataSource* copy() const = 0;

  /**
   * Split the source at position pos and return the splitted source
   * on success.
   * The new source will be moved after this source.
   *
   * The default implementation uses copy() to create a new source instance
   */
  virtual K3bAudioDataSource* split( const K3b::Msf& pos );

 protected:
  /**
   * Informs the parent track about changes.
   */
  void emitChange();

 private:
  void fixupOffsets();

  K3bAudioTrack* m_track;
  K3bAudioDataSource* m_prev;
  K3bAudioDataSource* m_next;

  K3b::Msf m_startOffset;
  K3b::Msf m_endOffset;
};

#endif
