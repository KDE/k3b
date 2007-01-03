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

#ifndef _K3B_VIDEODVD_RIPPING_DIALOG_H_
#define _K3B_VIDEODVD_RIPPING_DIALOG_H_

#include <k3binteractiondialog.h>
#include <k3bvideodvd.h>
#include "k3bvideodvdrippingjob.h"

#include <qvaluelist.h>
#include <qmap.h>


class K3bVideoDVDRippingWidget;
class QCheckListItem;

class K3bVideoDVDRippingDialog : public K3bInteractionDialog
{
  Q_OBJECT

 public: 
  K3bVideoDVDRippingDialog( const K3bVideoDVD::VideoDVD& dvd, 
			    const QValueList<int>& titles,
			    QWidget *parent = 0, const char *name = 0 );
  ~K3bVideoDVDRippingDialog();

  void setBaseDir( const QString& path );

  enum FileNamingPattern {
    PATTERN_TITLE_NUMBER         = 't',
    PATTERN_VOLUME_ID            = 'i',
    PATTERN_BEAUTIFIED_VOLUME_ID = 'b',
    PATTERN_LANGUAGE_CODE        = 'l',
    PATTERN_LANGUAGE_NAME        = 'n',
    PATTERN_AUDIO_FORMAT         = 'a',
    PATTERN_AUDIO_CHANNELS       = 'c',
    PATTERN_ORIG_VIDEO_SIZE      = 'v',
    PATTERN_VIDEO_SIZE           = 's',
    PATTERN_ASPECT_RATIO         = 'r',
    PATTERN_CURRENT_DATE         = 'd'
  };

 private slots:
  void slotStartClicked();
  void slotUpdateFilenames();
  void slotUpdateFilesizes();
  void slotUpdateVideoSizes();

 private:
  void populateTitleView( const QValueList<int>& titles );

  QString createFilename( const K3bVideoDVDRippingJob::TitleRipInfo& info, const QString& pattern ) const;

  void loadK3bDefaults();
  void loadUserDefaults( KConfigBase* );
  void saveUserDefaults( KConfigBase* );

  K3bVideoDVDRippingWidget* m_w;

  K3bVideoDVD::VideoDVD m_dvd;
  QMap<QCheckListItem*, K3bVideoDVDRippingJob::TitleRipInfo> m_titleRipInfos;

  class AudioStreamViewItem;

  class Private;
  Private* d;
};

#endif
