/***************************************************************************
                          k3bdatajob.h  -  description
                             -------------------
    begin                : Tue May 15 2001
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

#ifndef K3BDATAJOB_H
#define K3BDATAJOB_H

#include "../k3bjob.h"

#include <kprocess.h>

class K3bDataDoc;
class QString;


/**
  *@author Sebastian Trueg
  */

class K3bDataJob : public K3bBurnJob
{
  Q_OBJECT
	
 public:
  K3bDataJob( K3bDataDoc* );
  ~K3bDataJob();
	
  K3bDoc* doc() const;
  K3bDevice* writer() const;
		
 public slots:
  void cancel();
  void start();

 protected slots:
  void slotParseCdrecordOutput( KProcess*, char*, int );
  void slotParseMkisofsOutput( KProcess*, char*, int );
  void slotMkisofsFinished();
  void slotCdrecordFinished();
  void slotParseMkisofsSize(KProcess*, char*, int);
  void slotParseMsInfo(KProcess*, char*, int);
		
 private:
  K3bDataDoc* m_doc;
  QString m_pathSpecFile;
  KProcess* m_process;

  bool m_imageFinished;

  QString m_isoSize;
  QString m_msInfo;
		
  void writeImage();
  void writeCD();
  bool addMkisofsParameters();
  bool writePathSpec( const QString& filename );
  void cancelAll();
};

#endif
