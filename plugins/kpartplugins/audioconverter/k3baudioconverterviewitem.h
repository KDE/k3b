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

#ifndef _K3B_AUDIO_CONVERTER_VIEWITEM_H_
#define _K3B_AUDIO_CONVERTER_VIEWITEM_H_

#include <k3blistview.h>
#include <k3bmsf.h>



class K3bAudioDecoder;


class K3bAudioConverterViewItem : public K3bListViewItem
{
 public:
  K3bAudioConverterViewItem( const QString& url, K3bAudioDecoder*, QListView* parent, QListViewItem* after );
  ~K3bAudioConverterViewItem();

  bool lengthReady() const { return m_lengthReady; }
  K3bAudioDecoder* decoder() const { return m_decoder; }
  const QString& path() const { return m_path; }

 private:
  void setLength( const K3b::Msf& );

  K3bAudioDecoder* m_decoder;
  bool m_lengthReady;
  QString m_path;

  class LengthThread;
  LengthThread* m_lengthThread;

  friend class LengthThread;
};

#endif
