/***************************************************************************
                          k3bisoimagewritingdialog.h  -  description
                             -------------------
    begin                : Fri Nov 30 2001
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

#ifndef K3BISOIMAGEWRITINGDIALOG_H
#define K3BISOIMAGEWRITINGDIALOG_H

#include <kdialogbase.h>


class QCheckBox;
class K3bWriterSelectionWidget;
class KLineEdit;
class QToolButton;
class QLabel;
class K3bIsoImageJob;
class KURL;

/**
  *@author Sebastian Trueg
  */
class K3bIsoImageWritingDialog : public KDialogBase
{
Q_OBJECT

 public: 
  K3bIsoImageWritingDialog( QWidget* = 0, const char* = 0, bool = true );
  ~K3bIsoImageWritingDialog();

  void setImage( const KURL& url );

 protected slots:
  void slotUser1();
  void slotUser2();
  void updateImageSize( const QString& );
  void slotFindImageFile();
  void slotWriterChanged();
  void slotCueBinChecked( bool c );
  void slotCheckMd5Sum();

 private:
  void setupGui();

  K3bIsoImageJob* m_job;

  K3bWriterSelectionWidget* m_writerSelectionWidget;
  QCheckBox* m_checkDummy;
  QCheckBox* m_checkDao;
  QCheckBox* m_checkBurnProof;

  QCheckBox* m_checkRawWrite;
  QCheckBox* m_checkUseCueFile;
  QCheckBox* m_checkNoFix;

  QLabel*    m_labelImageSize;
  KLineEdit* m_editImagePath;
  QToolButton* m_buttonFindImageFile;


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
  bool m_bCueBinAvailable;
  QString m_cuePath;
  QString m_binPath;
};

#endif
