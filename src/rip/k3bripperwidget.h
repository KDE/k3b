/***************************************************************************
                          k3bripperwidget.h  -  description
                             -------------------
    begin                : Tue Mar 27 2001
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

#ifndef K3BRIPPERWIDGET_H
#define K3BRIPPERWIDGET_H


#include <qvbox.h>
#include <kdialogbase.h>
#include <qstringlist.h>

#include <cddb/k3bcddbquery.h>

#include "../cdinfo/k3bdiskinfo.h"

class KListView;
class KLineEdit;
class QToolButton;
class QButtonGroup;
class QRadioButton;
class QCheckBox;
class QSpinBox;


/**
  *@author Sebastian Trueg
  */
class K3bRipperWidget : public KDialogBase
{
  Q_OBJECT

 public: 
  K3bRipperWidget( const K3bDiskInfo&, const K3bCddbResultEntry&, const QValueList<int>&, 
		   QWidget *parent = 0, const char *name = 0 );
  ~K3bRipperWidget();
  void setStaticDir( const QString& path );

 public slots:  
  void refresh();
  void init();

 private:
  K3bDiskInfo m_diskInfo;
  K3bCddbResultEntry m_cddbEntry;
  QValueList<int> m_trackNumbers;

  KListView*    m_viewTracks;
  QToolButton*  m_buttonStaticDir;
  QToolButton*  m_buttonPattern;
  KLineEdit*    m_editStaticRipPath;
  QCheckBox*    m_checkUsePattern;

  QButtonGroup* m_groupFileType;
  QRadioButton* m_radioWav;
  QRadioButton* m_radioMp3;
  QRadioButton* m_radioOgg;

  QComboBox* m_comboParanoiaMode;
  QSpinBox* m_spinRetries;

  void setupGui();
  void setupContextHelp();
  
 private slots:
  void slotUser1();
  void slotOk();
  void showPatternDialog();
  void slotFindStaticDir();
};

#endif
