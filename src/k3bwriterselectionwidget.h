/***************************************************************************
                          k3bwriterselectionwidget.h  -  description
                             -------------------
    begin                : Mon Dec 3 2001
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

#ifndef K3BWRITERSELECTIONWIDGET_H
#define K3BWRITERSELECTIONWIDGET_H

#include <qwidget.h>


class QComboBox;
class QRadioButton;
class QButtonGroup;

class K3bDevice;
class KConfig;



/**
  *@author Sebastian Trueg
  */
class K3bWriterSelectionWidget : public QWidget
{
   Q_OBJECT

 public: 
  K3bWriterSelectionWidget(QWidget *parent=0, const char *name=0);
  ~K3bWriterSelectionWidget();

  int writerSpeed() const;
  K3bDevice* writerDevice() const;

  /**
   * returns K3b::WritingApp
   * DEFAULT, CDRECORD, CDRDAO
   */
  int writingApp() const;

 signals:
  void writerChanged();
  void writingAppChanged( int app );

 private slots:
  void slotRefreshWriterSpeeds();
  void slotWritingAppSelected( int id );
  void slotConfigChanged( KConfig* c );
  void slotSpeedChanged( int index );

 private:
  QComboBox* m_comboSpeed;
  QComboBox* m_comboWriter;

  QButtonGroup* m_groupCdWritingApp;
  QRadioButton* m_selectDefault;
  QRadioButton* m_selectCdrecord;
  QRadioButton* m_selectCdrdao;
};

#endif
