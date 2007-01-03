/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_VIDEODVD_RIPPING_TITLE_LISTVIEW_H_
#define _K3B_VIDEODVD_RIPPING_TITLE_LISTVIEW_H_

#include <k3blistview.h>
#include <k3bvideodvd.h>
#include <k3bmedium.h>

#include <qvaluevector.h>


class K3bVideoDVDRippingPreview;
class QHideEvent;

class K3bVideoDVDRippingTitleListView : public K3bListView
{
  Q_OBJECT

 public:
  K3bVideoDVDRippingTitleListView( QWidget* parent );
  ~K3bVideoDVDRippingTitleListView();

  void setVideoDVD( const K3bVideoDVD::VideoDVD& dvd );

 private slots:
  void slotPreviewDone( bool );

 private:
  void hideEvent( QHideEvent* );

  class TitleViewItem;
  class TitleToolTip;

  TitleToolTip* m_toolTip;

  QValueVector<TitleViewItem*> m_itemMap;
  K3bVideoDVDRippingPreview* m_previewGen;
  unsigned int m_currentPreviewTitle;

  K3bVideoDVD::VideoDVD m_dvd;
  K3bMedium m_medium;
};

#endif
