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


#ifndef K3BISOIMAGEWRITINGDIALOG_H
#define K3BISOIMAGEWRITINGDIALOG_H

#include <k3binteractiondialog.h>


class QCheckBox;
class K3bWriterSelectionWidget;
class QLabel;
class K3bIso9660ImageWritingJob;
class KURL;
class K3bMd5Job;
class KActiveLabel;
class KProgress;
class K3bDataModeWidget;
class K3bWritingModeWidget;
class KURLRequester;


/**
  *@author Sebastian Trueg
  */
class K3bIsoImageWritingDialog : public K3bInteractionDialog
{
  Q_OBJECT

 public: 
  K3bIsoImageWritingDialog( bool dvd, QWidget* = 0, const char* = 0, bool = true );
  ~K3bIsoImageWritingDialog();

  void setImage( const KURL& url );

 protected slots:
  void slotStartClicked();
  void updateImageSize( const QString& );
  void slotWriterChanged();
  void slotMd5JobFinished( bool );

  void slotLoadUserDefaults();
  void slotSaveUserDefaults();
  void slotLoadK3bDefaults();

 private:
  void setupGui();

  K3bIso9660ImageWritingJob* m_job;
  K3bMd5Job* m_md5Job;

  K3bWriterSelectionWidget* m_writerSelectionWidget;
  QCheckBox* m_checkDummy;
  QCheckBox* m_checkBurnProof;
  QCheckBox* m_checkNoFix;
  K3bDataModeWidget* m_dataModeWidget;
  K3bWritingModeWidget* m_writingModeWidget;

  QLabel*    m_labelImageSize;
  KURLRequester* m_editImagePath;
  KProgress* m_md5ProgressWidget;
  KActiveLabel* m_md5Label;

  QWidget* m_isoInfoWidget;
  QLabel* m_labelIsoId;
  QLabel* m_labelIsoSystemId;
  QLabel* m_labelIsoVolumeId;
  QLabel* m_labelIsoVolumeSetId;
  QLabel* m_labelIsoPublisherId;
  QLabel* m_labelIsoPreparerId;
  QLabel* m_labelIsoApplicationId;

  QLabel* m_generalInfoLabel;

  bool m_bIsoImage;
  QString m_lastCheckedFile;

  bool m_dvd;
};

#endif
