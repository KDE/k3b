/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3baudiodecoder.h"

#include <kdebug.h>
#include <kfilemetainfo.h>


class K3bAudioDecoder::Private
{
public:
  Private()
    : metaInfo(0) {
  }

  unsigned long alreadyDecoded;
  K3b::Msf decodingStartPos;

  KFileMetaInfo* metaInfo;
};



K3bAudioDecoder::K3bAudioDecoder( QObject* parent, const char* name )
  : K3bPlugin( parent, name )
{
  d = new Private();
}


K3bAudioDecoder::~K3bAudioDecoder()
{
  delete d->metaInfo;
  delete d;
}


void K3bAudioDecoder::setFilename( const QString& filename )
{
  m_fileName = filename;
  delete d->metaInfo;
  d->metaInfo = 0;
}


bool K3bAudioDecoder::analyseFile()
{
  cleanup();

  return analyseFileInternal();
}


bool K3bAudioDecoder::initDecoder()
{
  cleanup();

  d->alreadyDecoded = 0;
  d->decodingStartPos = 0;

  return initDecoderInternal();
}


int K3bAudioDecoder::decode( char* _data, int maxLen )
{
  unsigned long lengthToDecode = (length() - d->decodingStartPos).audioBytes();

  if( d->alreadyDecoded >= lengthToDecode )
    return 0;

  int len = decodeInternal( _data, maxLen );
  if( len < 0 )
    return -1;

  else if( len == 0 ) {
    // check if we need to pad
    int bytesToPad = lengthToDecode - d->alreadyDecoded;
    if( bytesToPad > 0 ) {
      kdDebug() << "(K3bAudioDecoder) track length: " << lengthToDecode
		<< "; decoded module data: " << d->alreadyDecoded
		<< "; we need to pad " << bytesToPad << " bytes." << endl;

      ::memset( _data, 0, maxLen );
      if( maxLen < bytesToPad )
	bytesToPad = maxLen;

      d->alreadyDecoded += bytesToPad;
      return bytesToPad;
    }
    else {
      kdDebug() << "(K3bAudioDecoder) decoded " << d->alreadyDecoded << " bytes." << endl;
      return 0;
    }
  }
  else {
    // check if we decoded too much
    if( d->alreadyDecoded + len > lengthToDecode ) {
      kdDebug() << "(K3bAudioDecoder) we decoded too much. Cutting output." << endl;
      len = length().audioBytes() - d->alreadyDecoded;
    }

    d->alreadyDecoded += len;
    return len;
  }
}


bool K3bAudioDecoder::seek( const K3b::Msf& pos )
{
  d->decodingStartPos = pos;
  d->alreadyDecoded = 0;
  return seekInternal( pos );
}


void K3bAudioDecoder::cleanup()
{
}


QString K3bAudioDecoder::metaInfo( const QString& tag )
{
  if( !d->metaInfo )
    d->metaInfo = new KFileMetaInfo( filename() );

  if( d->metaInfo->isValid() ) {
    KFileMetaInfoItem item = d->metaInfo->item( tag );
    if( item.isValid() )
      return item.string();
  }

  return QString::null;
}


#include "k3baudiodecoder.moc"
