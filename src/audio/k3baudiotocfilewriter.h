/* 
 *
 * $Id: $
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

#ifndef K3B_AUDIO_TOCFILE_WRITER_H
#define K3B_AUDIO_TOCFILE_WRITER_H

class K3bAudioDoc;
class K3bAudioTrack;
class QString;
class QTextStream;


class K3bAudioTocfileWriter
{
 public:
  static bool writeAudioTocFile( K3bAudioDoc*, const QString& filename );
  static bool writeAudioToc( K3bAudioDoc*, QTextStream& );

 private:
  static void writeCdTextEntries( K3bAudioTrack* track, QTextStream& t );
  static QString prepareForTocFile( const QString& str );
};

#endif
