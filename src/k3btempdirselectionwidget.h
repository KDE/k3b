/* 
 *
 * $Id: $
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


#ifndef K3BTEMPDIRSELECTIONWIDGET_H
#define K3BTEMPDIRSELECTIONWIDGET_H

#include <qgroupbox.h>

class QTimer;
class QLabel;
class QLineEdit;
class QToolButton;


/**
  *@author Sebastian Trueg
  */
class K3bTempDirSelectionWidget : public QGroupBox
{
  Q_OBJECT

 public: 
  K3bTempDirSelectionWidget( QWidget *parent = 0, const char *name = 0 );
  ~K3bTempDirSelectionWidget();

  /** determines if the selection dialog should ask for a dir or an file */
  enum mode { DIR, FILE };

  unsigned long freeTempSpace() const { return m_freeTempSpace; }
  QString tempPath() const;

 public slots:
  void setTempPath( const QString& );
  void setSelectionMode( int mode );
  void setNeededSize( unsigned long kb );
  void saveConfig();

 private slots:
  void slotUpdateFreeTempSpace();
  void slotFreeTempSpace(const QString&, unsigned long, unsigned long, unsigned long);
  void slotTempDirButtonPressed();

 private:
  QLabel* m_labelCdSize;
  QLabel* m_labelFreeSpace;
  QLineEdit* m_editDirectory;
  QToolButton* m_buttonFindIsoImage;
  unsigned long m_freeTempSpace;

  unsigned long m_requestedSize;

  int m_mode;
};

#endif
