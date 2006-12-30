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


#ifndef K3BTEMPDIRSELECTIONWIDGET_H
#define K3BTEMPDIRSELECTIONWIDGET_H

#include <qgroupbox.h>
#include <kio/global.h>

class QTimer;
class QLabel;
class KURLRequester;
class KConfigBase;


/**
  *@author Sebastian Trueg
  */
class K3bTempDirSelectionWidget : public QGroupBox
{
  Q_OBJECT

 public: 
  K3bTempDirSelectionWidget( QWidget *parent = 0, const char *name = 0 );
  ~K3bTempDirSelectionWidget();

  /** determines if the selection dialog should ask for a dir or a file */
  enum mode { DIR, FILE };

  int selectionMode() const { return m_mode; }

  /**
   * \return Free space in KB
   * FIXME: use KIO::filesize_t and return the number of bytes
   */
  unsigned long freeTempSpace() const;
  QString tempPath() const;
  QString tempDirectory() const;

  /**
   * Use this if you don't want K3bTempDirSelectionWidget to modify the
   * user input based on the mode.
   */
  QString plainTempPath() const;

 public slots:
  void setTempPath( const QString& );
  void setSelectionMode( int mode );
  void setNeededSize( KIO::filesize_t bytes );

  /**
   * saves the current path as the global default tempd dir.
   */
  void saveConfig();

  void readConfig( KConfigBase* );
  void saveConfig( KConfigBase* );

 private slots:
  void slotUpdateFreeTempSpace();
  void slotTempDirButtonPressed( KURLRequester* );

 private:
  QLabel* m_imageFileLabel;
  QLabel* m_labelCdSize;
  QLabel* m_labelFreeSpace;
  KURLRequester* m_editDirectory;

  mutable unsigned long m_freeTempSpace;
  unsigned long m_requestedSize;

  int m_mode;
};

#endif
