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

#include "k3baudioconverterviewitem.h"

#include <k3baudiodecoder.h>

#include <klocale.h>

#include <qthread.h>
#include <qfont.h>


class K3bAudioConverterViewItem::LengthThread : public QThread
{
public:
  LengthThread( K3bAudioConverterViewItem* item ) 
    : m_item(item) {
  }

protected:
  void run() {
    if( m_item->decoder()->analyseFile() )
      m_item->setLength( m_item->decoder()->length() );
    else
      m_item->setLength( 0 );
  }

private:
  K3bAudioConverterViewItem* m_item;
};


K3bAudioConverterViewItem::K3bAudioConverterViewItem( const QString& url, 
						      K3bAudioDecoder* dec, 
						      QListView* parent, 
						      QListViewItem* after )
  : K3bListViewItem( parent, after ),
    m_decoder(dec),
    m_lengthReady(false),
    m_path(url)
{
  setText( 0, " " + url.section( '/', -1 ) + " " );
  setText( 1, " " + dec->fileType() + " " );
  setText( 2, i18n("calculating...") );

  // italic type
  QFont f(listView()->font());
  f.setItalic( true );
  setFont( 1, f );

  dec->setFilename( url );

  m_lengthThread = new LengthThread( this );
  m_lengthThread->start();
}


K3bAudioConverterViewItem::~K3bAudioConverterViewItem()
{
  delete m_lengthThread;
}


void K3bAudioConverterViewItem::setLength( const K3b::Msf& len )
{
  setText( 2, " " + len.toString() + " " );
  setText( 1, " " + m_decoder->fileType() + " " ); // might have changed (mp3)
  m_lengthReady = true;
}

