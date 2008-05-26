/* 
 *
 * $Id$
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BISOIMAGEWRITINGDIALOG_H
#define K3BISOIMAGEWRITINGDIALOG_H

#include <k3binteractiondialog.h>
//Added by qt3to4:
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QLabel>


class QCheckBox;
class K3bWriterSelectionWidget;
class QLabel;
class KUrl;
class K3bMd5Job;
class K3bWritingModeWidget;
class KUrlRequester;
class K3bListView;
class QSpinBox;
class QDragEnterEvent;
class QDropEvent;
class K3ListView;
class Q3ListViewItem;
class QPoint;


/**
  *@author Sebastian Trueg
  */
class K3bIsoImageWritingDialog : public K3bInteractionDialog
{
  Q_OBJECT

 public: 
  K3bIsoImageWritingDialog( QWidget* = 0 );
  ~K3bIsoImageWritingDialog();

  void setImage( const KUrl& url );

 protected Q_SLOTS:
  void slotStartClicked();
  void updateImageSize( const QString& );
  void slotWriterChanged();
  void slotMd5JobPercent( int );
  void slotMd5JobFinished( bool );
  void slotContextMenu( K3ListView*, Q3ListViewItem*, const QPoint& pos );

 protected:
  void loadUserDefaults( const KConfigGroup& );
  void saveUserDefaults( KConfigGroup& );
  void loadK3bDefaults();

  void calculateMd5Sum( const QString& );
  void dragEnterEvent( QDragEnterEvent* );
  void dropEvent( QDropEvent* );

  void init();

 private:
  void setupGui();
  QString imagePath() const;

  K3bMd5Job* m_md5Job;

  K3bWriterSelectionWidget* m_writerSelectionWidget;
  QCheckBox* m_checkDummy;
  QCheckBox* m_checkVerify;
  QSpinBox* m_spinCopies;
  K3bWritingModeWidget* m_writingModeWidget;

  KUrlRequester* m_editImagePath;
  K3bListView* m_infoView;

  class Private;
  Private* d;
};

#endif
