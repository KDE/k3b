/***************************************************************************
                          k3bmp3track.h  -  description
                             -------------------
    begin                : Thu Jun 14 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BMP3TRACK_H
#define K3BMP3TRACK_H


#include "k3baudiotrack.h"

/**
  *@author Sebastian Trueg
  */

class K3bMp3Track : public K3bAudioTrack
{
 public:
  K3bMp3Track( QList<K3bAudioTrack>* parent, const QString& filename );
  ~K3bMp3Track();

  QString bufferFile() const { return m_bufferFile; }
  void setBufferFile( const QString& file ) { m_bufferFile = file; }

 private:
  /** read info from ID3 Tag and calculate length **/
  void readTrackInfo( const QString& fileName );
  //  bool findFrameHeader( unsigned int );
  bool mp3HeaderCheck(unsigned int header);

  int mp3Padding(unsigned int header);
  int mp3SampleRate(unsigned int header);
  int mp3LayerNumber(unsigned int header);
  int mp3Bitrate(unsigned int header);
  int mp3VersionNumber(unsigned int header);
  bool mp3Protection(unsigned int header);

  QString m_bufferFile;
};

#endif
