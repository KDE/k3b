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

#include "../k3bcddb.h"
#include "../cdinfo/k3bdiskinfo.h"

class KListView;
class QRadioButton;
class KLineEdit;
class K3bPatternParser;
class QToolButton;
class QRadioButton;
class QButtonGroup;


/**
  *@author Sebastian Trueg
  */
class K3bRipperWidget : public KDialogBase
{
  Q_OBJECT

 public: 
  K3bRipperWidget( const K3bDiskInfo&, const K3bCddbEntry&, const QValueList<int>&, 
		   QWidget *parent = 0, const char *name = 0 );
  ~K3bRipperWidget();

 public slots:  
  void refresh();
  void init();

 private:
  K3bDiskInfo m_diskInfo;
  K3bCddbEntry m_cddbEntry;
  QValueList<int> m_trackNumbers;

  K3bPatternParser *m_parser;

  KListView*    m_viewTracks;
  QToolButton*  m_buttonStaticDir;
  QToolButton*  m_buttonPattern;
  KLineEdit*    m_editStaticRipPath;
  QRadioButton* m_useStatic;
  QRadioButton* m_usePattern;
  QLabel*       m_labelSummaryName;

  QButtonGroup* m_groupFileType;
  QRadioButton* m_radioWav;
  QRadioButton* m_radioMp3;
  QRadioButton* m_radioOgg;

  void setupGui();
  void setSongList();
  
 private slots:
  void slotUser1();
  void slotOk();
  void useStatic();
  void usePattern();
  void showPatternDialog();
  void slotFindStaticDir();
};

#endif
