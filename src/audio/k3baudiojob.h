/***************************************************************************
                          k3baudiojob.h  -  description
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

#ifndef K3BAUDIOJOB_H
#define K3BAUDIOJOB_H

#include "../k3bjob.h"

class K3bAudioDoc;
class K3bAudioTrack;
class K3bMp3DecodingJob;
class QString;


#include <kprocess.h>


/**
  *@author Sebastian Trueg
  */

class K3bAudioJob : public K3bJob  {

	Q_OBJECT

public:
	K3bAudioJob( K3bAudioDoc* doc );
	~K3bAudioJob();

	K3bAudioDoc* doc() { return m_doc; }
	
public slots:
	void start();
	void cancel();
	
protected slots:
	void slotParseCdrecordOutput( KProcess*, char*, int );
	void slotParseCdrdaoOutput( KProcess*, char* output, int len );
	void slotCdrecordFinished();
	void slotCdrdaoFinished();
	void slotMp3JobFinished();
	void slotEmitProgress( int trackMade, int TrackSize );
	
private:
	void decodeNextFile();
	void startWriting();
	
	KProcess m_process;
	K3bAudioDoc* m_doc;
	K3bMp3DecodingJob* m_mp3Job;
	K3bAudioTrack* m_currentProcessedTrack;
	bool firstTrack;
	QString m_tocFile;
	int m_iNumTracksAlreadyWritten;
	int m_iNumFilesToDecode;
	int m_iNumFilesAlreadyDecoded;
		
signals:
	void writingLeadOut();
	void bufferStatus( int );
};

#endif
