/***************************************************************************
                          k3bcdinfo.h  -  description
                             -------------------
    begin                : Sun Apr 22 2001
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

#ifndef K3BCDINFO_H
#define K3BCDINFO_H

#include <klistview.h>

class KAction;
class K3bDevice;
class KProcess;
class QTimer;



/**
  *@author Sebastian Trueg
  */
class K3bCdInfo : public KListView
{
 Q_OBJECT

 public: 
  K3bCdInfo( QWidget* parent = 0, const char* name = 0 );
  ~K3bCdInfo();

  KAction* refreshAction() const { return m_actionRefresh; }

 public slots:
  void setDevice( K3bDevice* );

 private slots:
  void slotTestDevice();
  void slotRefresh();
  void slotParseCdrdaoOutput( KProcess*, char*, int );
  void slotParseCdrecordOutput( KProcess*, char*, int );
  void slotCdrdaoFinished();
  void slotCdrecordFinished();

 private:
  int tries;
  KAction* m_actionRefresh;
  K3bDevice* m_device;
  KProcess* m_process;
  QTimer* m_infoTimer;

  class PrivateCDInfo;
  PrivateCDInfo* m_cdinfo;

  void updateView();
};

class K3bCdInfo::PrivateCDInfo
{
 public:
  bool cdrw;
  QString medium;
  bool empty;
  QString size;
  bool appendable;
  int sessions;
  QString tocType;

  bool cdrw_valid;
  bool medium_valid;
  bool empty_valid;
  bool size_valid;
  bool appendable_valid;
  bool sessions_valid;
  bool tocType_valid;

  void reset()
    {
      cdrw_valid =
	medium_valid =
	empty_valid =
	size_valid =
	appendable_valid =
	sessions_valid =
	tocType_valid = false;
    }
};

#endif
