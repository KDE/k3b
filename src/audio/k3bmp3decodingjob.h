/***************************************************************************
                          k3bmp3decodingjob.h  -  description
                             -------------------
    begin                : Fri May 4 2001
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

#ifndef K3BMP3DECODINGJOB_H
#define K3BMP3DECODINGJOB_H

#include "../k3bjob.h"

#include <kprocess.h>

class QString;

/**
  *@author Sebastian Trueg
  */

/**
 * K3bJob to decode a single MP3 file to a wav file.
 **/
class K3bMp3DecodingJob : public K3bJob  {

	Q_OBJECT

public: 
	K3bMp3DecodingJob( const QString& fileName );
	~K3bMp3DecodingJob() {}

	bool setSourceFile( const QString& fileName );
	
	QString decodedFile() const { return m_decodedFile; }
	
public slots:
	void start();
	void cancel();
	
protected slots:
	void slotParseMpg123Output( KProcess*, char* output, int len );
	void slotMpg123Finished();
	
private:
	KProcess m_process;
	QString m_sourceFile;
	QString m_decodedFile;
	bool m_fileDecodingSuccessful;
};

#endif
