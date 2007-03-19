/* 
 *
 * $Id: sourceheader,v 1.3 2005/01/19 13:03:46 trueg Exp $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_AUDIO_DATA_SOURCE_ITERATOR_H_
#define _K3B_AUDIO_DATA_SOURCE_ITERATOR_H_
#include "k3b_export.h"
class K3bAudioDataSource;
class K3bAudioTrack;
class K3bAudioDoc;


/**
 * This Iterator iterates over the sources in an audio project
 *
 * Be aware that this iterator does not properly update when the doc
 * changes. A manual update can be issued with first(). This is becasue
 * an update would either involve slots (this being a QObject) which is 
 * too much overhead or the AudioDoc would need to have knowledge of all
 * the iterators which is also overhead that would be overkill.
 */
class LIBK3B_EXPORT K3bAudioDataSourceIterator
{
 public:
  /**
   * This will place the iterator on the first source just like first() does.
   */
  explicit K3bAudioDataSourceIterator( K3bAudioDoc* );

  K3bAudioDataSource* current() const;

  bool hasNext() const;

  /**
   * \return the next source or 0 if at end.
   */
  K3bAudioDataSource* next();

  /**
   * Reset the iterator
   */
  K3bAudioDataSource* first();

 private:
  K3bAudioDoc* m_doc;
  K3bAudioTrack* m_currentTrack;
  K3bAudioDataSource* m_currentSource;
};

#endif
