/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_CDIMAGEWRITINGDIALOG_H_
#define _K3B_CDIMAGEWRITINGDIALOG_H_

#include <k3binteractiondialog.h>


class QCheckBox;
class K3bWriterSelectionWidget;
class QLabel;
class KURL;
class KActiveLabel;
class KProgress;
class K3bDataModeWidget;
class K3bWritingModeWidget;
class KURLRequester;
class K3bListView;
class QSpinBox;
class QComboBox;
class K3bIso9660;



/**
  *@author Sebastian Trueg
  */
class K3bCdImageWritingDialog : public K3bInteractionDialog
{
  Q_OBJECT

 public: 
  K3bCdImageWritingDialog( QWidget* = 0, const char* = 0, bool = true );
  ~K3bCdImageWritingDialog();

  void setImage( const KURL& url );

 protected slots:
  void slotStartClicked();

  void slotMd5JobPercent( int );
  void slotMd5JobFinished( bool );
  void slotMd5SumCompare();

  void slotToggleAll();
  void slotUpdateImage( const QString& );

  void slotLoadUserDefaults();
  void slotSaveUserDefaults();
  void slotLoadK3bDefaults();

 protected:
  void calculateMd5Sum( const QString& );

 private:
  enum {
    IMAGE_UNKNOWN,
    IMAGE_ISO,
    IMAGE_CUE_BIN,
    IMAGE_CDRDAO_TOC,
    IMAGE_CDRECORD_CLONE };

  void setupGui();
  void createIso9660InfoItems( K3bIso9660* );
  void createCdrecordCloneItems( const QString&, const QString& );
  void createCueBinItems( const QString&, const QString& );
  int currentImageType();

  K3bWriterSelectionWidget* m_writerSelectionWidget;
  QCheckBox* m_checkDummy;
  QCheckBox* m_checkNoFix;
  QCheckBox* m_checkVerify;
  K3bDataModeWidget* m_dataModeWidget;
  K3bWritingModeWidget* m_writingModeWidget;
  QSpinBox* m_spinCopies;

  KURLRequester* m_editImagePath;
  QComboBox* m_comboImageType;

  K3bListView* m_infoView;

  class Private;
  Private* d;
};

#endif
