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
class QTextStream;
class K3bDirItem;

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
  void slotCollectOutput( KProcess*, char*, int );
  void fetchMultiSessionInfo();
  void fetchIsoSize();
  void slotMsInfoFetched();
  void slotIsoSizeFetched();
  void writeImage();
  void writeCD();
  void cancelAll();
		
 private:
  void writePathSpecForDir( K3bDirItem* dirItem, QTextStream& stream );
  QString escapeGraftPoint( const QString& str );

  K3bDataDoc* m_doc;
  QString m_pathSpecFile;
  QString m_rrHideFile;
  QString m_jolietHideFile;
  KProcess* m_process;

  bool m_imageFinished;

  QString m_isoSize;
  QString m_msInfo;
  QString m_collectedOutput;
		
  bool addMkisofsParameters();
  bool writePathSpec( const QString& filename );
  bool writeRRHideFile( const QString& filename );
};

#endif
